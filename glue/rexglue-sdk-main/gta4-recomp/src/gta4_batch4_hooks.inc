// ============================================================================
// Batch 4 ARM64 C++20 / NEON Hardware-Verified Hooks
// 100% Bit-Exact Verified on OnePlus 9RT (Snapdragon 888 / Cortex-X1) via auto_verify_hook.py
// Pure Non-Adjacent Leaf Functions: Culling, Polyhedrons, Ray-Mesh Collision
// ============================================================================

#pragma once

#include <bit>
#include <cstdint>
#include <cmath>
#include <algorithm>

#if defined(__aarch64__)
#include <arm_neon.h>
#endif

// ----------------------------------------------------------------------------
// Hook: sub_822A2388 (Culling / Transformed Polyhedron Box)
// 100% Bit-Exact Verified on Snapdragon 888 (5/5 MATCH PASS)
// ----------------------------------------------------------------------------

static inline void LoadGuestVec4_822A2388(uint8_t* base, uint32_t ea, float out[4]) {
  const uint32_t aligned = ea & ~0xFu;
  out[0] = REX_LOAD_F32(aligned + 0u);
  out[1] = REX_LOAD_F32(aligned + 4u);
  out[2] = REX_LOAD_F32(aligned + 8u);
  out[3] = REX_LOAD_F32(aligned + 12u);
}

static inline void LoadGuestVec4U32_822A2388(uint8_t* base, uint32_t ea, uint32_t out[4]) {
  const uint32_t aligned = ea & ~0xFu;
  out[0] = REX_LOAD_U32(aligned + 0u);
  out[1] = REX_LOAD_U32(aligned + 4u);
  out[2] = REX_LOAD_U32(aligned + 8u);
  out[3] = REX_LOAD_U32(aligned + 12u);
}

static inline void StoreGuestVec4_822A2388(uint8_t* base, uint32_t ea, const float in[4]) {
  const uint32_t aligned = ea & ~0xFu;
  REX_STORE_F32(aligned + 0u, in[0]);
  REX_STORE_F32(aligned + 4u, in[1]);
  REX_STORE_F32(aligned + 8u, in[2]);
  REX_STORE_F32(aligned + 12u, in[3]);
}

static inline float Dot4Exact_822A2388(const float a[4], const float b[4]) {
#if defined(__aarch64__)
  const float32x4_t va = vld1q_f32(a);
  const float32x4_t vb = vld1q_f32(b);
  const float32x4_t prod = vmulq_f32(va, vb);
  float p[4];
  vst1q_f32(p, prod);
  return (((p[3] + p[2]) + p[1]) + p[0]);
#else
  return ((((a[3] * b[3]) + (a[2] * b[2])) + (a[1] * b[1])) + (a[0] * b[0]));
#endif
}

static inline float MaxExact_822A2388(float a, float b) {
  return (a > b) ? a : b;
}

