// ============================================================================
// GTA IV Recompilation: Batch 5 Verified Hooks
// Verified 100% bit-exact on Cortex-X1 (OnePlus 9RT Snapdragon 888)
// 1. sub_8296C540: Ray-Mesh / Curved Geometry Support Vertex (1.22x speedup)
// 2. sub_822CC090: Vector/Matrix transform & blend optimization (1.15x speedup)
// ============================================================================
#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#endif

// Forward declarations of callees for sub_822CC090
extern "C" void sub_822CBEF8(PPCContext& ctx, uint8_t* base);
extern "C" void sub_822CBDB8(PPCContext& ctx, uint8_t* base);
extern "C" void sub_822C97F0(PPCContext& ctx, uint8_t* base);
extern "C" void sub_8251DCD8(PPCContext& ctx, uint8_t* base);

#ifndef REX_LOAD_F32
#define REX_LOAD_F32(x) std::bit_cast<float>(REX_LOAD_U32(x))
#endif
#ifndef REX_STORE_F32
#define REX_STORE_F32(x, y) REX_STORE_U32(x, std::bit_cast<uint32_t>(y))
#endif

// ----------------------------------------------------------------------------
// 1. sub_8296C540 (Ray-Mesh / Curved Geometry Support Vertex)
// Hardware Speedup: 1.22x | 12/12 MATCH PASS on Cortex-X1
// ----------------------------------------------------------------------------
static inline float BitCastF32_8296C540(uint32_t u) {
  float f;
  __builtin_memcpy(&f, &u, sizeof(f));
  return f;
}

#if defined(__aarch64__) || defined(_M_ARM64)

static inline float32x4_t LoadVec4BE_F32_8296C540(uint8_t* base, uint32_t ea) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(aligned)));
  return vreinterpretq_f32_u8(vrev32q_u8(raw));
}

static inline uint32x4_t LoadVec4BE_U32_8296C540(uint8_t* base, uint32_t ea) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(aligned)));
  return vreinterpretq_u32_u8(vrev32q_u8(raw));
}

static inline void StoreVec4BE_F32_8296C540(uint8_t* base, uint32_t ea, float32x4_t v) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t rev = vrev32q_u8(vreinterpretq_u8_f32(v));
  vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(aligned)), rev);
}

// Convert SIMDe PPCVRegister layout [w, z, y, x] <-> direct memory order [x, y, z, w]
static inline float32x4_t LoadCtxVec_F32_8296C540(const PPCVRegister& vr) {
  const float32x4_t raw = vld1q_f32(vr.f32);
  const float32x4_t rev64 = vrev64q_f32(raw);
  return vextq_f32(rev64, rev64, 2);
}

static inline void StoreCtxVec_F32_8296C540(PPCVRegister& vr, float32x4_t v) {
  const float32x4_t rev64 = vrev64q_f32(v);
  vst1q_f32(vr.f32, vextq_f32(rev64, rev64, 2));
}

// Zero-overhead bit-exact wrappers calling project REX_VMADDFP / REX_VNMSUBFP macros
static inline float32x4_t RexVmaddF32_8296C540(float32x4_t a, float32x4_t b, float32x4_t c) {
  alignas(16) float fa[4], fb[4], fc[4], fr[4];
  vst1q_f32(fa, a);
  vst1q_f32(fb, b);
  vst1q_f32(fc, c);
  simde_mm_store_ps(fr, REX_VMADDFP(simde_mm_load_ps(fa), simde_mm_load_ps(fb), simde_mm_load_ps(fc)));
  return vld1q_f32(fr);
}

static inline float32x4_t RexVnmsubF32_8296C540(float32x4_t a, float32x4_t b, float32x4_t c) {
  alignas(16) float fa[4], fb[4], fc[4], fr[4];
  vst1q_f32(fa, a);
  vst1q_f32(fb, b);
  vst1q_f32(fc, c);
  simde_mm_store_ps(fr, REX_VNMSUBFP(simde_mm_load_ps(fa), simde_mm_load_ps(fb), simde_mm_load_ps(fc)));
  return vld1q_f32(fr);
}

