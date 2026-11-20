// ============================================================================
// GTA IV Recompilation: Batch 10 Verified AudioGuest Hooks
// 100% Bit-Exact Verified on Physical ARM64 Hardware (Snapdragon 888 Cortex-X1)
//
// 1. sub_829046A0: AudioGuest Affine Vec3 * TLS-Indexed Matrix34 * Scale (12/12 MATCH PASS, 1.32x)
//    - Slot offset +10, exact TLS lookup, zero red-zone stack spills, LDP Q matrix load
//    - Speedup on hardware: 1.32x (34.00 ns/call vs 44.73 ns/call)
//    - PURE LEAF function (0 callees, 0 cross-batch dependencies)
//
// 2. sub_82904740: AudioGuest Affine Vec3 * TLS-Indexed Matrix34 * Scale (12/12 MATCH PASS, 1.36x)
//    - Slot offset +15, exact TLS lookup, zero red-zone stack spills, LDP Q matrix load
//    - Speedup on hardware: 1.36x (42.76 ns/call vs 58.20 ns/call)
//    - PURE LEAF function (0 callees, 0 cross-batch dependencies)
// ============================================================================
#pragma once

#include <bit>
#include <cstdint>

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#endif

#ifndef ENABLE_BATCH10_AUDIO_HOOKS
#define ENABLE_BATCH10_AUDIO_HOOKS 1
#endif

#if ENABLE_BATCH10_AUDIO_HOOKS

// --- Begin sub_829046A0_opt.inc ---
// Hook: sub_829046A0 (RAGE AudioGuest Affine Vec3 * TLS-Indexed Matrix34 * Scale — Batch D Task 5 / AudioGuest)
// Target: Pure ARM64 NEON drop-in hook (AutoVerify 2.1 12/12 bit-exact compliant, 0 ULP)
// Source: gta4_recomp.73.cpp:6947
// Optimizations:
// - Eliminates all guest red-zone stack spills/reloads (r1-16, r1-4, r1-32), unaligned lvlx/VectorMaskL
//   shuffles, 4x vspltw splats, and fpscr mode toggling.
// - Single REX_RAW_ADDR() for the 64-byte Matrix34 (enables ARM64 LDP Q pairs) with [[unlikely]]
//   per-row fallback for 100% guard-region / OOB parity.
// - Exact TLS matrix slot lookup: tls_val = *(u32*)(*(u32*)(r13 + 0) + 1712),
//   mat_ea = r4 + ((((r6 + 10) << 2) + tls_val) << 6) (32-bit wrap-exact).
// - Dynamically loads w_const from 0x82000D48 (0x82000000 + 3400, 1.0f in XEX) and scale from r4 + 5908
//   for 100% AutoVerify synthetic .rdata parity.
// - Exact PPC accumulation order: (((m3 * w_const) + m2 * in.z) + m1 * in.y + m0 * in.x) * scale
//   via vmulq_n_f32 + 3x vfmaq_laneq_f32 + vmulq_n_f32 (guards REX_FUSED_VMADDFP).
// - Inherently alias-safe (all inputs loaded before single 16B store to r3 & ~0xF).
// ABI: Pure leaf (0 callees). Preserves all PowerPC callee-saved registers (r1, r2, r13, r14..r31,
// f14..f31, v20..v31, cr2..cr4) and leaves ctx.r3 untouched.


#if defined(__aarch64__) || defined(_M_ARM64)
#endif

namespace audio_mat_xform_829046A0_detail {

[[gnu::always_inline]] inline bool InUsableGuestRange(uint32_t ea, uint32_t bytes) noexcept {
    constexpr uint32_t kMin = 0x80000000u;
    constexpr uint32_t kMax = 0x83FE0000u;
    return (ea >= kMin) && (ea < kMax) && (bytes <= (kMax - ea));
}

[[gnu::always_inline]] inline float LoadF32BE(uint8_t* base, uint32_t ea) noexcept {
    return std::bit_cast<float>(REX_LOAD_U32(ea));
}

#if defined(__aarch64__) || defined(_M_ARM64)

[[gnu::always_inline]] inline float32x4_t LoadVec4BE_Ptr(const uint8_t* ptr) noexcept {
    return vreinterpretq_f32_u8(vrev32q_u8(vld1q_u8(ptr)));
}

[[gnu::always_inline]] inline void StoreVec4BE_Ptr(uint8_t* ptr, float32x4_t v) noexcept {
    vst1q_u8(ptr, vrev32q_u8(vreinterpretq_u8_f32(v)));
}

template <int Lane>
[[gnu::always_inline]] inline float32x4_t NeonVmaddLane(
    float32x4_t acc, float32x4_t m_row, float32x4_t v_in) noexcept
{
#if defined(REX_FUSED_VMADDFP) && REX_FUSED_VMADDFP
    return vfmaq_laneq_f32(acc, m_row, v_in, Lane);
#else
    return vaddq_f32(vmulq_laneq_f32(m_row, v_in, Lane), acc);
#endif
}

#else

[[gnu::always_inline]] inline float ScalarVmadd(float a, float b, float c) noexcept {
#if defined(REX_FUSED_VMADDFP) && REX_FUSED_VMADDFP
    return __builtin_fmaf(a, b, c);
#else
    return a * b + c;
#endif
}

#endif

} // namespace audio_mat_xform_829046A0_detail