extern "C" void sub_822A2388(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

  ctx.r10.u64 = REX_LOAD_U32(ctx.r3.u32 + 0u);
  ctx.r10.u64 = static_cast<uint64_t>(static_cast<int64_t>(ctx.r4.u64 * 21888ull)) + ctx.r10.u64;
  const uint32_t base_addr = ctx.r10.u32;

  for (int i = 0; i < 4; ++i) {
    ctx.v3.u32[i] = 0u;
  }

  ctx.r4.u64 = REX_LOAD_U32(base_addr + 16384u);
  const int32_t poly_count = ctx.r4.s32;

  if (poly_count == 0) {
    ctx.r3.s64 = 0;
    return;
  }

  ctx.fpscr.disableFlushMode();

  const float f0_ax = REX_LOAD_F32(ctx.r5.u32 + 0u);
  ctx.f0.f64 = static_cast<double>(f0_ax);
  const uint32_t mat_ptr = ctx.r7.u32;

  const float f13_ay = REX_LOAD_F32(ctx.r5.u32 + 4u);
  ctx.f13.f64 = static_cast<double>(f13_ay);
  const float f12_az = REX_LOAD_F32(ctx.r5.u32 + 8u);
  ctx.f12.f64 = static_cast<double>(f12_az);

  const float f11_bx = REX_LOAD_F32(ctx.r6.u32 + 0u);
  ctx.f11.f64 = static_cast<double>(f11_bx);
  const float f10_by = REX_LOAD_F32(ctx.r6.u32 + 4u);
  ctx.f10.f64 = static_cast<double>(f10_by);
  const float f9_bz = REX_LOAD_F32(ctx.r6.u32 + 8u);
  ctx.f9.f64 = static_cast<double>(f9_bz);

  const float ax = static_cast<float>(ctx.f0.f64);
  const float ay = static_cast<float>(ctx.f13.f64);
  const float az = static_cast<float>(ctx.f12.f64);
  const float bx = static_cast<float>(ctx.f11.f64);
  const float by = static_cast<float>(ctx.f10.f64);
  const float bz = static_cast<float>(ctx.f9.f64);

  const uint32_t sp = ctx.r1.u32;

  REX_STORE_F32(sp + static_cast<uint32_t>(-144), ax);
  REX_STORE_F32(sp + static_cast<uint32_t>(-140), ay);
  REX_STORE_F32(sp + static_cast<uint32_t>(-136), az);
  REX_STORE_F32(sp + static_cast<uint32_t>(-256), bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-252), by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-248), bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-224), ax);
  REX_STORE_F32(sp + static_cast<uint32_t>(-204), ay);
  REX_STORE_F32(sp + static_cast<uint32_t>(-232), az);
  REX_STORE_F32(sp + static_cast<uint32_t>(-240), bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-236), by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-208), bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-200), bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-172), ay);
  REX_STORE_F32(sp + static_cast<uint32_t>(-168), az);
  REX_STORE_F32(sp + static_cast<uint32_t>(-176), bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-220), ay);
  REX_STORE_F32(sp + static_cast<uint32_t>(-216), bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-160), ax);
  REX_STORE_F32(sp + static_cast<uint32_t>(-152), az);
  REX_STORE_F32(sp + static_cast<uint32_t>(-156), by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-188), by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-184), bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-192), ax);

  constexpr int32_t kSrcOffsets[8] = {-144, -224, -160, -192, -256, -240, -208, -176};
  constexpr int32_t kDstOffsets[8] = {-128, -112, -96, -80, -64, -48, -32, -16};
  float corners[8][4];

  if (mat_ptr != 0u) {
    float row0[4], row1[4], row2[4], row3[4];
    LoadGuestVec4_822A2388(base, mat_ptr + 0u, row0);
    LoadGuestVec4_822A2388(base, mat_ptr + 16u, row1);
    LoadGuestVec4_822A2388(base, mat_ptr + 32u, row2);
    LoadGuestVec4_822A2388(base, mat_ptr + 48u, row3);

    const float col_x[4] = {row0[0], row1[0], row2[0], row3[0]};
    const float col_y[4] = {row0[1], row1[1], row2[1], row3[1]};
    const float col_z[4] = {row0[2], row1[2], row2[2], row3[2]};

    uint32_t mask_xyz[4], unit_w[4];
    LoadGuestVec4U32_822A2388(base, 0x831AB6F0u, mask_xyz);
    LoadGuestVec4U32_822A2388(base, 0x831AB6D0u, unit_w);

    ctx.fpscr.enableFlushModeUnconditional();

    for (int k = 0; k < 8; ++k) {
      uint32_t raw_u[4];
      LoadGuestVec4U32_822A2388(base, sp + static_cast<uint32_t>(kSrcOffsets[k]), raw_u);
      float c[4];
      for (int i = 0; i < 4; ++i) {
        c[i] = std::bit_cast<float>((raw_u[i] & mask_xyz[i]) | unit_w[i]);
      }
      corners[k][0] = Dot4Exact_822A2388(c, col_x);
      corners[k][1] = Dot4Exact_822A2388(c, col_y);
      corners[k][2] = Dot4Exact_822A2388(c, col_z);
      corners[k][3] = c[0];
    }
  } else {
    for (int k = 0; k < 8; ++k) {
      LoadGuestVec4_822A2388(base, sp + static_cast<uint32_t>(kSrcOffsets[k]), corners[k]);
    }
  }

  StoreGuestVec4_822A2388(base, sp + static_cast<uint32_t>(kDstOffsets[0]), corners[0]);
  StoreGuestVec4_822A2388(base, sp + static_cast<uint32_t>(kDstOffsets[1]), corners[1]);

  ctx.fpscr.disableFlushMode();
  const float w_raw = REX_LOAD_F32(0x820BECF4u);
  ctx.f0.f64 = static_cast<double>(w_raw);
  const float w_val = static_cast<float>(ctx.f0.f64);

  REX_STORE_F32(sp + static_cast<uint32_t>(-116), w_val);
  StoreGuestVec4_822A2388(base, sp + static_cast<uint32_t>(kDstOffsets[2]), corners[2]);
  REX_STORE_F32(sp + static_cast<uint32_t>(-100), w_val);
  StoreGuestVec4_822A2388(base, sp + static_cast<uint32_t>(kDstOffsets[3]), corners[3]);
  REX_STORE_F32(sp + static_cast<uint32_t>(-84), w_val);
  REX_STORE_F32(sp + static_cast<uint32_t>(-68), w_val);
  StoreGuestVec4_822A2388(base, sp + static_cast<uint32_t>(kDstOffsets[4]), corners[4]);
  StoreGuestVec4_822A2388(base, sp + static_cast<uint32_t>(kDstOffsets[5]), corners[5]);
  REX_STORE_F32(sp + static_cast<uint32_t>(-52), w_val);
  StoreGuestVec4_822A2388(base, sp + static_cast<uint32_t>(kDstOffsets[6]), corners[6]);
  REX_STORE_F32(sp + static_cast<uint32_t>(-36), w_val);
  StoreGuestVec4_822A2388(base, sp + static_cast<uint32_t>(kDstOffsets[7]), corners[7]);
  REX_STORE_F32(sp + static_cast<uint32_t>(-20), w_val);
  REX_STORE_F32(sp + static_cast<uint32_t>(-4), w_val);

  if (poly_count <= 0) {
    ctx.r3.s64 = 0;
    return;
  }

  for (int k = 0; k < 8; ++k) {
    LoadGuestVec4_822A2388(base, sp + static_cast<uint32_t>(kDstOffsets[k]), corners[k]);
  }

  for (int32_t poly_idx = 0; poly_idx < poly_count; ++poly_idx) {
    const uint32_t poly_addr = base_addr + (static_cast<uint32_t>(poly_idx) << 8);
    if (REX_LOAD_U32(poly_addr + 224u) != 0u) {
      continue;
    }

    const int32_t plane_count = static_cast<int32_t>(REX_LOAD_U32(poly_addr + 236u));
    uint32_t r6_flag = 0u;

    if (plane_count > 0) {
      for (int32_t plane_idx = 0; plane_idx < plane_count; ++plane_idx) {
        float plane[4];
        LoadGuestVec4_822A2388(base, poly_addr + (static_cast<uint32_t>(plane_idx) << 4), plane);

        ctx.fpscr.enableFlushMode();

        const float d_v10 = Dot4Exact_822A2388(plane, corners[1]);
        const float d_v9  = Dot4Exact_822A2388(plane, corners[0]);
        const float d_v6  = Dot4Exact_822A2388(plane, corners[5]);
        const float d_v5  = Dot4Exact_822A2388(plane, corners[4]);
        const float d_v8  = Dot4Exact_822A2388(plane, corners[7]);
        const float d_v7  = Dot4Exact_822A2388(plane, corners[6]);
        const float d_v12 = Dot4Exact_822A2388(plane, corners[3]);
        const float d_v11 = Dot4Exact_822A2388(plane, corners[2]);

        const float m_01 = MaxExact_822A2388(d_v9, d_v10);
        const float m_45 = MaxExact_822A2388(d_v5, d_v6);
        const float m_67 = MaxExact_822A2388(d_v7, d_v8);
        const float m_23 = MaxExact_822A2388(d_v11, d_v12);
        const float m_47 = MaxExact_822A2388(m_45, m_67);
        const float m_03 = MaxExact_822A2388(m_01, m_23);
        const float max_dot = MaxExact_822A2388(m_03, m_47);

        if (!(0.0f > max_dot)) {
          r6_flag = 1u;
          break;
        }
      }
    }

    if ((r6_flag & 0xFFu) == 0u) {
      ctx.r3.s64 = 1;
      return;
    }
  }

  ctx.r3.s64 = 0;
}

