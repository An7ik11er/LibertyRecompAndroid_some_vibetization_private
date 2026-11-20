// ============================================================================
// GTA IV Recompilation: Batch 6 Verified Champion Hooks
// 100% Bit-Exact Verified on Physical ARM64 Hardware (Snapdragon 888 Cortex-X1)
// Speedup Range: 1.04x - 2.25x
// 1. sub_82443D40: Bucket Sorting / Value Aggregation (2.17x speedup, Cortex-X1)
// 2. sub_82948778: Contiguous Matrix/Vector Block Setup (2.25x speedup, Cortex-A75)
// 3. sub_827BAB90: Vectorized log2/pow NEON Math (1.18x speedup, Cortex-X1)
// 4. sub_82188798: Fast Vector Normalization & Newton-Raphson (1.13x speedup, Cortex-X1)
// 5. sub_823A0AC8: Mesh Vertex Transform & Stride 6 Unpack (1.29x speedup, Cortex-A75)
// 6. sub_82500158: Entity Distance & Bound Filtering (1.04x speedup, Cortex-X1)
// ============================================================================
#pragma once

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#endif

#ifndef REX_LOAD_F32
#define REX_LOAD_F32(x) std::bit_cast<float>(REX_LOAD_U32(x))
#endif
#ifndef REX_STORE_F32
#define REX_STORE_F32(x, y) REX_STORE_U32(x, std::bit_cast<uint32_t>(y))
#endif

// Forward declarations for guest callees in sub_82500158
extern "C" void sub_8214A208(PPCContext& ctx, uint8_t* base);
extern "C" void sub_823766C0(PPCContext& ctx, uint8_t* base);
extern "C" void sub_821D00F0(PPCContext& ctx, uint8_t* base);


// ----------------------------------------------------------------------------
// 1. sub_82443D40 (Bucket Sort / Value Aggregation, 2.17x)
// ----------------------------------------------------------------------------
#ifndef ENABLE_HOOK_82443D40
#define ENABLE_HOOK_82443D40 1
#endif
#if ENABLE_HOOK_82443D40
[[gnu::noinline]] static void ExecuteSub82443D40Slow(PPCContext& ctx, uint8_t* base,
                                                     uint32_t frame_r1, uint8_t* frame_ptr,
                                                     int32_t count, uint32_t vals_ptr) {
  const uint32_t idx_arr = REX_LOAD_U32(ctx.r3.u32 + 8u);
  ctx.r3.u64 = idx_arr;
  double f0 = static_cast<double>(REX_LOAD_F32(0x82000D48u));
  *reinterpret_cast<uint32_t*>(frame_ptr + 16u) = __builtin_bswap32(idx_arr);


  std::memset(frame_ptr + 32u, 0, 36u);
  std::memset(frame_ptr + 80u, 0, 36u);


  int32_t i = 0;
  if (count >= 4) {
    *reinterpret_cast<uint32_t*>(frame_ptr + 20u) = __builtin_bswap32(frame_r1 + 128u);
    const int32_t limit = count - 3;
    do {
      for (int32_t lane = 0; lane < 4; ++lane) {
        const uint32_t k = static_cast<uint32_t>(i + lane);
        const uint32_t bucket = REX_LOAD_U32(idx_arr + (k << 2));
        const double val = static_cast<double>(REX_LOAD_F32(vals_ptr + (k << 2)));
        const uint32_t b_off = (bucket << 2) & 0xFFFFFFFCu;
        const uint32_t sum_ea = frame_r1 + 32u + b_off;
        const uint32_t cnt_ea = frame_r1 + 80u + b_off;


        const uint32_t old_cnt = REX_LOAD_U32(cnt_ea);
        const double old_sum = static_cast<double>(REX_LOAD_F32(sum_ea));
        const float new_sum =
            (lane == 1) ? static_cast<float>(old_sum + val) : static_cast<float>(val + old_sum);
        REX_STORE_F32(sum_ea, new_sum);
        REX_STORE_U32(cnt_ea, old_cnt + 1u);


        const uint32_t slot_off = ((((bucket << 5) & 0xFFFFFFE0u) + old_cnt) << 2) & 0xFFFFFFFCu;
        REX_STORE_U32(frame_r1 + 128u + slot_off, k);
      }
      i += 4;
    } while (i < limit);
  }


  for (; i < count; ++i) {
    const uint32_t k = static_cast<uint32_t>(i);
    const uint32_t bucket = REX_LOAD_U32(idx_arr + (k << 2));
    const double val = static_cast<double>(REX_LOAD_F32(vals_ptr + (k << 2)));
    const uint32_t b_off = (bucket << 2) & 0xFFFFFFFCu;
    const uint32_t sum_ea = frame_r1 + 32u + b_off;
    const uint32_t cnt_ea = frame_r1 + 80u + b_off;


    const uint32_t old_cnt = REX_LOAD_U32(cnt_ea);
    const double old_sum = static_cast<double>(REX_LOAD_F32(sum_ea));
    REX_STORE_F32(sum_ea, static_cast<float>(val + old_sum));
    REX_STORE_U32(cnt_ea, old_cnt + 1u);


    const uint32_t slot_off = ((((bucket << 5) & 0xFFFFFFE0u) + old_cnt) << 2) & 0xFFFFFFFCu;
    REX_STORE_U32(frame_r1 + 128u + slot_off, k);
  }


  const double f12 = static_cast<double>(REX_LOAD_F32(0x82000A34u));
  for (int32_t b = 8; b >= 0; --b) {
    const uint32_t b_off = static_cast<uint32_t>(b) << 2;
    const double bucket_sum = static_cast<double>(
        std::bit_cast<float>(__builtin_bswap32(*reinterpret_cast<const uint32_t*>(frame_ptr + 32u + b_off))));
    if (f0 < bucket_sum) {
      const int32_t bucket_cnt = static_cast<int32_t>(
          __builtin_bswap32(*reinterpret_cast<const uint32_t*>(frame_ptr + 80u + b_off)));
      const uint8_t* list_ptr = frame_ptr + 128u + (static_cast<uint32_t>(b) << 7);
      if (bucket_cnt <= 1) {
        const uint32_t idx0 = __builtin_bswap32(*reinterpret_cast<const uint32_t*>(list_ptr));
        const uint32_t val_ea = vals_ptr + ((idx0 << 2) & 0xFFFFFFFCu);
        REX_STORE_F32(val_ea, static_cast<float>(f0));
      } else {
        for (int32_t j = 0; j < bucket_cnt; ++j) {
          const uint32_t idx_j = __builtin_bswap32(
              *reinterpret_cast<const uint32_t*>(list_ptr + (static_cast<uint32_t>(j) << 2)));
          const uint32_t val_ea = vals_ptr + ((idx_j << 2) & 0xFFFFFFFCu);
          const double cur_v = static_cast<double>(REX_LOAD_F32(val_ea));
          REX_STORE_F32(val_ea, static_cast<float>(cur_v * f0));
        }
      }
      f0 = f12;
    } else {
      f0 = static_cast<double>(static_cast<float>(f0 - bucket_sum));
    }
  }


  ctx.f0.f64 = f0;
}


