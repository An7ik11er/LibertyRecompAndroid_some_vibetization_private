#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>

#if defined(__aarch64__)
#include <arm_neon.h>
#endif

namespace rex::graphics::gta4_native {

// All constant allocations are contiguous, whole float4 registers. One tracker
// belongs to one host-visible frame-slot buffer. BeginFrame is called only after
// the slot fence (or unsubmitted rollback); ResetStorage is required whenever
// that buffer is replaced. No shader ABI or per-draw allocation is changed.
class NativeConstantUploadTracker {
 public:
  static constexpr size_t kRegisterBytes = 16;

  void BeginFrame() { written_bytes_ = 0; }
  void ResetStorage() {
    initialized_bytes_ = 0;
    written_bytes_ = 0;
  }

  std::optional<size_t> Write(std::span<uint8_t> storage, size_t offset,
                              std::span<const uint8_t> source, bool guest_word_order,
                              bool compare_existing = true) {
    if (source.empty() || offset % kRegisterBytes || source.size() % kRegisterBytes ||
        offset > storage.size() || source.size() > storage.size() - offset ||
        initialized_bytes_ > storage.size() ||
        source.size() > std::numeric_limits<size_t>::max() - written_bytes_) {
      return std::nullopt;
    }

    uint8_t* const dst_base = storage.data() + offset;
    const uint8_t* const src_base = source.data();
    const size_t total_bytes = source.size();
    const size_t existing = (compare_existing && offset < initialized_bytes_)
                                ? std::min(total_bytes, initialized_bytes_ - offset)
                                : 0;

    const auto stream_write = [guest_word_order](uint8_t* __restrict dst,
                                                 const uint8_t* __restrict src,
                                                 size_t bytes) noexcept {
      if (!guest_word_order) {
        std::memcpy(dst, src, bytes);
        return;
      }
      size_t i = 0;
#if defined(__aarch64__)
      for (; i + 64 <= bytes; i += 64) {
        uint8x16x4_t block = vld1q_u8_x4(src + i);
        block.val[0] = vrev32q_u8(block.val[0]);
        block.val[1] = vrev32q_u8(block.val[1]);
        block.val[2] = vrev32q_u8(block.val[2]);
        block.val[3] = vrev32q_u8(block.val[3]);
        vst1q_u8_x4(dst + i, block);
      }
      for (; i + 16 <= bytes; i += 16) {
        vst1q_u8(dst + i, vrev32q_u8(vld1q_u8(src + i)));
      }
#else
      for (; i < bytes; i += sizeof(uint32_t)) {
        uint32_t word;
        std::memcpy(&word, src + i, sizeof(word));
        word = std::byteswap(word);
        std::memcpy(dst + i, &word, sizeof(word));
      }
#endif
    };

    // Fast path: no read-before-write (mandatory for write-combined GPU upload memory
    // and first-time arena growth). Pure sequential 64B NEON stores.
    if (existing == 0) {
      stream_write(dst_base, src_base, total_bytes);
      initialized_bytes_ = std::max(initialized_bytes_, offset + total_bytes);
      written_bytes_ += total_bytes;
      return total_bytes;
    }

    size_t written = 0;
    size_t relative = 0;
#if defined(__aarch64__)
    // Single-read, zero-stack-copy NEON compare-and-store for cached buffers.
    for (; relative + 64 <= existing; relative += 64) {
      uint8x16x4_t conv = vld1q_u8_x4(src_base + relative);
      if (guest_word_order) {
        conv.val[0] = vrev32q_u8(conv.val[0]);
        conv.val[1] = vrev32q_u8(conv.val[1]);
        conv.val[2] = vrev32q_u8(conv.val[2]);
        conv.val[3] = vrev32q_u8(conv.val[3]);
      }
      const uint8x16x4_t cur = vld1q_u8_x4(dst_base + relative);
      const uint8x16_t d0 = veorq_u8(conv.val[0], cur.val[0]);
      const uint8x16_t d1 = veorq_u8(conv.val[1], cur.val[1]);
      const uint8x16_t d2 = veorq_u8(conv.val[2], cur.val[2]);
      const uint8x16_t d3 = veorq_u8(conv.val[3], cur.val[3]);
      const uint8x16_t any_diff = vorrq_u8(vorrq_u8(d0, d1), vorrq_u8(d2, d3));
      if (vmaxvq_u32(vreinterpretq_u32_u8(any_diff)) != 0u) {
        const uint32_t m0 = vmaxvq_u32(vreinterpretq_u32_u8(d0)) != 0u ? 1u : 0u;
        const uint32_t m1 = vmaxvq_u32(vreinterpretq_u32_u8(d1)) != 0u ? 1u : 0u;
        const uint32_t m2 = vmaxvq_u32(vreinterpretq_u32_u8(d2)) != 0u ? 1u : 0u;
        const uint32_t m3 = vmaxvq_u32(vreinterpretq_u32_u8(d3)) != 0u ? 1u : 0u;
        const uint32_t dirty_lanes = m0 + m1 + m2 + m3;
        if (dirty_lanes == 4u) {
          vst1q_u8_x4(dst_base + relative, conv);
        } else {
          if (m0) vst1q_u8(dst_base + relative + 0, conv.val[0]);
          if (m1) vst1q_u8(dst_base + relative + 16, conv.val[1]);
          if (m2) vst1q_u8(dst_base + relative + 32, conv.val[2]);
          if (m3) vst1q_u8(dst_base + relative + 48, conv.val[3]);
        }
        written += size_t(dirty_lanes) * kRegisterBytes;
      }
    }
    for (; relative < existing; relative += kRegisterBytes) {
      uint8x16_t conv = vld1q_u8(src_base + relative);
      if (guest_word_order) {
        conv = vrev32q_u8(conv);
      }
      const uint8x16_t cur = vld1q_u8(dst_base + relative);
      const uint8x16_t diff = veorq_u8(conv, cur);
      if (vmaxvq_u32(vreinterpretq_u32_u8(diff)) != 0u) {
        vst1q_u8(dst_base + relative, conv);
        written += kRegisterBytes;
      }
    }
#else
    for (; relative + 64 <= existing; relative += 64) {
      alignas(16) std::array<uint8_t, 64> converted;
      stream_write(converted.data(), src_base + relative, converted.size());
      uint8_t* destination = dst_base + relative;
      if (std::memcmp(destination, converted.data(), converted.size()) == 0) continue;
      for (size_t lane = 0; lane < converted.size(); lane += kRegisterBytes) {
        if (std::memcmp(destination + lane, converted.data() + lane, kRegisterBytes) != 0) {
          std::memcpy(destination + lane, converted.data() + lane, kRegisterBytes);
          written += kRegisterBytes;
        }
      }
    }
    for (; relative < existing; relative += kRegisterBytes) {
      alignas(16) std::array<uint8_t, kRegisterBytes> converted;
      stream_write(converted.data(), src_base + relative, converted.size());
      uint8_t* destination = dst_base + relative;
      if (std::memcmp(destination, converted.data(), converted.size()) != 0) {
        std::memcpy(destination, converted.data(), converted.size());
        written += kRegisterBytes;
      }
    }
#endif

    if (relative < total_bytes) {
      const size_t tail = total_bytes - relative;
      stream_write(dst_base + relative, src_base + relative, tail);
      written += tail;
    }
    initialized_bytes_ = std::max(initialized_bytes_, offset + total_bytes);
    written_bytes_ += written;
    return written;
  }

  size_t initialized_bytes() const { return initialized_bytes_; }
  size_t written_bytes() const { return written_bytes_; }

 private:
  size_t initialized_bytes_ = 0;
  size_t written_bytes_ = 0;
};

}  // namespace rex::graphics::gta4_native