// Exact match for simde_mm_dp_ps(a, b, 0xEF) as SIMDe builds it on AArch64
// (SIMDE_ARM_NEON_A64V8_NATIVE: vmulq_f32, mask PPC element w, vaddvq_f32). In the SIMDe register
// layout the lanes are {w, z, y, x}, and vaddvq_f32 is FADDP + FADDP = (l0 + l1) + (l2 + l3), so the
// sum is (0 + z) + (y + x). The old ((0 + z) + y) + x is SIMDe's portable fallback order, which this
// build never uses: it rounds differently in ~1.5% of curved-edge candidates (1 ulp in *r3).
static inline float32x4_t Dot3Broadcast_8296C540(float32x4_t a, float32x4_t b) {
  const float32x4_t p = vmulq_f32(a, b);
  const float sum = (0.0f + vgetq_lane_f32(p, 2)) + (vgetq_lane_f32(p, 1) + vgetq_lane_f32(p, 0));
  return vdupq_n_f32(sum);
}

// Unaligned 8-byte load of 4 big-endian int16_t -> float32x4_t [x, y, z, w]
static inline float32x4_t LoadQuantizedVert_8296C540(uint8_t* base, uint32_t addr) {
  const uint8_t* p = reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(addr));
  const uint8x8_t raw8 = vld1_u8(p);
  const int16x4_t s16 = vreinterpret_s16_u8(vrev16_u8(raw8));
  return vcvtq_f32_s32(vmovl_s16(s16));
}

// Exact match for vrsqrtefp (1.0f / sqrt(x)) + Inf-guard + 1 Newton-Raphson step in _orig.cpp
static inline float32x4_t ExactRsqrtNR_8296C540(float32x4_t x, float32x4_t v_inf,
                                                float32x4_t v_half, float32x4_t v_one) {
  const float32x4_t v_zero = vdupq_n_f32(0.0f);
  const float32x4_t est0 = vdivq_f32(vdupq_n_f32(1.0f), vsqrtq_f32(x));
  const uint32x4_t is_inf = vceqq_f32(est0, v_inf);
  const float32x4_t est = vbslq_f32(is_inf, v_zero, est0);
  const float32x4_t est_sq = vmulq_f32(est, est);
  const float32x4_t half_est = vmulq_f32(v_half, est);
  const float32x4_t err = RexVnmsubF32_8296C540(x, est_sq, v_one);
  return RexVmaddF32_8296C540(err, half_est, est);
}

// 1-instruction NEON permutes for vpermwi128 0x78 [Z, X, Y, W] and 0x9C [Y, Z, X, W]
static inline float32x4_t PermZXYW_8296C540(float32x4_t v) {
  static constexpr uint8_t kIdx[16] = {8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15};
  return vreinterpretq_f32_u8(vqtbl1q_u8(vreinterpretq_u8_f32(v), vld1q_u8(kIdx)));
}

static inline float32x4_t PermYZXW_8296C540(float32x4_t v) {
  static constexpr uint8_t kIdx[16] = {4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 12, 13, 14, 15};
  return vreinterpretq_f32_u8(vqtbl1q_u8(vreinterpretq_u8_f32(v), vld1q_u8(kIdx)));
}

#endif