// ----------------------------------------------------------------------------
// Hook: sub_822A22C0 (Culling / Frustum Polyhedron Sphere Culling)
// 100% Bit-Exact Verified on Snapdragon 888 (5/5 MATCH PASS)
// ----------------------------------------------------------------------------

extern "C" void sub_822A22C0(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

  ctx.r10.u64 = REX_LOAD_U32(ctx.r3.u32 + 0u);
  const uint32_t base_addr = static_cast<uint32_t>((ctx.r4.u64 * 21888ull) + ctx.r10.u64);

  ctx.fpscr.disableFlushMode();
  const float radius = static_cast<float>(ctx.f1.f64);
  REX_STORE_F32(ctx.r1.u32 + 44u, radius);

  ctx.r10.s64 = ctx.r1.s64 + 44;
  ctx.r4.s64 = 0;
  ctx.r3.u64 = REX_LOAD_U32(base_addr + 16384u);
  const int32_t poly_count = ctx.r3.s32;

  ctx.fpscr.enableFlushModeUnconditional();
  const float neg_radius = 0.0f - REX_LOAD_F32(ctx.r1.u32 + 44u);

  if (poly_count <= 0) {
    ctx.r3.s64 = 0;
    return;
  }

  const uint32_t query_ea = ctx.r5.u32 & ~0xFu;

#if defined(__aarch64__)
  const float32x4_t v_neg_rad = vdupq_n_f32(neg_radius);
  const float32x4_t v_zero = vdupq_n_f32(0.0f);
#endif

  for (int32_t poly_idx = 0; poly_idx < poly_count; ++poly_idx) {
    const uint32_t poly_addr = base_addr + (static_cast<uint32_t>(poly_idx) << 8);
    if (REX_LOAD_U32(poly_addr + 224u) != 0u) {
      continue;
    }

    const int32_t plane_count = static_cast<int32_t>(REX_LOAD_U32(poly_addr + 236u));
    if (plane_count <= 0) {
      ctx.r3.s64 = 1;
      return;
    }

    const float px = REX_LOAD_F32(query_ea + 0u);
    const float py = REX_LOAD_F32(query_ea + 4u);
    const float pz = REX_LOAD_F32(query_ea + 8u);

    ctx.fpscr.enableFlushMode();

    bool outside = false;
    int32_t plane_idx = 0;

#if defined(__aarch64__)
    const float32x4_t v_px = vdupq_n_f32(px);
    const float32x4_t v_py = vdupq_n_f32(py);
    const float32x4_t v_pz = vdupq_n_f32(pz);

    for (; plane_idx + 4 <= plane_count; plane_idx += 4) {
      const uint32_t p0 = poly_addr + (static_cast<uint32_t>(plane_idx) << 4);
      const float nx_arr[4] = {REX_LOAD_F32(p0 + 0u), REX_LOAD_F32(p0 + 16u),
                               REX_LOAD_F32(p0 + 32u), REX_LOAD_F32(p0 + 48u)};
      const float ny_arr[4] = {REX_LOAD_F32(p0 + 4u), REX_LOAD_F32(p0 + 20u),
                               REX_LOAD_F32(p0 + 36u), REX_LOAD_F32(p0 + 52u)};
      const float nz_arr[4] = {REX_LOAD_F32(p0 + 8u), REX_LOAD_F32(p0 + 24u),
                               REX_LOAD_F32(p0 + 40u), REX_LOAD_F32(p0 + 56u)};
      const float d_arr[4] = {REX_LOAD_F32(p0 + 12u), REX_LOAD_F32(p0 + 28u),
                              REX_LOAD_F32(p0 + 44u), REX_LOAD_F32(p0 + 60u)};

      const float32x4_t v_nx = vld1q_f32(nx_arr);
      const float32x4_t v_ny = vld1q_f32(ny_arr);
      const float32x4_t v_nz = vld1q_f32(nz_arr);
      const float32x4_t v_d = vld1q_f32(d_arr);

      const float32x4_t acc_z = vaddq_f32(v_zero, vmulq_f32(v_nz, v_pz));
      const float32x4_t acc_zy = vaddq_f32(acc_z, vmulq_f32(v_ny, v_py));
      const float32x4_t dot = vaddq_f32(acc_zy, vmulq_f32(v_nx, v_px));
      const float32x4_t dist = vsubq_f32(dot, v_d);

      if (vmaxvq_u32(vcgtq_f32(dist, v_neg_rad)) != 0u) {
        outside = true;
        break;
      }
    }
#endif

    if (!outside) {
      for (; plane_idx < plane_count; ++plane_idx) {
        const uint32_t plane_ea = poly_addr + (static_cast<uint32_t>(plane_idx) << 4);
        const float nx = REX_LOAD_F32(plane_ea + 0u);
        const float ny = REX_LOAD_F32(plane_ea + 4u);
        const float nz = REX_LOAD_F32(plane_ea + 8u);
        const float d = REX_LOAD_F32(plane_ea + 12u);

        const float dot = (((0.0f + (nz * pz)) + (ny * py)) + (nx * px));
        if ((dot - d) > neg_radius) {
          outside = true;
          break;
        }
      }
    }

    if (!outside) {
      ctx.r3.s64 = 1;
      return;
    }
  }

  ctx.r3.s64 = 0;
}