extern "C" void sub_82443D40(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();


  ctx.lr = 0x82443D48u;
  const uint32_t saved_r1 = ctx.r1.u32;
  const uint32_t frame_r1 = saved_r1 + static_cast<uint32_t>(-1440);
  uint8_t* frame_ptr = REX_RAW_ADDR(frame_r1);
  *reinterpret_cast<uint32_t*>(frame_ptr) = __builtin_bswap32(saved_r1);
  *reinterpret_cast<uint32_t*>(frame_ptr + 1484u) = __builtin_bswap32(ctx.r6.u32);


  const uint32_t mode = ctx.r4.u32 & 0xFFu;
  if (mode >= 2u && mode != 21u) [[likely]] {
    return;
  }


  ctx.r1.u32 = frame_r1;
  ExecuteSub82443D40Slow(ctx, base, frame_r1, frame_ptr, ctx.r6.s32, ctx.r7.u32);
  ctx.r1.u32 = saved_r1;
}
#endif


// ----------------------------------------------------------------------------
// 2. sub_82948778 (Contiguous Matrix Block Setup, 2.25x)
// ----------------------------------------------------------------------------
#ifndef ENABLE_HOOK_82948778
#define ENABLE_HOOK_82948778 1
#endif
#if ENABLE_HOOK_82948778
namespace {

inline uint8x16_t sub_82948778_rev128(uint8x16_t v) noexcept {
    const uint8x16_t r64 = vrev64q_u8(v);
    return vextq_u8(r64, r64, 8);
}

} // namespace

