#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace rex::graphics::gta4_native {

// Tracks commands actually emitted into one command buffer. Owns no GPU
// resources. Reset at recording boundaries and after any external draw path.
// Floating-point state is passed as bits, preserving signed zero and NaN values.
// Uses compact validity bitmasks so Reset() clears 3 uint32_t words instead of
// zeroing ~1.5 KB of std::optional storage.
template <size_t DescriptorSetCount>
class NativeDrawStateCache {
 public:
  struct IndexedDescriptorKey {
    uint64_t layout = 0;
    uint32_t descriptor_page = 0;
    uint32_t descriptor_copy = 0;
    uint64_t image_descriptor_epoch = 0;
    uint32_t active_frame_slot = 0;
    bool operator==(const IndexedDescriptorKey&) const = default;
  };
  struct DynamicDrawKey {
    const void* fixed_state = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t logical_width = 0;
    uint32_t logical_height = 0;
    uint32_t color_write_mask = 0;
    uint32_t debug_gpu_attribution = 0;
    uint64_t color_formats_lo = 0;
    uint64_t color_formats_hi = 0;
    bool operator==(const DynamicDrawKey&) const = default;
  };
  struct RawViewportKey {
    std::array<uint32_t, 6> viewport_bits{};
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t logical_width = 0;
    uint32_t logical_height = 0;
    bool operator==(const RawViewportKey&) const = default;
  };
  struct RawScissorKey {
    std::array<int32_t, 4> scissor{};
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t logical_width = 0;
    uint32_t logical_height = 0;
    uint32_t flags = 0;  // bit 0: viewport_empty, bit 1: debug_gpu_attr
    bool operator==(const RawScissorKey&) const = default;
  };
  struct RawDepthBiasKey {
    uint32_t depth_bias_bits = 0;
    uint32_t slope_scaled_bits = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t logical_width = 0;
    uint32_t logical_height = 0;
    bool operator==(const RawDepthBiasKey&) const = default;
  };
  struct RawStencilKey {
    uint64_t packed_masks_and_mode = 0;
    bool operator==(const RawStencilKey&) const = default;
  };
  struct RawBlendConstantsKey {
    std::array<uint32_t, 4> blend_constants{};
    std::array<uint32_t, 4> blend_controls{};
    uint32_t color_write_mask = 0;
    uint64_t color_formats_lo = 0;
    uint64_t color_formats_hi = 0;
    bool operator==(const RawBlendConstantsKey&) const = default;
  };

  void Reset() noexcept {
    valid_mask_ = 0;
    raw_valid_mask_ = 0;
    vertex_buffers_valid_mask_ = 0;
  }

  void ResetPipelineDescriptorsViewportScissor() noexcept {
    constexpr uint32_t kClobbered =
        kBitPipeline | kBitDescriptors | kBitIndexedDescriptors | kBitDynamicDraw |
        kBitViewport | kBitScissor | kBitPushConstants;
    valid_mask_ &= ~kClobbered;
    raw_valid_mask_ &= ~(kRawBitViewport | kRawBitScissor);
  }

  bool UpdatePipeline(uint64_t pipeline) noexcept {
    // All graphics pipelines in Gta4NativeGraphicsSystem declare VIEWPORT, SCISSOR,
    // DEPTH_BIAS, STENCIL_* and BLEND_CONSTANTS as VK_DYNAMIC_STATE_*, which are
    // command-buffer bound in Vulkan and not invalidated across pipeline switches.
    return UpdateTracked(kBitPipeline, pipeline_, pipeline);
  }

  bool HasMatchingIndexedDescriptors(const IndexedDescriptorKey& key) const noexcept {
    return (valid_mask_ & kBitIndexedDescriptors) != 0 && indexed_descriptors_ == key;
  }
  void RecordIndexedDescriptors(const IndexedDescriptorKey& key) noexcept {
    indexed_descriptors_ = key;
    valid_mask_ |= kBitIndexedDescriptors;
  }