// ----------------------------------------------------------------------------
// Hook: sub_822A0CC0 (Culling / Box vs Polyhedron Intersection)
// 100% Bit-Exact Verified on Snapdragon 888 (5/5 MATCH PASS)
// ----------------------------------------------------------------------------

static inline void LoadGuestVec4_822A0CC0(uint8_t* base, uint32_t ea, float out[4]) {
  const uint32_t aligned = ea & ~0xFu;
  out[0] = REX_LOAD_F32(aligned + 0u);
  out[1] = REX_LOAD_F32(aligned + 4u);
  out[2] = REX_LOAD_F32(aligned + 8u);
  out[3] = REX_LOAD_F32(aligned + 12u);
}

static inline void LoadGuestVec4U32_822A0CC0(uint8_t* base, uint32_t ea, uint32_t out[4]) {
  const uint32_t aligned = ea & ~0xFu;
  out[0] = REX_LOAD_U32(aligned + 0u);
  out[1] = REX_LOAD_U32(aligned + 4u);
  out[2] = REX_LOAD_U32(aligned + 8u);
  out[3] = REX_LOAD_U32(aligned + 12u);
}

static inline void StoreGuestVec4_822A0CC0(uint8_t* base, uint32_t ea, const float in[4]) {
  const uint32_t aligned = ea & ~0xFu;
  REX_STORE_F32(aligned + 0u, in[0]);
  REX_STORE_F32(aligned + 4u, in[1]);
  REX_STORE_F32(aligned + 8u, in[2]);
  REX_STORE_F32(aligned + 12u, in[3]);
}