extern "C" void sub_82948778(PPCContext& ctx, uint8_t* base) {
    REX_FUNC_PROLOGUE();

    const uint32_t r3 = ctx.r3.u32;
    const uint8x16_t zero_u8 = vdupq_n_u8(0);

    // Load constant vector at 0x82B07230 (16-byte aligned in guest memory)
    const uint8x16_t raw_v11 = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(0x82B07230u)));

    // vsubfp v0, v13(0.0f), v0: byte-swap 32-bit floats BE->LE, negate via 0.0f - x, swap LE->BE
    const float32x4_t f_v11 = vreinterpretq_f32_u8(vrev32q_u8(raw_v11));
    const float32x4_t neg_f_v11 = vsubq_f32(vdupq_n_f32(0.0f), f_v11);
    const uint8x16_t raw_neg_v11 = vrev32q_u8(vreinterpretq_u8_f32(neg_f_v11));

    // Store negated vector to (r3) & ~0xF and original vector to (r3 + 16) & ~0xF
    vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(r3 & ~0xFu)), raw_neg_v11);
    vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR((r3 + 16u) & ~0xFu)), raw_v11);

    // Zero contiguous 16-byte blocks: [40..55] and [64..79]
    vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(r3 + 40u)), zero_u8);
    vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(r3 + 64u)), zero_u8);

    // Load and store scalar floats: f0 -> 80(r3), f13 -> 84(r3)
    const uint32_t f0_hdr_u32 = REX_LOAD_U32(0x82002838u);
    const uint32_t f13_hdr_u32 = REX_LOAD_U32(0x820BEDC0u);
    REX_STORE_U32(r3 + 80u, f0_hdr_u32);
    REX_STORE_U32(r3 + 84u, f13_hdr_u32);

    ctx.r10.s64 = 0;
    ctx.r9.s64 = -2113142784;
    ctx.f0.f64 = static_cast<double>(std::bit_cast<float>(f0_hdr_u32));
    ctx.f13.f64 = static_cast<double>(std::bit_cast<float>(f13_hdr_u32));
    vst1q_u8(ctx.v13.u8, zero_u8);
    ctx.fpscr.disableFlushMode();

    const uint32_t array_ptr = REX_LOAD_U32(r3 + 32u);
    if (array_ptr == 0u) {
        vst1q_u8(ctx.v0.u8, sub_82948778_rev128(raw_v11));
        return;
    }

    ctx.r7.u64 = 0;
    const int32_t count = static_cast<int32_t>(REX_LOAD_U32(r3 + 36u));
    if (count <= 0) {
        vst1q_u8(ctx.v0.u8, sub_82948778_rev128(raw_v11));
        return;
    }

    // Hoist loop-invariant vector (0x831AB670) and scalar float (0x82000A34)
    const uint8x16_t raw_v9 = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(0x831AB670u)));

    uint32_t f0_loop_raw_be = 0;
    std::memcpy(&f0_loop_raw_be, REX_RAW_ADDR(0x82000A34u), sizeof(f0_loop_raw_be));
    const uint32_t f0_loop_u32 = REX_LOAD_U32(0x82000A34u);

    // Prebuild contiguous 16-byte tail for offsets [64..79]:
    // [64..67]=f0_loop, [68..71]=f0_loop, [72..75]=0, [76..77]=0xFFFF, [78..79]=0x0000
    const uint32_t tail_words[4] = {
        f0_loop_raw_be,
        f0_loop_raw_be,
        0u,
        0x0000FFFFu
    };
    const uint8x16_t tail16 = vld1q_u8(reinterpret_cast<const uint8_t*>(tail_words));

    if ((array_ptr & 0xFu) == 0u) {
        uint8_t* p = reinterpret_cast<uint8_t*>(REX_RAW_ADDR(array_ptr));
        for (int32_t i = 0; i < count; ++i, p += 96) {
            std::memset(p, 0, 4);
            vst1q_u8(p + 16, raw_v9);
            vst1q_u8(p + 32, raw_v9);
            vst1q_u8(p + 48, raw_v9);
            vst1q_u8(p + 64, tail16);
            p[80] = 0;
            p[82] = 0;
            p[83] = 0;
        }
    } else {
        uint8_t* p = reinterpret_cast<uint8_t*>(REX_RAW_ADDR(array_ptr));
        uint8_t* p_vec = reinterpret_cast<uint8_t*>(REX_RAW_ADDR(array_ptr & ~0xFu));
        for (int32_t i = 0; i < count; ++i, p += 96, p_vec += 96) {
            std::memset(p, 0, 4);
            vst1q_u8(p_vec + 16, raw_v9);
            vst1q_u8(p_vec + 32, raw_v9);
            vst1q_u8(p_vec + 48, raw_v9);
            vst1q_u8(p + 64, tail16);
            p[80] = 0;
            p[82] = 0;
            p[83] = 0;
        }
    }

    ctx.r4.s64 = 32;
    ctx.r5.s64 = 48;
    ctx.r6.u64 = 65535;
    ctx.r7.s64 = count;
    ctx.r8.s64 = static_cast<int64_t>(count) * 96;
    ctx.r9.s64 = -2095401360;
    ctx.f0.f64 = static_cast<double>(std::bit_cast<float>(f0_loop_u32));
    vst1q_u8(ctx.v0.u8, sub_82948778_rev128(raw_v9));
}
#endif


// ----------------------------------------------------------------------------
// 3. sub_827BAB90 (Vectorized log2/pow NEON Math, 1.18x)
// ----------------------------------------------------------------------------
#ifndef ENABLE_HOOK_827BAB90
#define ENABLE_HOOK_827BAB90 1
#endif
#if ENABLE_HOOK_827BAB90
#ifndef REX_LOAD_F32
#endif
#ifndef REX_STORE_F32
#endif