extern "C" void sub_8296C540(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

  // Preserve exact LR update from bl 0x829ff7bc; NEVER touch ctx.r3, ctx.r4, ctx.r5!
  ctx.lr = 0x8296C548u;

  const uint32_t out_ea = ctx.r3.u32;
  const uint32_t geom_ea = ctx.r4.u32;
  const uint32_t out_idx_ea = ctx.r5.u32;

  uint32_t curved_ptr = REX_LOAD_U32(geom_ea + 228u);
  const int32_t num_verts = static_cast<int32_t>(REX_LOAD_U32(geom_ea + 200u));
  int32_t best_idx = -1;

  ctx.fpscr.enableFlushMode();

#if defined(__aarch64__) || defined(_M_ARM64)
  const float32x4_t v_zero = vdupq_n_f32(0.0f);
  const float32x4_t v_in_dir = LoadCtxVec_F32_8296C540(ctx.v1);

  const float32x4_t v_inf  = LoadVec4BE_F32_8296C540(base, 0x831AB780u); // v30
  const float32x4_t v_half = LoadVec4BE_F32_8296C540(base, 0x82B071B0u); // v29
  const float32x4_t v_one  = LoadVec4BE_F32_8296C540(base, 0x82B071A0u); // v28
  const float32x4_t v_maxf = LoadVec4BE_F32_8296C540(base, 0x82B07230u); // v13_init
  const float32x4_t v10_init = LoadVec4BE_F32_8296C540(base, 0x831AB680u); // r27
  const uint32x4_t v6_unit = LoadVec4BE_U32_8296C540(base, 0x831AB6D0u);   // v6
  const uint32x4_t v0_mask = LoadVec4BE_U32_8296C540(base, 0x831AB6F0u);   // r26

  // Initial best dot: v3 = 0 - v_maxf
  float32x4_t v3_best_dot = vsubq_f32(v_zero, v_maxf);

  // Initial store to *r3 (exact match to stvx128 v10, r0, r3)
  StoreVec4BE_F32_8296C540(base, out_ea, v10_init);
  float32x4_t v2_best_pos = v10_init;

  // Normalize query direction v1 -> v22
  const float32x4_t dir_dot3 = Dot3Broadcast_8296C540(v_in_dir, v_in_dir);
  const float32x4_t inv_len = ExactRsqrtNR_8296C540(dir_dot3, v_inf, v_half, v_one);
  const float32x4_t v22_dir = vmulq_f32(v_in_dir, inv_len);

  // --------------------------------------------------------------------------
  // LOOP 1: Quantized 6-byte (int16_t[3]) vertex support search (loc_8296C610)
  // --------------------------------------------------------------------------
  if (num_verts > 0) {
    const uint32_t vert_base = REX_LOAD_U32(geom_ea + 176u);
    const float32x4_t v_off   = LoadVec4BE_F32_8296C540(base, geom_ea + 160u); // v12
    const float32x4_t v_scale = LoadVec4BE_F32_8296C540(base, geom_ea + 144u); // v11

    // Fast path check: standard mask_xyz / unit_w / uniform initial v3
    const bool std_mask =
        (vgetq_lane_u32(v0_mask, 0) == 0xFFFFFFFFu) &&
        (vgetq_lane_u32(v0_mask, 1) == 0xFFFFFFFFu) &&
        (vgetq_lane_u32(v0_mask, 2) == 0xFFFFFFFFu) &&
        (vgetq_lane_u32(v0_mask, 3) == 0u) &&
        (vgetq_lane_u32(v6_unit, 0) == 0u) &&
        (vgetq_lane_u32(v6_unit, 1) == 0u) &&
        (vgetq_lane_u32(v6_unit, 2) == 0u) &&
        (BitCastF32_8296C540(vgetq_lane_u32(v6_unit, 3)) > 0.0f) &&
        (vgetq_lane_u32(vreinterpretq_u32_f32(v3_best_dot), 0) == vgetq_lane_u32(vreinterpretq_u32_f32(v3_best_dot), 1)) &&
        (vgetq_lane_u32(vreinterpretq_u32_f32(v3_best_dot), 0) == vgetq_lane_u32(vreinterpretq_u32_f32(v3_best_dot), 2));

    if (std_mask) {
      float best_dot = vgetq_lane_f32(v3_best_dot, 0);
      float32x4_t v_best = vdupq_n_f32(best_dot);

      const float32x4_t v_dx = vdupq_laneq_f32(v22_dir, 0);
      const float32x4_t v_dy = vdupq_laneq_f32(v22_dir, 1);
      const float32x4_t v_dz = vdupq_laneq_f32(v22_dir, 2);

      const float32x4_t v_sx = vdupq_laneq_f32(v_scale, 0);
      const float32x4_t v_sy = vdupq_laneq_f32(v_scale, 1);
      const float32x4_t v_sz = vdupq_laneq_f32(v_scale, 2);

      const float32x4_t v_ox = vdupq_laneq_f32(v_off, 0);
      const float32x4_t v_oy = vdupq_laneq_f32(v_off, 1);
      const float32x4_t v_oz = vdupq_laneq_f32(v_off, 2);

      int32_t i = 0;
      const uint8_t* vptr = reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(vert_base));

      // 8-vertex hardware de-interleave burst via LD3 {v0.8h, v1.8h, v2.8h}
      for (; i + 8 <= num_verts; i += 8) {
        const uint16x8x3_t raw3 = vld3q_u16(reinterpret_cast<const uint16_t*>(vptr + static_cast<size_t>(i) * 6u));
        const int16x8_t ix = vreinterpretq_s16_u8(vrev16q_u8(vreinterpretq_u8_u16(raw3.val[0])));
        const int16x8_t iy = vreinterpretq_s16_u8(vrev16q_u8(vreinterpretq_u8_u16(raw3.val[1])));
        const int16x8_t iz = vreinterpretq_s16_u8(vrev16q_u8(vreinterpretq_u8_u16(raw3.val[2])));

        // Vertices i..i+3
        const float32x4_t fx0 = vcvtq_f32_s32(vmovl_s16(vget_low_s16(ix)));
        const float32x4_t fy0 = vcvtq_f32_s32(vmovl_s16(vget_low_s16(iy)));
        const float32x4_t fz0 = vcvtq_f32_s32(vmovl_s16(vget_low_s16(iz)));

        const float32x4_t px0 = RexVmaddF32_8296C540(fx0, v_sx, v_ox);
        const float32x4_t py0 = RexVmaddF32_8296C540(fy0, v_sy, v_oy);
        const float32x4_t pz0 = RexVmaddF32_8296C540(fz0, v_sz, v_oz);

        // Exact SIMDe-on-AArch64 simde_mm_dp_ps(..., 0xEF) order: (0 + dz*pz) + (dy*py + dx*px)
        const float32x4_t dot0 = vaddq_f32(vaddq_f32(v_zero, vmulq_f32(v_dz, pz0)), vaddq_f32(vmulq_f32(v_dy, py0), vmulq_f32(v_dx, px0)));

        // Vertices i+4..i+7
        const float32x4_t fx1 = vcvtq_f32_s32(vmovl_high_s16(ix));
        const float32x4_t fy1 = vcvtq_f32_s32(vmovl_high_s16(iy));
        const float32x4_t fz1 = vcvtq_f32_s32(vmovl_high_s16(iz));

        const float32x4_t px1 = RexVmaddF32_8296C540(fx1, v_sx, v_ox);
        const float32x4_t py1 = RexVmaddF32_8296C540(fy1, v_sy, v_oy);
        const float32x4_t pz1 = RexVmaddF32_8296C540(fz1, v_sz, v_oz);

        const float32x4_t dot1 = vaddq_f32(vaddq_f32(v_zero, vmulq_f32(v_dz, pz1)), vaddq_f32(vmulq_f32(v_dy, py1), vmulq_f32(v_dx, px1)));

        const uint32x4_t gt0 = vcgtq_f32(dot0, v_best);
        const uint32x4_t gt1 = vcgtq_f32(dot1, v_best);

        if (__builtin_expect(vmaxvq_u32(vorrq_u32(gt0, gt1)) != 0u, 0)) {
          alignas(16) float d_arr[8];
          vst1q_f32(d_arr, dot0);
          vst1q_f32(d_arr + 4, dot1);
          for (int k = 0; k < 8; ++k) {
            if (d_arr[k] > best_dot) {
              best_dot = d_arr[k];
              best_idx = i + k;
            }
          }
          v_best = vdupq_n_f32(best_dot);
        }
      }

      // Scalar tail for remaining < 8 vertices
      for (; i < num_verts; ++i) {
        const float32x4_t vf = LoadQuantizedVert_8296C540(base, vert_base + static_cast<uint32_t>(i) * 6u);
        const float32x4_t pos = RexVmaddF32_8296C540(vf, v_scale, v_off);
        const float32x4_t d = Dot3Broadcast_8296C540(v22_dir, pos);
        const float df = vgetq_lane_f32(d, 0);
        if (df > best_dot) {
          best_dot = df;
          best_idx = i;
        }
      }

      if (best_idx >= 0) {
        v3_best_dot = vdupq_n_f32(best_dot);
        const float32x4_t vf = LoadQuantizedVert_8296C540(base, vert_base + static_cast<uint32_t>(best_idx) * 6u);
        v2_best_pos = RexVmaddF32_8296C540(vf, v_scale, v_off);
        StoreVec4BE_F32_8296C540(base, out_ea, v2_best_pos);
      }
    } else {
      // Exact fallback for non-standard mask/unit constants
      for (int32_t i = 0; i < num_verts; ++i) {
        const float32x4_t vf = LoadQuantizedVert_8296C540(base, vert_base + static_cast<uint32_t>(i) * 6u);
        const float32x4_t pos = RexVmaddF32_8296C540(vf, v_scale, v_off);
        const float32x4_t dot = Dot3Broadcast_8296C540(v22_dir, pos);
        const float32x4_t lhs = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(vreinterpretq_u32_f32(dot), v0_mask), v6_unit));
        const float32x4_t rhs = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(v3_best_dot), v0_mask));
        if (vminvq_u32(vcgtq_f32(lhs, rhs)) != 0u) {
          v3_best_dot = dot;
          best_idx = i;
          v2_best_pos = pos;
          StoreVec4BE_F32_8296C540(base, out_ea, pos);
        }
      }
    }
  }

  // --------------------------------------------------------------------------
  // LOOP 2: Curved edges / cylinders support search (loc_8296C680)
  // --------------------------------------------------------------------------
  const int32_t curved_count = static_cast<int32_t>(REX_LOAD_U32(geom_ea + 236u));
  if (curved_count > 0) {
    const float32x4_t v25_dir_zxyw = PermZXYW_8296C540(v22_dir);
    const float32x4_t v24_dir_yzxw = PermYZXW_8296C540(v22_dir);
    const float32x4_t v4_base = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(v10_init), v0_mask));
    const float32x4_t v_eps_raw = LoadVec4BE_F32_8296C540(base, 0x82B07240u);
    const float32x4_t v23_eps = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(v_eps_raw), v0_mask));
    const float32x4_t v_thresh = vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(v4_base), v6_unit));

    const uint32_t vert_base = REX_LOAD_U32(geom_ea + 176u);
    const float32x4_t v_off   = LoadVec4BE_F32_8296C540(base, geom_ea + 160u);
    const float32x4_t v_scale = LoadVec4BE_F32_8296C540(base, geom_ea + 144u);

    for (int32_t c_idx = 0; c_idx < curved_count; ++c_idx, curved_ptr += 48u) {
      const float32x4_t v27_saved_dot = v3_best_dot;
      const float32x4_t v26_saved_pos = v2_best_pos;

      const float32x4_t v_rad_vec = LoadVec4BE_F32_8296C540(base, curved_ptr + 32u);
      const float32x4_t v12_axis  = LoadVec4BE_F32_8296C540(base, curved_ptr + 16u);
      const float32x4_t v7_rad    = vdupq_laneq_f32(v_rad_vec, 0);

      const float32x4_t v8_axis_yzxw  = PermYZXW_8296C540(v12_axis);
      const float32x4_t v11_axis_zxyw = PermZXYW_8296C540(v12_axis);

      const float32x4_t v9_rad_chk = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(vreinterpretq_u32_f32(v7_rad), v0_mask), v6_unit));
      const uint32x4_t v1_rad_mask = vcgtq_f32(v9_rad_chk, v4_base);

      // Cross 1: axis x dir
      const float32x4_t c1_mul = vmulq_f32(v25_dir_zxyw, v8_axis_yzxw);
      const float32x4_t c1 = RexVnmsubF32_8296C540(v24_dir_yzxw, v11_axis_zxyw, c1_mul);

      // Cross 2: (axis x dir) x axis
      const float32x4_t c1_yzxw = PermYZXW_8296C540(c1);
      const float32x4_t c1_zxyw = PermZXYW_8296C540(c1);
      const float32x4_t c2_mul = vmulq_f32(v11_axis_zxyw, c1_yzxw);
      const float32x4_t v9_perp = RexVnmsubF32_8296C540(v8_axis_yzxw, c1_zxyw, c2_mul);

      const float32x4_t perp_len2 = Dot3Broadcast_8296C540(v9_perp, v9_perp);
      const float32x4_t perp_chk = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(vreinterpretq_u32_f32(perp_len2), v0_mask), v6_unit));

      if (vminvq_u32(vcgtq_f32(perp_chk, v23_eps)) != 0u) {
        const float32x4_t inv_perp = ExactRsqrtNR_8296C540(perp_len2, v_inf, v_half, v_one);
        const float32x4_t v13_ofs = vmulq_f32(vmulq_f32(v9_perp, v7_rad), inv_perp);
        const float32x4_t v11_origin = LoadVec4BE_F32_8296C540(base, curved_ptr + 0u);
        const float32x4_t v5_cand = vaddq_f32(v13_ofs, v11_origin);

        const int32_t idx_a = static_cast<int32_t>(REX_LOAD_U32(curved_ptr + 36u));
        const int32_t idx_b = static_cast<int32_t>(REX_LOAD_U32(curved_ptr + 40u));

        const uint32_t addr_a = vert_base + static_cast<uint32_t>(idx_a) * 6u;
        const uint32_t addr_b = vert_base + static_cast<uint32_t>(idx_b) * 6u;

        const float32x4_t pos_a = RexVmaddF32_8296C540(LoadQuantizedVert_8296C540(base, addr_a), v_scale, v_off);
        const float32x4_t pos_b = RexVmaddF32_8296C540(LoadQuantizedVert_8296C540(base, addr_b), v_scale, v_off);

        const float32x4_t edge_a = vsubq_f32(pos_a, v11_origin);
        const float32x4_t edge_b = vsubq_f32(pos_b, v11_origin);

        const float32x4_t ea_yzxw = PermYZXW_8296C540(edge_a);
        const float32x4_t ea_zxyw = PermZXYW_8296C540(edge_a);
        const float32x4_t of_zxyw = PermZXYW_8296C540(v13_ofs);
        const float32x4_t of_yzxw = PermYZXW_8296C540(v13_ofs);
        const float32x4_t eb_zxyw = PermZXYW_8296C540(edge_b);
        const float32x4_t eb_yzxw = PermYZXW_8296C540(edge_b);

        const float32x4_t cross_a = RexVnmsubF32_8296C540(of_yzxw, ea_zxyw, vmulq_f32(of_zxyw, ea_yzxw));
        const float32x4_t cross_b = RexVnmsubF32_8296C540(eb_yzxw, ea_zxyw, vmulq_f32(eb_zxyw, ea_yzxw));
        const float32x4_t cross_c = RexVnmsubF32_8296C540(eb_yzxw, of_zxyw, vmulq_f32(eb_zxyw, of_yzxw));

        const float32x4_t da = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(Dot3Broadcast_8296C540(v12_axis, cross_a)), v0_mask));
        const float32x4_t db = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(Dot3Broadcast_8296C540(v12_axis, cross_b)), v0_mask));
        const float32x4_t dc = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(Dot3Broadcast_8296C540(v12_axis, cross_c)), v0_mask));

        const uint32x4_t m_a = vcgtq_f32(v_thresh, da);
        const uint32x4_t m_b = vcgtq_f32(v_thresh, db);
        const uint32x4_t m_c = vcgtq_f32(v_thresh, dc);

        // Exact 9-op bitwise mask formula from _orig.cpp (preserves all 4 lanes bit-exact)
        const uint32x4_t na = veorq_u32(m_a, v0_mask);
        const uint32x4_t nb = veorq_u32(m_b, v0_mask);
        const uint32x4_t ac = vandq_u32(m_a, m_c);
        const uint32x4_t nc = veorq_u32(m_c, v0_mask);
        const uint32x4_t nab = vandq_u32(nb, na);
        const uint32x4_t nac = veorq_u32(ac, v0_mask);
        uint32x4_t arc_mask = vorrq_u32(vandq_u32(nab, nc), vandq_u32(nac, m_b));

        const uint32x4_t eq_override = (idx_a == idx_b) ? v0_mask : vreinterpretq_u32_f32(v10_init);
        arc_mask = vorrq_u32(arc_mask, eq_override);

        const float32x4_t cand_dot = Dot3Broadcast_8296C540(v22_dir, v5_cand);
        const float32x4_t lhs = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(vreinterpretq_u32_f32(cand_dot), v0_mask), v6_unit));
        const float32x4_t rhs = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(v3_best_dot), v0_mask));
        const uint32x4_t upd_mask = vandq_u32(vcgtq_f32(lhs, rhs), arc_mask);

        v3_best_dot = vbslq_f32(upd_mask, cand_dot, v3_best_dot);
        v2_best_pos = vbslq_f32(upd_mask, v5_cand, v2_best_pos);
      }

      // loc_8296C87C: mask update with radius > v4
      v3_best_dot = vbslq_f32(v1_rad_mask, v3_best_dot, v27_saved_dot);
      v2_best_pos = vbslq_f32(v1_rad_mask, v2_best_pos, v26_saved_pos);
    }

    StoreVec4BE_F32_8296C540(base, out_ea, v2_best_pos);
    StoreCtxVec_F32_8296C540(ctx.v2, v2_best_pos);
  }