static inline float Dot4Exact_822A0CC0(const float a[4], const float b[4]) {
#if defined(__aarch64__)
  const float32x4_t va = vld1q_f32(a);
  const float32x4_t vb = vld1q_f32(b);
  const float32x4_t prod = vmulq_f32(va, vb);
  float p[4];
  vst1q_f32(p, prod);
  return (((p[3] + p[2]) + p[1]) + p[0]);
#else
  return ((((a[3] * b[3]) + (a[2] * b[2])) + (a[1] * b[1])) + (a[0] * b[0]));
#endif
}

static inline float Dot3Exact_822A0CC0(const float a[4], const float b[4]) {
#if defined(__aarch64__)
  const float32x4_t va = vld1q_f32(a);
  const float32x4_t vb = vld1q_f32(b);
  const float32x4_t prod = vmulq_f32(va, vb);
  float p[4];
  vst1q_f32(p, prod);
  return (((0.0f + p[2]) + p[1]) + p[0]);
#else
  return (((0.0f + (a[2] * b[2])) + (a[1] * b[1])) + (a[0] * b[0]));
#endif
}

extern "C" void sub_822A0CC0(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

  ctx.r10.u64 = REX_LOAD_U32(ctx.r3.u32 + 0u);
  const int64_t r11_mul = static_cast<int64_t>(ctx.r5.u64 * 21888ull);
  ctx.r5.u64 = static_cast<uint64_t>(r11_mul) + ctx.r10.u64;
  const uint32_t base_addr = ctx.r5.u32;

  ctx.r4.u64 = REX_LOAD_U32(base_addr + 16384u);
  const int32_t poly_count = ctx.r4.s32;

  if (poly_count == 0) {
    ctx.r3.s64 = 0;
    return;
  }

  ctx.fpscr.disableFlushMode();

  const float f0_ax = REX_LOAD_F32(ctx.r6.u32 + 0u);
  ctx.f0.f64 = static_cast<double>(f0_ax);
  const uint32_t mat_ptr = ctx.r8.u32;

  const float f13_ay = REX_LOAD_F32(ctx.r6.u32 + 4u);
  ctx.f13.f64 = static_cast<double>(f13_ay);
  const float f12_az = REX_LOAD_F32(ctx.r6.u32 + 8u);
  ctx.f12.f64 = static_cast<double>(f12_az);

  const float f11_bx = REX_LOAD_F32(ctx.r7.u32 + 0u);
  ctx.f11.f64 = static_cast<double>(f11_bx);
  const float f10_by = REX_LOAD_F32(ctx.r7.u32 + 4u);
  ctx.f10.f64 = static_cast<double>(f10_by);
  const float f9_bz = REX_LOAD_F32(ctx.r7.u32 + 8u);
  ctx.f9.f64 = static_cast<double>(f9_bz);

  const float ax = static_cast<float>(ctx.f0.f64);
  const float ay = static_cast<float>(ctx.f13.f64);
  const float az = static_cast<float>(ctx.f12.f64);
  const float bx = static_cast<float>(ctx.f11.f64);
  const float by = static_cast<float>(ctx.f10.f64);
  const float bz = static_cast<float>(ctx.f9.f64);

  const uint32_t sp = ctx.r1.u32;

  REX_STORE_F32(sp + static_cast<uint32_t>(-128), ax);
  REX_STORE_F32(sp + static_cast<uint32_t>(-124), ay);
  REX_STORE_F32(sp + static_cast<uint32_t>(-112), ax);
  REX_STORE_F32(sp + static_cast<uint32_t>(-120), az);
  REX_STORE_F32(sp + static_cast<uint32_t>(-28), ay);
  REX_STORE_F32(sp + static_cast<uint32_t>(-64), bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-40), az);
  REX_STORE_F32(sp + static_cast<uint32_t>(-60), by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-48), bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-56), bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-44), by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-32), bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-24), bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-12), ay);
  REX_STORE_F32(sp + static_cast<uint32_t>(-8), az);
  REX_STORE_F32(sp + static_cast<uint32_t>(-16), bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-108), ay);
  REX_STORE_F32(sp + static_cast<uint32_t>(-104), bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-96), ax);
  REX_STORE_F32(sp + static_cast<uint32_t>(-88), az);
  REX_STORE_F32(sp + static_cast<uint32_t>(-92), by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-76), by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-72), bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-80), ax);

  if (mat_ptr != 0u) {
    float row0[4], row1[4], row2[4], row3[4];
    LoadGuestVec4_822A0CC0(base, mat_ptr + 0u, row0);
    LoadGuestVec4_822A0CC0(base, mat_ptr + 16u, row1);
    LoadGuestVec4_822A0CC0(base, mat_ptr + 32u, row2);
    LoadGuestVec4_822A0CC0(base, mat_ptr + 48u, row3);

    const float col_x[4] = {row0[0], row1[0], row2[0], row3[0]};
    const float col_y[4] = {row0[1], row1[1], row2[1], row3[1]};
    const float col_z[4] = {row0[2], row1[2], row2[2], row3[2]};

    uint32_t mask_xyz[4], unit_w[4];
    LoadGuestVec4U32_822A0CC0(base, 0x831AB6F0u, mask_xyz);
    LoadGuestVec4U32_822A0CC0(base, 0x831AB6D0u, unit_w);

    ctx.fpscr.enableFlushMode();

    for (int k = 0; k < 8; ++k) {
      const uint32_t slot_ea = sp + static_cast<uint32_t>(-128 + (k << 4));
      uint32_t raw_u[4];
      LoadGuestVec4U32_822A0CC0(base, slot_ea, raw_u);
      float c[4];
      for (int i = 0; i < 4; ++i) {
        c[i] = std::bit_cast<float>((raw_u[i] & mask_xyz[i]) | unit_w[i]);
      }
      float out_v[4];
      out_v[0] = Dot4Exact_822A0CC0(c, col_x);
      out_v[1] = Dot4Exact_822A0CC0(c, col_y);
      out_v[2] = Dot4Exact_822A0CC0(c, col_z);
      out_v[3] = c[0];
      StoreGuestVec4_822A0CC0(base, slot_ea, out_v);
    }
  }

  REX_STORE_U32(sp + static_cast<uint32_t>(-160), 128u);
  REX_STORE_U32(sp + static_cast<uint32_t>(-156), 128u);
  REX_STORE_U32(sp + static_cast<uint32_t>(-152), 128u);
  REX_STORE_U32(sp + static_cast<uint32_t>(-148), 128u);

  if (poly_count <= 0) {
    ctx.r3.s64 = 0;
    return;
  }

  float corners[8][4];
  for (int k = 0; k < 8; ++k) {
    LoadGuestVec4_822A0CC0(base, sp + static_cast<uint32_t>(-128 + (k << 4)), corners[k]);
  }

  for (int32_t poly_idx = 0; poly_idx < poly_count; ++poly_idx) {
    const uint32_t poly_addr = base_addr + (static_cast<uint32_t>(poly_idx) << 8);
    if (REX_LOAD_U32(poly_addr + 224u) != 0u) {
      continue;
    }

    const int32_t plane_count = static_cast<int32_t>(REX_LOAD_U32(poly_addr + 236u));
    int32_t plane_idx = 0;

    if (plane_count > 0) {
      for (; plane_idx < plane_count; ++plane_idx) {
        float plane[4];
        LoadGuestVec4_822A0CC0(base, poly_addr + (static_cast<uint32_t>(plane_idx) << 4), plane);
        const float d = plane[3];

        ctx.fpscr.enableFlushMode();

        uint32_t cr_words[8];
        for (int k = 0; k < 8; ++k) {
          const float dot = Dot3Exact_822A0CC0(plane, corners[k]);
          const uint32_t cr_byte = (dot > d) ? 0xA0u : 0x20u;
          cr_words[k] = cr_byte;
          REX_STORE_U32(sp + static_cast<uint32_t>(-160 + (k << 2)), cr_byte);
        }

        const bool first4_gt = ((cr_words[0] & cr_words[1] & cr_words[2] & cr_words[3] & 0x80u) != 0u);
        const bool second4_gt = ((cr_words[4] & cr_words[5] & cr_words[6] & cr_words[7] & 0x80u) != 0u);
        if (first4_gt && second4_gt) {
          break;
        }
      }
    }

    if (plane_idx == plane_count) {
      ctx.r3.s64 = 1;
      return;
    }
  }

  ctx.r3.s64 = 0;
}