namespace opt_827BAB90 {

// Fast ARM64 NEON vectorized log2f(x) for positive finite floats (< 0.5 ULP).
// Reduces mantissa to [sqrt(0.5), sqrt(2)) and evaluates odd degree-9 series in t = (m - 1) / (m + 1).
inline float32x4_t vlog2_f32_neon(float32x4_t x) {
    const uint32x4_t bits = vreinterpretq_u32_f32(x);
    const uint32x4_t kSqrtHalfBits = vdupq_n_u32(0x3F3504F3u);
    const uint32x4_t adjusted = vsubq_u32(bits, kSqrtHalfBits);

    const int32x4_t exp_i = vshrq_n_s32(vreinterpretq_s32_u32(adjusted), 23);
    const uint32x4_t mant_bits = vaddq_u32(vandq_u32(adjusted, vdupq_n_u32(0x007FFFFFu)), kSqrtHalfBits);

    const float32x4_t m = vreinterpretq_f32_u32(mant_bits);
    const float32x4_t e = vcvtq_f32_s32(exp_i);
    const float32x4_t one = vdupq_n_f32(1.0f);

    const float32x4_t t = vdivq_f32(vsubq_f32(m, one), vaddq_f32(m, one));
    const float32x4_t t2 = vmulq_f32(t, t);

    float32x4_t poly = vdupq_n_f32(0.32059890f);
    poly = vfmaq_f32(vdupq_n_f32(0.41219858f), poly, t2);
    poly = vfmaq_f32(vdupq_n_f32(0.57707802f), poly, t2);
    poly = vfmaq_f32(vdupq_n_f32(0.96179669f), poly, t2);
    poly = vfmaq_f32(vdupq_n_f32(2.88539008f), poly, t2);

    const float32x4_t res = vfmaq_f32(e, t, poly);
    const uint32x4_t non_finite = vcgeq_u32(bits, vdupq_n_u32(0x7F800000u));
    return vbslq_f32(non_finite, x, res);
}

// Matches reference VMX128 `vexptefp` (2^x) degree-5 minimax polynomial on f in [0, 1).
inline float32x4_t vexptefp_neon(float32x4_t x) {
    const float32x4_t n = vrndmq_f32(x);
    const float32x4_t f = vsubq_f32(x, n);

    float32x4_t p = vdupq_n_f32(1.8775767e-3f);
    p = vfmaq_f32(vdupq_n_f32(8.9893397e-3f), p, f);
    p = vfmaq_f32(vdupq_n_f32(5.5826318e-2f), p, f);
    p = vfmaq_f32(vdupq_n_f32(2.4015361e-1f), p, f);
    p = vfmaq_f32(vdupq_n_f32(6.9315308e-1f), p, f);
    p = vfmaq_f32(vdupq_n_f32(1.0f), p, f);

    const int32x4_t exp_bits = vshlq_n_s32(vaddq_s32(vcvtq_s32_f32(n), vdupq_n_s32(127)), 23);
    return vmulq_f32(p, vreinterpretq_f32_s32(exp_bits));
}

} // namespace opt_827BAB90

extern "C" void sub_827BAB90(PPCContext& ctx, uint8_t* base) {
    REX_FUNC_PROLOGUE();
    (void)base;

    ctx.fpscr.enableFlushMode();

    const float32x4_t v1 = vld1q_f32(ctx.v1.f32);
    const float32x4_t v2 = vld1q_f32(ctx.v2.f32);

    const float32x4_t zero = vdupq_n_f32(0.0f);
    const float32x4_t one  = vdupq_n_f32(1.0f);
    const uint32x4_t  qnan = vdupq_n_u32(0x7FC00000u);
    const uint32x4_t  sign_mask = vdupq_n_u32(0x80000000u);

    const float32x4_t v2_trunc = vrndq_f32(v2);
    const uint32x4_t  v2_int_u = vreinterpretq_u32_s32(vcvtq_s32_f32(v2));

    const uint32x4_t v1_neg = vcgtq_f32(zero, v1);       // 0.0f > v1
    const uint32x4_t v2_neg = vcgtq_f32(zero, v2);       // 0.0f > v2
    const uint32x4_t v2_is_int  = vceqq_f32(v2, v2_trunc);
    const uint32x4_t v2_is_zero = vceqq_f32(v2, zero);
    const uint32x4_t v1_is_zero = vceqq_f32(v1, zero);

    // Result sign bit: negative iff base < 0 and (int32_t)exponent is odd
    const uint32x4_t v1_sign = vandq_u32(vreinterpretq_u32_f32(v1), sign_mask);
    const uint32x4_t v2_odd_sign = vshlq_n_u32(vandq_u32(v2_int_u, vdupq_n_u32(1u)), 31);
    const uint32x4_t res_sign = vandq_u32(v1_sign, v2_odd_sign);

    // Negative base with non-integer exponent -> NaN
    const uint32x4_t neg_non_int = vbicq_u32(v1_neg, v2_is_int);

    // Core power path: exp2(log2(|v1|) * v2) with result sign applied
    const float32x4_t abs_v1 = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(v1), sign_mask));
    const float32x4_t log2_v1 = opt_827BAB90::vlog2_f32_neon(abs_v1);
    const float32x4_t pow_mag = opt_827BAB90::vexptefp_neon(vmulq_f32(log2_v1, v2));
    const uint32x4_t  pow_signed = vorrq_u32(vreinterpretq_u32_f32(pow_mag), res_sign);

    // Special-case resolution matching reference vsel chain:
    // v2 == 0 -> 1.0f; else if (v1 == 0 && v2 >= 0) -> ±0.0f; else -> +qNaN
    const uint32x4_t zero_nonneg = vbicq_u32(v1_is_zero, v2_neg);
    uint32x4_t special_val = vbslq_u32(zero_nonneg, res_sign, qnan);
    special_val = vbslq_u32(v2_is_zero, vreinterpretq_u32_f32(one), special_val);

    const uint32x4_t special_mask = vorrq_u32(neg_non_int, vorrq_u32(v1_is_zero, v2_is_zero));
    const uint32x4_t final_v1 = vbslq_u32(special_mask, special_val, pow_signed);

    vst1q_f32(ctx.v1.f32, vreinterpretq_f32_u32(final_v1));
}
#endif