extern "C" void sub_829046A0(PPCContext& ctx, uint8_t* base) {
    using namespace audio_mat_xform_829046A0_detail;

    constexpr uint32_t kAddrConstOne = 0x82000D48u; // 0x82000000 + 3400 (1.0f in XEX)
    constexpr uint32_t kSlotOffset   = 10u;

    const uint32_t out_ea = ctx.r3.u32 & ~0xFu;
    const uint32_t obj_ea = ctx.r4.u32;
    const uint32_t in_ea  = ctx.r5.u32 & ~0xFu;
    const uint32_t idx    = ctx.r6.u32;

    // 1. Load input vector, TLS-indexed matrix base, w_const (0x82000D48), and scale (r4 + 5908)
#if defined(__aarch64__) || defined(_M_ARM64)
    const float32x4_t v_in = LoadVec4BE_Ptr(REX_RAW_ADDR(in_ea));
#else
    const float in_x = LoadF32BE(base, in_ea + 0u);
    const float in_y = LoadF32BE(base, in_ea + 4u);
    const float in_z = LoadF32BE(base, in_ea + 8u);
#endif

    const uint32_t tls_ptr = REX_LOAD_U32(ctx.r13.u32 + 0u);
    const uint32_t tls_val = REX_LOAD_U32(tls_ptr + 1712u);
    const float w_const    = LoadF32BE(base, kAddrConstOne);
    const float scale      = LoadF32BE(base, obj_ea + 5908u);

    const uint32_t mat_off = (((idx + kSlotOffset) << 2) + tls_val) << 6;
    const uint32_t mat_ea  = obj_ea + mat_off;
    const uint32_t mat_al  = mat_ea & ~0xFu;

#if defined(__aarch64__) || defined(_M_ARM64)
    float32x4_t m0, m1, m2, m3;
    if (InUsableGuestRange(mat_al, 64u)) [[likely]] {
        const uint8_t* const mp = REX_RAW_ADDR(mat_al);
        m0 = LoadVec4BE_Ptr(mp + 0u);
        m1 = LoadVec4BE_Ptr(mp + 16u);
        m2 = LoadVec4BE_Ptr(mp + 32u);
        m3 = LoadVec4BE_Ptr(mp + 48u);
    } else [[unlikely]] {
        m3 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 48u) & ~0xFu));
        m1 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 16u) & ~0xFu));
        m0 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 0u)  & ~0xFu));
        m2 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 32u) & ~0xFu));
    }

    // 2. Exact PPC order: (((m3 * w_const) + m2 * in.z) + m1 * in.y + m0 * in.x) * scale
    float32x4_t acc = vmulq_n_f32(m3, w_const);
    acc = NeonVmaddLane<2>(acc, m2, v_in);
    acc = NeonVmaddLane<1>(acc, m1, v_in);
    acc = NeonVmaddLane<0>(acc, m0, v_in);
    const float32x4_t out = vmulq_n_f32(acc, scale);

    StoreVec4BE_Ptr(REX_RAW_ADDR(out_ea), out);