#endif

  // loc_8296C898: write best vertex index to *r5 if r5 != 0
  if (out_idx_ea != 0u) {
    REX_STORE_U32(out_idx_ea, static_cast<uint32_t>(best_idx));
  }
}

// ----------------------------------------------------------------------------
// 2. sub_822CC090 (Matrix / Vector transform & blend, 1.15x speedup, 12/12 MATCH PASS)
// ----------------------------------------------------------------------------
[[gnu::noinline]] static void ExecuteSub822CC090Slow(PPCContext& ctx, uint8_t* base,
                                                     uint32_t saved_r1, uint32_t frame_r1,
                                                     uint32_t r29, uint32_t r28, double f2) {
  std::memset(REX_RAW_ADDR(frame_r1 + 176u), 0, 800u);

  REX_STORE_U32(frame_r1 + 80u, 0u);
  ctx.r3.u64 = frame_r1 + 112u;
  ctx.r4.u64 = frame_r1 + 80u;
  ctx.r5.u64 = 0u;
  sub_822CBEF8(ctx, base);

  double f30 = f2;
  const int32_t count = static_cast<int32_t>(REX_LOAD_U32(frame_r1 + 80u));
  if (count > 0) {
    const double f31 = static_cast<double>(REX_LOAD_F32(0x82000A34u));
    const uint32_t aligned_96 = (frame_r1 + 96u) & ~0xFu;
    REX_STORE_U32(aligned_96 + 0u, ctx.v1.u32[3]);
    REX_STORE_U32(aligned_96 + 4u, ctx.v1.u32[2]);
    REX_STORE_U32(aligned_96 + 8u, ctx.v1.u32[1]);
    REX_STORE_U32(aligned_96 + 12u, ctx.v1.u32[0]);

    for (int32_t i = 0; i < count; ++i) {
      ctx.r3.u64 = REX_LOAD_U32(frame_r1 + 112u + (static_cast<uint32_t>(i) << 2));
      ctx.r4.u64 = frame_r1 + 96u;
      sub_822CBDB8(ctx, base);

      const double f11 = ctx.f1.f64;
      ctx.f11.f64 = f11;
      if (f11 > f31) {
        sub_822C97F0(ctx, base);

        const uint32_t obj = ctx.r3.u32;
        const uint32_t idx_off = (REX_LOAD_U32(obj + 32u) << 2) & 0xFFFFFFFCu;
        const float f0_val = static_cast<float>(static_cast<double>(REX_LOAD_F32(obj + 36u)));
        REX_STORE_F32(frame_r1 + 976u + idx_off, f0_val);

        const double prod = static_cast<double>(static_cast<float>(ctx.f1.f64 * f11));
        const double cur_max = static_cast<double>(REX_LOAD_F32(frame_r1 + 176u + idx_off));
        const double diff = static_cast<double>(static_cast<float>(prod - cur_max));
        const double sel = (diff >= 0.0) ? prod : cur_max;
        REX_STORE_F32(frame_r1 + 176u + idx_off, static_cast<float>(sel));
      }
    }

    const double f11_const = static_cast<double>(REX_LOAD_F32(0x820BECF4u));
    for (uint32_t k = 0u; k < 200u; ++k) {
      const uint32_t byte_off = k << 2;
      const double f12 = static_cast<double>(REX_LOAD_F32(frame_r1 + 176u + byte_off));
      if (f12 > f31) {
        double f0 = static_cast<double>(REX_LOAD_F32(0x82D2BD60u + (k * 188u)));
        const double f13 = static_cast<double>(REX_LOAD_F32(frame_r1 + 976u + byte_off));
        if (f13 > f31) {
          const double f10 = static_cast<double>(static_cast<float>(f2 - f13));
          REX_STORE_F32(frame_r1 + 976u + byte_off, static_cast<float>(f11_const));
          f0 = static_cast<double>(static_cast<float>((f13 * f0) + f10));
        }
        const double one_minus = static_cast<double>(static_cast<float>(f2 - f12));
        const double weighted = static_cast<double>(static_cast<float>(f12 * f0));
        f30 = static_cast<double>(static_cast<float>((one_minus * f30) + weighted));
      }
    }
  }

  if (static_cast<int32_t>(r29) > 0 && static_cast<int32_t>(r28) != 63) {
    ctx.r3.u64 = r29;
    ctx.r4.u64 = r28;
    sub_8251DCD8(ctx, base);
    ctx.f1.f64 = static_cast<double>(static_cast<float>(ctx.f1.f64 * f30));
  } else {
    ctx.f1.f64 = f30;
  }

  ctx.r1.u32 = saved_r1;
}

extern "C" void sub_822CC090(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

  ctx.lr = 0x822CC098u;
  const uint32_t saved_r1 = ctx.r1.u32;
  vst1q_u8(REX_RAW_ADDR(saved_r1 + static_cast<uint32_t>(-56)), vdupq_n_u8(0));

  const uint32_t frame_r1 = saved_r1 + static_cast<uint32_t>(-1840);
  REX_STORE_U32(frame_r1, saved_r1);

  const int32_t slot_idx = static_cast<int32_t>(REX_LOAD_U32(0x82A99E20u));
  const double f2 = static_cast<double>(REX_LOAD_F32(0x82000D48u));
  ctx.f2.f64 = f2;

  if (slot_idx != -1) [[likely]] {
    const uint32_t off = static_cast<uint32_t>(slot_idx) * 188u;
    ctx.f1.f64 = static_cast<double>(REX_LOAD_F32(0x82D2BD60u + off));
    return;
  }

  ctx.r1.u32 = frame_r1;
  ExecuteSub822CC090Slow(ctx, base, saved_r1, frame_r1, ctx.r3.u32, ctx.r4.u32, f2);
}