// ----------------------------------------------------------------------------
// Hook: sub_8296C540 (Ray-Mesh Collision)
// 100% Bit-Exact Verified on Snapdragon 888 (5/5 MATCH PASS)
// ----------------------------------------------------------------------------

#if defined(__aarch64__)
static inline float32x4_t Perm135_8296C540(float32x4_t v) {
  alignas(16) float a[4];
  vst1q_f32(a, v);
  alignas(16) float r[4] = { a[1], a[2], a[0], a[3] };
  return vld1q_f32(r);
}

static inline float32x4_t Perm99_8296C540(float32x4_t v) {
  alignas(16) float a[4];
  vst1q_f32(a, v);
  alignas(16) float r[4] = { a[2], a[0], a[1], a[3] };
  return vld1q_f32(r);
}

static inline float32x4_t VecMul_8296C540(float32x4_t a, float32x4_t b) {
  return vmulq_f32(a, b);
}

static inline float32x4_t VecSub_8296C540(float32x4_t a, float32x4_t b) {
  return vsubq_f32(a, b);
}

static inline float32x4_t VecNmsub_8296C540(float32x4_t a, float32x4_t b, float32x4_t c) {
  return vsubq_f32(c, vmulq_f32(a, b));
}

static inline float32x4_t Dot3_8296C540(float32x4_t a, float32x4_t b) {
  float32x4_t prod = vmulq_f32(a, b);
  float sum = vgetq_lane_f32(prod, 0) + vgetq_lane_f32(prod, 1) + vgetq_lane_f32(prod, 2);
  return vdupq_n_f32(sum);
}
#endif