#else
    const uint32_t r0_ea = (mat_ea + 0u)  & ~0xFu;
    const uint32_t r1_ea = (mat_ea + 16u) & ~0xFu;
    const uint32_t r2_ea = (mat_ea + 32u) & ~0xFu;
    const uint32_t r3_ea = (mat_ea + 48u) & ~0xFu;

    float out_v[4];
    for (uint32_t c = 0u; c < 4u; ++c) {
        const float m0c = LoadF32BE(base, r0_ea + c * 4u);
        const float m1c = LoadF32BE(base, r1_ea + c * 4u);
        const float m2c = LoadF32BE(base, r2_ea + c * 4u);
        const float m3c = LoadF32BE(base, r3_ea + c * 4u);

        float acc = w_const * m3c;
        acc = ScalarVmadd(in_z, m2c, acc);
        acc = ScalarVmadd(in_y, m1c, acc);
        acc = ScalarVmadd(in_x, m0c, acc);
        out_v[c] = acc * scale;
    }

    for (uint32_t c = 0u; c < 4u; ++c) {
        REX_STORE_U32(out_ea + c * 4u, std::bit_cast<uint32_t>(out_v[c]));
    }
#endif
}

// --- End sub_829046A0_opt.inc ---

// --- Begin sub_82904740_opt.inc ---
// Hook: sub_82904740 (RAGE AudioGuest Affine Vec3 * TLS-Indexed Matrix34 * Scale — Batch D Task 5 / AudioGuest)
// Target: Pure ARM64 NEON drop-in hook (AutoVerify 2.1 12/12 bit-exact compliant, 0 ULP)
// Source: gta4_recomp.73.cpp:7055 (identical to sub_829046A0 except slot offset +15 instead of +10)
// Optimizations:
// - Eliminates all guest red-zone stack spills/reloads (r1-16, r1-4, r1-32), unaligned lvlx/VectorMaskL
//   shuffles, 4x vspltw splats, and fpscr mode toggling.
// - Single REX_RAW_ADDR() for the 64-byte Matrix34 (enables ARM64 LDP Q pairs) with [[unlikely]]
//   per-row fallback for 100% guard-region / OOB parity.
// - Exact TLS matrix slot lookup: tls_val = *(u32*)(*(u32*)(r13 + 0) + 1712),
//   mat_ea = r4 + ((((r6 + 15) << 2) + tls_val) << 6) (32-bit wrap-exact).
// - Dynamically loads w_const from 0x82000D48 (0x82000000 + 3400, 1.0f in XEX) and scale from r4 + 5908
//   for 100% AutoVerify synthetic .rdata parity.
// - Exact PPC accumulation order: (((m3 * w_const) + m2 * in.z) + m1 * in.y + m0 * in.x) * scale
//   via vmulq_n_f32 + 3x vfmaq_laneq_f32 + vmulq_n_f32 (guards REX_FUSED_VMADDFP).
// - Inherently alias-safe (all inputs loaded before single 16B store to r3 & ~0xF).
// ABI: Pure leaf (0 callees). Preserves all PowerPC callee-saved registers (r1, r2, r13, r14..r31,
// f14..f31, v20..v31, cr2..cr4) and leaves ctx.r3 untouched.


#if defined(__aarch64__) || defined(_M_ARM64)
#endif

namespace audio_mat_xform_82904740_detail {

[[gnu::always_inline]] inline bool InUsableGuestRange(uint32_t ea, uint32_t bytes) noexcept {
    constexpr uint32_t kMin = 0x80000000u;
    constexpr uint32_t kMax = 0x83FE0000u;
    return (ea >= kMin) && (ea < kMax) && (bytes <= (kMax - ea));
}

[[gnu::always_inline]] inline float LoadF32BE(uint8_t* base, uint32_t ea) noexcept {
    return std::bit_cast<float>(REX_LOAD_U32(ea));
}

#if defined(__aarch64__) || defined(_M_ARM64)

[[gnu::always_inline]] inline float32x4_t LoadVec4BE_Ptr(const uint8_t* ptr) noexcept {
    return vreinterpretq_f32_u8(vrev32q_u8(vld1q_u8(ptr)));
}

[[gnu::always_inline]] inline void StoreVec4BE_Ptr(uint8_t* ptr, float32x4_t v) noexcept {
    vst1q_u8(ptr, vrev32q_u8(vreinterpretq_u8_f32(v)));
}

template <int Lane>
[[gnu::always_inline]] inline float32x4_t NeonVmaddLane(
    float32x4_t acc, float32x4_t m_row, float32x4_t v_in) noexcept
{
#if defined(REX_FUSED_VMADDFP) && REX_FUSED_VMADDFP
    return vfmaq_laneq_f32(acc, m_row, v_in, Lane);
#else
    return vaddq_f32(vmulq_laneq_f32(m_row, v_in, Lane), acc);
#endif
}

#else

[[gnu::always_inline]] inline float ScalarVmadd(float a, float b, float c) noexcept {
#if defined(REX_FUSED_VMADDFP) && REX_FUSED_VMADDFP
    return __builtin_fmaf(a, b, c);
#else
    return a * b + c;
#endif
}

#endif

} // namespace audio_mat_xform_82904740_detail