  bool HasMatchingDynamicDraw(const DynamicDrawKey& key) const noexcept {
    return (valid_mask_ & kBitDynamicDraw) != 0 && dynamic_draw_ == key;
  }
  void RecordDynamicDraw(const DynamicDrawKey& key) noexcept {
    dynamic_draw_ = key;
    valid_mask_ |= kBitDynamicDraw;
  }

  bool HasMatchingRawViewport(const RawViewportKey& key, bool& empty_out) const noexcept {
    if ((raw_valid_mask_ & kRawBitViewport) != 0 && raw_viewport_ == key) {
      empty_out = raw_viewport_empty_;
      return true;
    }
    return false;
  }
  void RecordRawViewport(const RawViewportKey& key, bool empty) noexcept {
    raw_viewport_ = key;
    raw_viewport_empty_ = empty;
    raw_valid_mask_ |= kRawBitViewport;
  }

  bool HasMatchingRawScissor(const RawScissorKey& key) const noexcept {
    return (raw_valid_mask_ & kRawBitScissor) != 0 && raw_scissor_ == key;
  }
  void RecordRawScissor(const RawScissorKey& key) noexcept {
    raw_scissor_ = key;
    raw_valid_mask_ |= kRawBitScissor;
  }

  bool HasMatchingRawDepthBias(const RawDepthBiasKey& key) const noexcept {
    return (raw_valid_mask_ & kRawBitDepthBias) != 0 && raw_depth_bias_ == key;
  }
  void RecordRawDepthBias(const RawDepthBiasKey& key) noexcept {
    raw_depth_bias_ = key;
    raw_valid_mask_ |= kRawBitDepthBias;
  }

  bool HasMatchingRawStencil(uint64_t packed_key) const noexcept {
    return (raw_valid_mask_ & kRawBitStencil) != 0 &&
           raw_stencil_.packed_masks_and_mode == packed_key;
  }
  void RecordRawStencil(uint64_t packed_key) noexcept {
    raw_stencil_.packed_masks_and_mode = packed_key;
    raw_valid_mask_ |= kRawBitStencil;
  }

  bool HasMatchingRawBlendConstants(const RawBlendConstantsKey& key) const noexcept {
    return (raw_valid_mask_ & kRawBitBlendConstants) != 0 && raw_blend_constants_ == key;
  }
  void RecordRawBlendConstants(const RawBlendConstantsKey& key) noexcept {
    raw_blend_constants_ = key;
    raw_valid_mask_ |= kRawBitBlendConstants;
  }

  bool UpdateDescriptors(uint64_t layout,
                         const std::array<uint64_t, DescriptorSetCount>& sets) noexcept {
    if (!UpdateTracked(kBitDescriptors, descriptors_, LayoutValues<DescriptorSetCount>{layout, sets})) {
      return false;
    }
    valid_mask_ &= ~kBitIndexedDescriptors;
    return true;
  }
  bool UpdateViewport(const std::array<uint32_t, 6>& bits) noexcept {
    return UpdateTracked(kBitViewport, viewport_, bits);
  }
  bool UpdateScissor(const std::array<int64_t, 4>& rectangle) noexcept {
    return UpdateTracked(kBitScissor, scissor_, rectangle);
  }
  bool UpdateDepthBias(const std::array<uint32_t, 3>& bits) noexcept {
    return UpdateTracked(kBitDepthBias, depth_bias_, bits);
  }
  bool UpdateStencil(const std::array<uint32_t, 6>& faces) noexcept {
    return UpdateTracked(kBitStencil, stencil_, faces);
  }
  bool UpdateBlendConstants(const std::array<uint32_t, 4>& bits) noexcept {
    return UpdateTracked(kBitBlendConstants, blend_constants_, bits);
  }
  bool UpdatePushConstants(uint64_t layout, const std::array<uint64_t, 3>& addresses) noexcept {
    return UpdateTracked(kBitPushConstants, push_constants_, LayoutValues<3>{layout, addresses});
  }
  // Vertex buffer bindings are command-buffer state that survives pipeline
  // and render-pass changes; the command buffer is part of the value.
  bool UpdateVertexBuffer(uint64_t command_buffer, uint32_t binding, uint64_t buffer,
                          uint64_t offset) noexcept {
    if (binding >= vertex_buffers_.size()) return true;
    const uint32_t bit = uint32_t(1) << binding;
    const std::array<uint64_t, 3> value{command_buffer, buffer, offset};
    if ((vertex_buffers_valid_mask_ & bit) != 0 && vertex_buffers_[binding] == value) {
      return false;
    }
    vertex_buffers_[binding] = value;
    vertex_buffers_valid_mask_ |= bit;
    return true;
  }
  // Index buffer binding is command-buffer state that survives pipeline
  // and render-pass changes.
  bool UpdateIndexBuffer(uint64_t command_buffer, uint64_t buffer, uint64_t offset,
                         uint32_t index_type) noexcept {
    return UpdateTracked(kBitIndexBuffer, index_buffer_,
                         std::array<uint64_t, 4>{command_buffer, buffer, offset, uint64_t(index_type)});
  }