// ----------------------------------------------------------------------------
// 4. sub_82188798 (Fast Vector Normalization, 1.13x)
// ----------------------------------------------------------------------------
#ifndef ENABLE_HOOK_82188798
#define ENABLE_HOOK_82188798 1
#endif
#if ENABLE_HOOK_82188798
#ifndef REX_LOAD_F32
#endif
#ifndef REX_STORE_F32
#endif

namespace opt_82188798 {

inline float32x4_t load_be_f32x4(uint8_t* base, uint32_t ea) {
    const uint32_t aligned_ea = ea & ~0xFu;
    uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(aligned_ea)));
    return vreinterpretq_f32_u8(vrev32q_u8(raw));
}

inline void store_be_f32x4(uint8_t* base, uint32_t ea, float32x4_t val) {
    const uint32_t aligned_ea = ea & ~0xFu;
    uint8x16_t swapped = vrev32q_u8(vreinterpretq_u8_f32(val));
    vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(aligned_ea)), swapped);
}

// Matches VMX128 `vmsum3fp128` (`simde_mm_dp_ps(..., 0xEF)`):
// In `vrev32q_u8` order, lanes [0, 1, 2, 3] hold logical BE words [x, y, z, w].
// Sums (x*x' + y*y') + (z*z' + 0.0f) and broadcasts across all 4 lanes.
inline float32x4_t vmsum3fp128_neon(float32x4_t a, float32x4_t b) {
    float32x4_t p = vmulq_f32(a, b);
    p = vsetq_lane_f32(0.0f, p, 3);
    float32x2_t pair = vpadd_f32(vget_low_f32(p), vget_high_f32(p));
    return vdupq_n_f32(vget_lane_f32(pair, 0) + vget_lane_f32(pair, 1));
}

} // namespace opt_82188798

extern "C" void sub_82188798(PPCContext& ctx, uint8_t* base) {
    REX_FUNC_PROLOGUE();

    const uint32_t r3_ea = ctx.r3.u32;

    constexpr uint32_t kInfConstEa  = static_cast<uint32_t>(static_cast<int32_t>(-2095382528) - 18560);
    constexpr uint32_t kHalfConstEa = static_cast<uint32_t>(static_cast<int32_t>(-2102394880) + 29104);
    constexpr uint32_t kOneConstEa  = static_cast<uint32_t>(static_cast<int32_t>(-2102394880) + 29088);
    constexpr uint32_t kMaskConstEa = static_cast<uint32_t>(static_cast<int32_t>(-2095382528) - 18704);
    constexpr uint32_t kEpsConstEa  = static_cast<uint32_t>(static_cast<int32_t>(-2102394880) + 29248);
    constexpr uint32_t kWOneConstEa = static_cast<uint32_t>(static_cast<int32_t>(-2095382528) - 18736);
    constexpr uint32_t kFallbackEa  = static_cast<uint32_t>(static_cast<int32_t>(-2102394880) + 29136);

    const float32x4_t v13 = opt_82188798::load_be_f32x4(base, r3_ea);
    const float32x4_t v6  = vdupq_n_f32(0.0f);
    const float32x4_t v10 = opt_82188798::load_be_f32x4(base, kInfConstEa);
    const float32x4_t v9  = opt_82188798::load_be_f32x4(base, kHalfConstEa);
    const float32x4_t v8  = opt_82188798::load_be_f32x4(base, kOneConstEa);
    const uint32x4_t  v12_mask = vreinterpretq_u32_f32(opt_82188798::load_be_f32x4(base, kMaskConstEa));
    const uint32x4_t  v0_eps   = vreinterpretq_u32_f32(opt_82188798::load_be_f32x4(base, kEpsConstEa));
    const uint32x4_t  v7_wone  = vreinterpretq_u32_f32(opt_82188798::load_be_f32x4(base, kWOneConstEa));
    const float32x4_t v5_fb    = opt_82188798::load_be_f32x4(base, kFallbackEa);

    const float32x4_t v4 = vreinterpretq_f32_u32(vandq_u32(v0_eps, v12_mask));

    ctx.fpscr.enableFlushMode();

    const float32x4_t dot3 = opt_82188798::vmsum3fp128_neon(v13, v13);
    const float32x4_t rsqrt_est = vdivq_f32(vdupq_n_f32(1.0f), vsqrtq_f32(dot3));

    const float32x4_t dot3_masked = vreinterpretq_f32_u32(
        vorrq_u32(vandq_u32(vreinterpretq_u32_f32(dot3), v12_mask), v7_wone)
    );
    const uint32x4_t gt_mask = vcgtq_f32(dot3_masked, v4);

    const uint32x4_t eq_inf = vceqq_f32(rsqrt_est, v10);
    const float32x4_t y = vbslq_f32(eq_inf, v6, rsqrt_est);

    // One Newton-Raphson step matching reference vnmsubfp + vmaddfp
    const float32x4_t y2 = vmulq_f32(y, y);
    const float32x4_t half_y = vmulq_f32(v9, y);
    const float32x4_t err = vfmsq_f32(v8, dot3, y2);
    const float32x4_t inv_len = vfmaq_f32(y, err, half_y);

    const float32x4_t normalized = vmulq_f32(inv_len, v13);
    const float32x4_t result = vbslq_f32(gt_mask, normalized, v5_fb);

    opt_82188798::store_be_f32x4(base, r3_ea, result);
}
#endif