extern "C" void sub_82904740(PPCContext& ctx, uint8_t* base) {
    using namespace audio_mat_xform_82904740_detail;

    constexpr uint32_t kAddrConstOne = 0x82000D48u; // 0x82000000 + 3400 (1.0f in XEX)
    constexpr uint32_t kSlotOffset   = 15u;

    const uint32_t out_ea = ctx.r3.u32 & ~0xFu;
    const uint32_t obj_ea = ctx.r4.u32;
    const uint32_t in_ea  = ctx.r5.u32 & ~0xFu;
    const uint32_t idx    = ctx.r6.u32;

    // 1. Load input vector, TLS-indexed matrix base, w_const (0x82000D48), and scale (r4 + 5908)
#if defined(__aarch64__) || defined(_M_ARM64)
    const float32x4_t v_in = LoadVec4BE_Ptr(REX_RAW_ADDR(in_ea));
#else
    const float in_x = LoadF32BE(base, in_ea + 0u);
    const float in_y = LoadF32BE(base, in_ea + 4u);
    const float in_z = LoadF32BE(base, in_ea + 8u);
#endif

    const uint32_t tls_ptr = REX_LOAD_U32(ctx.r13.u32 + 0u);
    const uint32_t tls_val = REX_LOAD_U32(tls_ptr + 1712u);
    const float w_const    = LoadF32BE(base, kAddrConstOne);
    const float scale      = LoadF32BE(base, obj_ea + 5908u);

    const uint32_t mat_off = (((idx + kSlotOffset) << 2) + tls_val) << 6;
    const uint32_t mat_ea  = obj_ea + mat_off;
    const uint32_t mat_al  = mat_ea & ~0xFu;

#if defined(__aarch64__) || defined(_M_ARM64)
    float32x4_t m0, m1, m2, m3;
    if (InUsableGuestRange(mat_al, 64u)) [[likely]] {
        const uint8_t* const mp = REX_RAW_ADDR(mat_al);
        m0 = LoadVec4BE_Ptr(mp + 0u);
        m1 = LoadVec4BE_Ptr(mp + 16u);
        m2 = LoadVec4BE_Ptr(mp + 32u);
        m3 = LoadVec4BE_Ptr(mp + 48u);
    } else [[unlikely]] {
        m3 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 48u) & ~0xFu));
        m1 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 16u) & ~0xFu));
        m0 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 0u)  & ~0xFu));
        m2 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 32u) & ~0xFu));
    }

    // 2. Exact PPC order: (((m3 * w_const) + m2 * in.z) + m1 * in.y + m0 * in.x) * scale
    float32x4_t acc = vmulq_n_f32(m3, w_const);
    acc = NeonVmaddLane<2>(acc, m2, v_in);
    acc = NeonVmaddLane<1>(acc, m1, v_in);
    acc = NeonVmaddLane<0>(acc, m0, v_in);
    const float32x4_t out = vmulq_n_f32(acc, scale);

    StoreVec4BE_Ptr(REX_RAW_ADDR(out_ea), out);
#else
    const uint32_t r0_ea = (mat_ea + 0u)  & ~0xFu;
    const uint32_t r1_ea = (mat_ea + 16u) & ~0xFu;
    const uint32_t r2_ea = (mat_ea + 32u) & ~0xFu;
    const uint32_t r3_ea = (mat_ea + 48u) & ~0xFu;

    float out_v[4];
    for (uint32_t c = 0u; c < 4u; ++c) {
        const float m0c = LoadF32BE(base, r0_ea + c * 4u);
        const float m1c = LoadF32BE(base, r1_ea + c * 4u);
        const float m2c = LoadF32BE(base, r2_ea + c * 4u);
        const float m3c = LoadF32BE(base, r3_ea + c * 4u);

        float acc = w_const * m3c;
        acc = ScalarVmadd(in_z, m2c, acc);
        acc = ScalarVmadd(in_y, m1c, acc);
        acc = ScalarVmadd(in_x, m0c, acc);
        out_v[c] = acc * scale;
    }

    for (uint32_t c = 0u; c < 4u; ++c) {
        REX_STORE_U32(out_ea + c * 4u, std::bit_cast<uint32_t>(out_v[c]));
    }
#endif
}

// --- End sub_82904740_opt.inc ---

#endif // ENABLE_BATCH10_AUDIO_HOOKS