 private:
  static constexpr uint32_t kBitPipeline           = 1u << 0;
  static constexpr uint32_t kBitDescriptors        = 1u << 1;
  static constexpr uint32_t kBitIndexedDescriptors = 1u << 2;
  static constexpr uint32_t kBitDynamicDraw        = 1u << 3;
  static constexpr uint32_t kBitViewport           = 1u << 4;
  static constexpr uint32_t kBitScissor            = 1u << 5;
  static constexpr uint32_t kBitDepthBias          = 1u << 6;
  static constexpr uint32_t kBitStencil            = 1u << 7;
  static constexpr uint32_t kBitBlendConstants     = 1u << 8;
  static constexpr uint32_t kBitPushConstants      = 1u << 9;
  static constexpr uint32_t kBitIndexBuffer        = 1u << 10;

  static constexpr uint32_t kRawBitViewport        = 1u << 0;
  static constexpr uint32_t kRawBitScissor         = 1u << 1;
  static constexpr uint32_t kRawBitDepthBias       = 1u << 2;
  static constexpr uint32_t kRawBitStencil         = 1u << 3;
  static constexpr uint32_t kRawBitBlendConstants  = 1u << 4;

  template <typename Value>
  bool UpdateTracked(uint32_t bit, Value& slot, const Value& value) noexcept {
    if ((valid_mask_ & bit) != 0 && slot == value) {
      return false;
    }
    slot = value;
    valid_mask_ |= bit;
    return true;
  }

  template <size_t Count>
  struct LayoutValues {
    uint64_t layout = 0;
    std::array<uint64_t, Count> values{};
    bool operator==(const LayoutValues&) const = default;
  };

  uint32_t valid_mask_ = 0;
  uint32_t raw_valid_mask_ = 0;
  uint32_t vertex_buffers_valid_mask_ = 0;
  bool raw_viewport_empty_ = false;

  uint64_t pipeline_ = 0;
  LayoutValues<DescriptorSetCount> descriptors_{};
  IndexedDescriptorKey indexed_descriptors_{};
  DynamicDrawKey dynamic_draw_{};
  RawViewportKey raw_viewport_{};
  RawScissorKey raw_scissor_{};
  RawDepthBiasKey raw_depth_bias_{};
  RawStencilKey raw_stencil_{};
  RawBlendConstantsKey raw_blend_constants_{};
  std::array<uint32_t, 6> viewport_{};
  std::array<int64_t, 4> scissor_{};
  std::array<uint32_t, 3> depth_bias_{};
  std::array<uint32_t, 6> stencil_{};
  std::array<uint32_t, 4> blend_constants_{};
  LayoutValues<3> push_constants_{};
  std::array<uint64_t, 4> index_buffer_{};
  std::array<std::array<uint64_t, 3>, 32> vertex_buffers_{};
};

}  // namespace rex::graphics::gta4_native