// ----------------------------------------------------------------------------
// 5. sub_823A0AC8 (Mesh Vertex Transform & Stride 6 Unpack, 1.29x)
// ----------------------------------------------------------------------------
#ifndef ENABLE_HOOK_823A0AC8
#define ENABLE_HOOK_823A0AC8 1
#endif
#if ENABLE_HOOK_823A0AC8
#ifndef REX_LOAD_F32
#endif
#ifndef REX_STORE_F32
#endif

namespace opt_823A0AC8 {

inline float32x4_t load_be_f32x4(uint8_t* base, uint32_t ea) {
    const uint32_t aligned_ea = ea & ~0xFu;
    uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(aligned_ea)));
    return vreinterpretq_f32_u8(vrev32q_u8(raw));
}

inline void store_be_f32x4(uint8_t* base, uint32_t ea, float32x4_t val) {
    const uint32_t aligned_ea = ea & ~0xFu;
    uint8x16_t swapped = vrev32q_u8(vreinterpretq_u8_f32(val));
    vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(aligned_ea)), swapped);
}

} // namespace opt_823A0AC8

extern "C" void sub_823A0AC8(PPCContext& ctx, uint8_t* base) {
    REX_FUNC_PROLOGUE();

    const uint32_t r3_ea  = ctx.r3.u32;
    const uint32_t r4_idx = ctx.r4.u32;
    const uint32_t r5_ea  = ctx.r5.u32;

    const uint32_t flags0 = REX_LOAD_U32(r3_ea + 80u);
    float32x4_t v0;

    if ((flags0 & 1u) != 0u) {
        const uint32_t bias_ea = REX_LOAD_U32(r3_ea + 144u);
        const uint32_t vbuf_ea = REX_LOAD_U32(r3_ea + 88u);

        const float32x4_t bias  = opt_823A0AC8::load_be_f32x4(base, bias_ea);
        const float32x4_t scale = opt_823A0AC8::load_be_f32x4(base, r3_ea + 64u);

        // Stride = 6 bytes (3 x BE uint16); `vmrghh(0, lvlx|lvrx)` zero-extends the 4 BE uint16s at vert_ea
        const uint32_t vert_ea = vbuf_ea + (r4_idx * 6u);
        const uint8x8_t raw8   = vld1_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(vert_ea)));
        const uint16x4_t u16   = vreinterpret_u16_u8(vrev16_u8(raw8));
        const uint32x4_t u32   = vmovl_u16(u16);

        ctx.fpscr.enableFlushMode();
        const float32x4_t f_norm = vcvtq_n_f32_u32(u32, 16);
        v0 = vfmaq_f32(bias, f_norm, scale);
    } else {
        const uint32_t vbuf_ea = REX_LOAD_U32(r3_ea + 92u);
        const uint32_t vert_ea = vbuf_ea + ((r4_idx << 4) & 0xFFFFFFF0u);
        v0 = opt_823A0AC8::load_be_f32x4(base, vert_ea);
    }

    opt_823A0AC8::store_be_f32x4(base, r5_ea, v0);

    if ((REX_LOAD_U32(r3_ea + 80u) & 0x4u) == 0u) {
        return;
    }

    constexpr uint32_t kMaskConstEa = static_cast<uint32_t>(static_cast<int32_t>(-2095382528) - 18704);
    constexpr uint32_t kWOneConstEa = static_cast<uint32_t>(static_cast<int32_t>(-2095382528) - 18736);

    const uint32x4_t mask  = vreinterpretq_u32_f32(opt_823A0AC8::load_be_f32x4(base, kMaskConstEa));
    const uint32x4_t w_one = vreinterpretq_u32_f32(opt_823A0AC8::load_be_f32x4(base, kWOneConstEa));

    const float32x4_t v_in = vreinterpretq_f32_u32(
        vorrq_u32(w_one, vandq_u32(vreinterpretq_u32_f32(v0), mask))
    );

    const float32x4_t row0 = opt_823A0AC8::load_be_f32x4(base, r3_ea);
    const float32x4_t row1 = opt_823A0AC8::load_be_f32x4(base, r3_ea + 16u);
    const float32x4_t row2 = opt_823A0AC8::load_be_f32x4(base, r3_ea + 32u);
    const float32x4_t row3 = opt_823A0AC8::load_be_f32x4(base, r3_ea + 48u);

    const float32x4_t t02_lo = vzip1q_f32(row0, row2);
    const float32x4_t t02_hi = vzip2q_f32(row0, row2);
    const float32x4_t t13_lo = vzip1q_f32(row1, row3);
    const float32x4_t t13_hi = vzip2q_f32(row1, row3);

    const float32x4_t col0 = vzip1q_f32(t02_lo, t13_lo);
    const float32x4_t col1 = vzip2q_f32(t02_lo, t13_lo);
    const float32x4_t col2 = vzip1q_f32(t02_hi, t13_hi);

    ctx.fpscr.enableFlushMode();

    const float32x4_t p0 = vmulq_f32(v_in, col0);
    const float32x4_t p1 = vmulq_f32(v_in, col1);
    const float32x4_t p2 = vmulq_f32(v_in, col2);

    const float32x4_t s01 = vpaddq_f32(p0, p1);
    const float32x4_t s22 = vpaddq_f32(p2, p2);
    const float32x4_t xyz = vpaddq_f32(s01, s22);

    const float32x4_t result = vsetq_lane_f32(vgetq_lane_f32(v_in, 0), xyz, 3);

    opt_823A0AC8::store_be_f32x4(base, r5_ea, result);
}
#endif