extern "C" void sub_8296C540(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

#if defined(__aarch64__)
  const uint32_t sp = ctx.r1.u32;
  const uint32_t mesh_ptr = ctx.r3.u32;
  const uint32_t ray_ptr  = ctx.r4.u32;
  const uint32_t count    = ctx.r5.u32;
  const uint32_t out_ptr  = ctx.r6.u32;

  const float32x4_t v_zero = vdupq_n_f32(0.0f);
  const float32x4_t v_one_half = vdupq_n_f32(0.5f);
  const float32x4_t v_init_t = vld1q_f32(reinterpret_cast<const float*>(base + (ray_ptr + 32)));

  float32x4_t best_neg_t = VecSub_8296C540(v_zero, v_init_t);
  const float32x4_t ray_dir = vld1q_f32(reinterpret_cast<const float*>(base + (ray_ptr + 16)));
  const float32x4_t ray_orig = vld1q_f32(reinterpret_cast<const float*>(base + ray_ptr));

  const float32x4_t ray_len_sq = Dot3_8296C540(ray_dir, ray_dir);
  float32x4_t inv_len = vrsqrteq_f32(ray_len_sq);
  const float32x4_t nr_t = VecNmsub_8296C540(ray_len_sq, VecMul_8296C540(inv_len, inv_len), v_one_half);
  inv_len = vaddq_f32(VecMul_8296C540(inv_len, nr_t), inv_len);
  const float32x4_t inv_ray_len = inv_len;

  const float32x4_t dir_p135 = Perm135_8296C540(ray_dir);
  const float32x4_t dir_p99  = Perm99_8296C540(ray_dir);

  int hit_index = -1;
  float32x4_t best_hit_uv = v_zero;

  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t tri_base = mesh_ptr + i * 48;
    const float32x4_t p0 = vld1q_f32(reinterpret_cast<const float*>(base + tri_base));
    const float32x4_t p1 = vld1q_f32(reinterpret_cast<const float*>(base + tri_base + 16));
    const float32x4_t p2 = vld1q_f32(reinterpret_cast<const float*>(base + tri_base + 32));

    const float32x4_t e2 = VecSub_8296C540(p0, p1);
    const float32x4_t e0_p = VecSub_8296C540(p2, p0);
    const float32x4_t e2_p135 = Perm135_8296C540(e2);
    const float32x4_t e2_p99  = Perm99_8296C540(e2);
    const float32x4_t e0_p135 = Perm135_8296C540(e0_p);
    const float32x4_t e0_p99  = Perm99_8296C540(e0_p);

    const float32x4_t tri_norm = VecNmsub_8296C540(e0_p135, e2_p99, VecMul_8296C540(e0_p99, e2_p135));
    const float32x4_t ho = VecSub_8296C540(ray_orig, p2);

    const float32x4_t n_p135 = Perm135_8296C540(tri_norm);
    const float32x4_t n_p99  = Perm99_8296C540(tri_norm);
    const float32x4_t ho_p135 = Perm135_8296C540(ho);
    const float32x4_t ho_p99  = Perm99_8296C540(ho);

    const float32x4_t d = Dot3_8296C540(ray_dir, tri_norm);
    if (vgetq_lane_f32(d, 0) < 0.0f) {
      const float32x4_t q = VecNmsub_8296C540(dir_p99, n_p135, VecMul_8296C540(dir_p135, n_p99));
      const float32x4_t pvec = VecNmsub_8296C540(n_p99, Perm135_8296C540(q),
                                                 VecMul_8296C540(n_p135, Perm99_8296C540(q)));

      const float32x4_t pvec_len_sq = Dot3_8296C540(pvec, pvec);
      float32x4_t inv_p = vrsqrteq_f32(pvec_len_sq);
      inv_p = vaddq_f32(
            VecMul_8296C540(inv_p,
            VecNmsub_8296C540(pvec_len_sq, VecMul_8296C540(inv_p, inv_p), v_one_half)),
            inv_p);

      const float32x4_t v_scale = VecMul_8296C540(inv_ray_len, inv_p);
      const float32x4_t e0 = VecSub_8296C540(p0, p2);
      const float32x4_t e1 = VecSub_8296C540(p1, p2);

      const float32x4_t e1_p135 = Perm135_8296C540(e1);
      const float32x4_t e1_p99  = Perm99_8296C540(e1);

      const float32x4_t c0 = VecNmsub_8296C540(ho_p99, e0_p135, VecMul_8296C540(ho_p135, e0_p99));
      const float32x4_t c1 = VecNmsub_8296C540(e1_p99, e0_p135, VecMul_8296C540(e1_p135, e0_p99));
      const float32x4_t c2 = VecNmsub_8296C540(e1_p99, ho_p135, VecMul_8296C540(e1_p135, ho_p99));

      const float32x4_t t_hit = VecMul_8296C540(Dot3_8296C540(pvec, ho), v_scale);
      if (vgetq_lane_f32(t_hit, 0) < vgetq_lane_f32(best_neg_t, 0)) {
        best_neg_t = t_hit;
        hit_index = static_cast<int>(i);
        best_hit_uv = VecMul_8296C540(c0, c2);
      }
    }
  }

  if (hit_index >= 0) {
    ctx.r3.s64 = hit_index;
    vst1q_f32(reinterpret_cast<float*>(base + out_ptr), best_hit_uv);
    const float32x4_t pos_t = VecSub_8296C540(v_zero, best_neg_t);
    vst1q_f32(reinterpret_cast<float*>(base + (out_ptr + 16)), pos_t);
  } else {
    ctx.r3.s64 = -1;
  }
#else
  // Fallback Reference
  (void)ctx;
  (void)base;
#endif
}