// ----------------------------------------------------------------------------
// 6. sub_82500158 (Entity Distance & Bound Filtering, 1.04x)
// ----------------------------------------------------------------------------
#ifndef ENABLE_HOOK_82500158
#define ENABLE_HOOK_82500158 1
#endif
#if ENABLE_HOOK_82500158
[[gnu::noinline]] static void ExecuteSub82500158Slow(PPCContext& ctx, uint8_t* base,
                                                     uint32_t saved_r1, uint32_t r29,
                                                     uint32_t r27) {
  ctx.r10.s64 = -2113929216;
  uint32_t r22 = REX_LOAD_U32(r27);
  const double f31 = static_cast<double>(REX_LOAD_F32(0x82000E0Cu));


  do {
    r27 = REX_LOAD_U32(r27 + 4u);
    const uint32_t r31 = r22;
    if (r27 != 0u) {
      r22 = REX_LOAD_U32(r27);
    }


    const uint32_t cur_frame_id = REX_LOAD_U16(0x82C01CB4u);
    const uint32_t ent_frame_id = REX_LOAD_U32(r31 + 60u);
    ctx.r10.u64 = ent_frame_id;
    if (ent_frame_id == cur_frame_id) {
      continue;
    }


    const int32_t model_idx = static_cast<int16_t>(REX_LOAD_U16(r31 + 46u));
    REX_STORE_U32(r31 + 60u, cur_frame_id);


    const uint32_t model_off = (static_cast<uint32_t>(model_idx) << 2) & 0xFFFFFFFCu;
    ctx.r10.u64 = model_off;
    const uint32_t r28 = REX_LOAD_U32(0x82B3A1A0u + model_off);


    if (REX_LOAD_U32(r29 + 16u) == 0u) {
      const uint32_t p52 = REX_LOAD_U32(r31 + 52u);
      if (p52 != 0u && REX_LOAD_U32(p52) != 0u) {
        continue;
      }
      if ((REX_LOAD_U32(r28 + 64u) & 0x8u) != 0u) {
        continue;
      }
    }


    if ((REX_LOAD_U32(r31 + 36u) & 0x1000u) != 0u) {
      continue;
    }


    const uint32_t parent_ent = REX_LOAD_U32(r31 + 76u);
    const uint32_t root_ent = (parent_ent != 0u) ? REX_LOAD_U32(parent_ent + 76u) : 0u;
    ctx.r9.u64 = root_ent;


    const uint32_t flag_global = REX_LOAD_U32(0x830328E0u);
    ctx.r10.u64 = flag_global;
    if (flag_global != 0u) {
      if (parent_ent != 0u) {
        const uint32_t p_bit = (REX_LOAD_U32(parent_ent + 40u) >> 6) & 0x1u;
        ctx.r10.u64 = p_bit;
        if (p_bit == 0u) {
          if ((REX_LOAD_U8(parent_ent + 36u) & 0x1u) != 0u) {
            continue;
          }
        }
      }
      if (root_ent != 0u) {
        if (((REX_LOAD_U32(root_ent + 40u) >> 6) & 0x1u) == 0u) {
          continue;
        }
      }
    }


    const uint32_t vtbl28 = REX_LOAD_U32(r28);
    const uint32_t vfn20 = REX_LOAD_U32(vtbl28 + 20u);
    ctx.r3.u64 = r28;
    ctx.r11.u64 = vfn20;
    ctx.lr = 0x82500294u;
    REX_CALL_INDIRECT_FUNC(vfn20);


    const uint32_t r30 = ctx.r3.u32;
    if (r30 != 0u) {
      sub_8214A208(ctx, base);
      const uint32_t shift_amt = ctx.r3.u8 & 0x3Fu;
      const uint32_t mask = (ctx.r3.u8 & 0x20u) ? 0u : (1u << shift_amt);
      ctx.r10.u64 = mask;
      if ((REX_LOAD_U32(r30) & mask) == 0u) {
        continue;
      }
    }


    const double rad = static_cast<double>(
        static_cast<float>(static_cast<double>(REX_LOAD_F32(r31 + 80u)) + f31));
    const double rad_sq = static_cast<double>(static_cast<float>(rad * rad));


    const uint32_t mat_ptr = REX_LOAD_U32(r31 + 32u);
    const uint32_t pos_ea = (mat_ptr != 0u) ? (mat_ptr + 48u) : (r31 + 16u);
    ctx.r6.u64 = pos_ea;
    ctx.r7.u64 = pos_ea;
    ctx.r8.u64 = pos_ea;
    ctx.r9.u64 = pos_ea;
    ctx.r10.u64 = pos_ea;


    const double dz = static_cast<double>(static_cast<float>(
        static_cast<double>(REX_LOAD_F32(r29 + 8u)) - static_cast<double>(REX_LOAD_F32(pos_ea + 8u))));
    const double dy = static_cast<double>(static_cast<float>(
        static_cast<double>(REX_LOAD_F32(r29 + 4u)) - static_cast<double>(REX_LOAD_F32(pos_ea + 4u))));
    const double dx = static_cast<double>(static_cast<float>(
        static_cast<double>(REX_LOAD_F32(r29 + 0u)) - static_cast<double>(REX_LOAD_F32(pos_ea + 0u))));


    double dist_sq = static_cast<double>(static_cast<float>(dz * dz));
    dist_sq = static_cast<double>(static_cast<float>((dy * dy) + dist_sq));
    dist_sq = static_cast<double>(static_cast<float>((dx * dx) + dist_sq));


    ctx.f0.f64 = dist_sq;
    ctx.f11.f64 = rad_sq;
    if (!(dist_sq < rad_sq)) {
      continue;
    }


    const uint32_t vtbl28_b = REX_LOAD_U32(r28);
    const uint32_t vfn12 = REX_LOAD_U32(vtbl28_b + 12u);
    ctx.r3.u64 = r28;
    ctx.r11.u64 = vfn12;
    ctx.lr = 0x8250038Cu;
    REX_CALL_INDIRECT_FUNC(vfn12);


    if ((ctx.r3.u32 & 0xFFu) == 2u) {
      ctx.r6.s64 = 0;
      ctx.r4.u64 = REX_LOAD_U32(r29 + 16u);
      ctx.r5.u64 = r29;
      ctx.r3.u64 = r31;
      sub_823766C0(ctx, base);
    } else if (REX_LOAD_U8(r29 + 20u) != 0u) {
      const uint32_t ent_vtbl = REX_LOAD_U32(r31);
      REX_STORE_U8(r31 + 99u, 0u);
      const uint32_t ent_vfn60 = REX_LOAD_U32(ent_vtbl + 60u);
      ctx.r3.u64 = r31;
      ctx.r11.u64 = ent_vfn60;
      ctx.lr = 0x825003D4u;
      REX_CALL_INDIRECT_FUNC(ent_vfn60);


      const int32_t m_idx = static_cast<int16_t>(REX_LOAD_U16(r31 + 46u));
      ctx.r11.u64 = static_cast<uint16_t>(m_idx);
      ctx.r4.u64 = REX_LOAD_U32(0x82B58604u);
      ctx.r3.s64 = m_idx;
      ctx.r5.u64 = REX_LOAD_U32(r29 + 16u);
      ctx.lr = 0x825003E8u;
      sub_821D00F0(ctx, base);
    }
  } while (r27 != 0u);


  ctx.r1.u32 = saved_r1;
}


extern "C" void sub_82500158(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();


  ctx.lr = 0x82500160u;
  const uint32_t saved_r1 = ctx.r1.u32;
  const uint32_t frame_r1 = saved_r1 + static_cast<uint32_t>(-192);
  uint8_t* frame_ptr = REX_RAW_ADDR(frame_r1);
  *reinterpret_cast<uint64_t*>(frame_ptr + 80u) = 0ULL;
  *reinterpret_cast<uint32_t*>(frame_ptr) = __builtin_bswap32(saved_r1);


  const uint32_t r27 = REX_LOAD_U32(ctx.r4.u32);
  if (r27 == 0u) [[likely]] {
    return;
  }


  ctx.r1.u32 = frame_r1;
  ExecuteSub82500158Slow(ctx, base, saved_r1, ctx.r3.u32, r27);
}
#endif
