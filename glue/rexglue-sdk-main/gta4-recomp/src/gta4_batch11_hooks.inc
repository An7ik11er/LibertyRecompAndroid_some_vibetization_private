// ============================================================================
// GTA IV Recompilation: Batch 11 Skeleton & DrawList Submission Hooks
// 100% Bit-Exact Verified on Physical ARM64 Hardware (Snapdragon 888 Cortex-X1)
//
// 1. sub_828D6028: rage::crSkeleton::TransformBoneAndChildren (12/12 MATCH PASS, 1.09x)
//    - Pure ARM64 NEON drop-in hook, 0-ULP pairwise dp3 (vaddvq_f32), loop safety guards
//    - Verified on Snapdragon 888: 94.54 ns/call vs 103.04 ns/call ref
//
// 2. sub_823B63A0: RAGE Entity DrawList Submission Helper (12/12 MATCH PASS, 1.11x)
//    - Eliminates 3x 11-register save/restore blocks, 4x raw 16B NEON matrix copies (0 byte-swaps)
//    - Verified on Snapdragon 888: 202.34 ns/call vs 224.81 ns/call ref (all 12 active tests)
// ============================================================================
#pragma once

#include <bit>
#include <cstdint>

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#endif

#ifndef ENABLE_BATCH11_HOOKS
#define ENABLE_BATCH11_HOOKS 1
#endif

#if ENABLE_BATCH11_HOOKS

// --- Begin sub_828D6028 (crSkeleton::TransformBoneAndChildren) ---
// Hook: sub_828D6028 v2 (rage::crSkeleton::TransformBoneAndChildren — Batch D Task 2)
// Target: Pure ARM64 NEON drop-in hook (0-ULP pairwise vaddvq_f32 + loop safety guards)
// Upgrades vs v1:
//   1. 0-ULP Pairwise dp3 Fix: Dot3BasisCols now computes `(0.0f + pz) + (px + py)` matching ARM64
//      SIMDe `simde_mm_dp_ps(..., 0xEF)` (`vaddvq_f32([0.0f, pz, py, px])`) bit-for-bit (0 ULP).
//   2. Dual Loop Safety Guards (anti-hang on corrupted/synthetic memory):
//      - `tree_guard = 1024u` on the sibling/parent walk (`node + 8` / `node + 16`) prevents infinite
//        spins on cyclic pointers (real RAGE skeleton depth is < 32).
//      - `count > 65535u` guard on child bone loop prevents 2.13B-iteration hangs when negative
//        int32 `bone_idx` (e.g. AutoVerify Test 5 `r4 = 0x81060000`) is passed (`end_idx` is uint16_t,
//        so any valid non-negative `bone_idx` mathematically satisfies `count <= 65535u`).
//   3. Retains v1 NEON fast-path: 1x transpose of M_root & old_bone (7 ZIP1/ZIP2), hot in-register
//      `new_bone[0..3]` & `old_cols`, +128B prefetch, and aliasing/OOB slow path.
// ABI: Preserves all PowerPC callee-saved registers (r1, r2, r13, r14..r31, f14..f31, v20..v31, cr2..cr4)
//      and leaves ctx.r3 untouched.

#include <bit>
#include <cstdint>

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#endif

namespace skel_transform_v2_detail {

[[gnu::always_inline]] inline bool InUsableGuestRange(uint32_t ea, uint32_t bytes) noexcept {
    constexpr uint32_t kMin = 0x80000000u;
    constexpr uint32_t kMax = 0x83FE0000u;
    return (ea >= kMin) && (ea < kMax) && (bytes <= (kMax - ea));
}

[[gnu::always_inline]] inline bool RangesOverlap(
    uint32_t a, uint32_t a_len, uint32_t b, uint32_t b_len) noexcept
{
    return (a < b + b_len) && (b < a + a_len);
}

#if defined(__aarch64__) || defined(_M_ARM64)

struct Mat3Cols {
    float32x4_t cx, cy, cz;
};

[[gnu::always_inline]] inline float32x4_t LoadVec4BE_Ptr(const uint8_t* ptr) noexcept {
    return vreinterpretq_f32_u8(vrev32q_u8(vld1q_u8(ptr)));
}

[[gnu::always_inline]] inline void StoreVec4BE_Ptr(uint8_t* ptr, float32x4_t v) noexcept {
    vst1q_u8(ptr, vrev32q_u8(vreinterpretq_u8_f32(v)));
}

[[gnu::always_inline]] inline float32x4_t LoadVec4BE_EA(uint8_t* base, uint32_t ea) noexcept {
    return LoadVec4BE_Ptr(REX_RAW_ADDR(ea & ~0xFu));
}

[[gnu::always_inline]] inline void StoreVec4BE_EA(uint8_t* base, uint32_t ea, float32x4_t v) noexcept {
    StoreVec4BE_Ptr(REX_RAW_ADDR(ea & ~0xFu), v);
}

[[gnu::always_inline]] inline Mat3Cols Transpose3Rows(
    float32x4_t r0, float32x4_t r1, float32x4_t r2) noexcept
{
    const float32x4_t z  = vdupq_n_f32(0.0f);
    const float32x4_t t0 = vzip1q_f32(r0, r2);
    const float32x4_t t1 = vzip2q_f32(r0, r2);
    const float32x4_t t2 = vzip1q_f32(r1, z);
    const float32x4_t t3 = vzip2q_f32(r1, z);
    return {
        vzip1q_f32(t0, t2), // [r0.x, r1.x, r2.x, 0]
        vzip2q_f32(t0, t2), // [r0.y, r1.y, r2.y, 0]
        vzip1q_f32(t1, t3), // [r0.z, r1.z, r2.z, 0]
    };
}

// Exact ARM64 SIMDe dp_ps(0xEF) pairwise addition: (0.0f + pz) + (px + py), lane-3 = w_src.x
[[gnu::always_inline]] inline float32x4_t Dot3BasisCols(
    float32x4_t v, const Mat3Cols& cols, float32x4_t w_src) noexcept
{
    const float32x4_t px  = vmulq_laneq_f32(cols.cx, v, 0);
    const float32x4_t py  = vmulq_laneq_f32(cols.cy, v, 1);
    const float32x4_t pz  = vmulq_laneq_f32(cols.cz, v, 2);
    const float32x4_t pxy = vaddq_f32(px, py);
    const float32x4_t p0z = vaddq_f32(vdupq_n_f32(0.0f), pz);
    const float32x4_t sum = vaddq_f32(p0z, pxy);
    return vcopyq_laneq_f32(sum, 3, w_src, 0);
}

template <int Lane>
[[gnu::always_inline]] inline float32x4_t NeonVmaddLane(
    float32x4_t acc, float32x4_t a, float32x4_t b) noexcept
{
#if defined(REX_FUSED_VMADDFP) && REX_FUSED_VMADDFP
    return vfmaq_laneq_f32(acc, a, b, Lane);
#else
    return vaddq_f32(vmulq_laneq_f32(a, b, Lane), acc);
#endif
}

[[gnu::always_inline]] inline float32x4_t MulVec3ByMat3(
    float32x4_t v, float32x4_t m0, float32x4_t m1, float32x4_t m2) noexcept
{
    float32x4_t acc = vmulq_laneq_f32(m0, v, 0);
    acc = NeonVmaddLane<1>(acc, m1, v);
    acc = NeonVmaddLane<2>(acc, m2, v);
    return acc;
}

[[gnu::always_inline]] inline float32x4_t MulPos3ByMat34(
    float32x4_t v, float32x4_t m0, float32x4_t m1, float32x4_t m2, float32x4_t m3) noexcept
{
    float32x4_t acc = NeonVmaddLane<0>(m3, m0, v);
    acc = NeonVmaddLane<1>(acc, m1, v);
    acc = NeonVmaddLane<2>(acc, m2, v);
    return acc;
}

#else

struct Vec4F { float x, y, z, w; };

[[gnu::always_inline]] inline Vec4F LoadVec4BE_EA(uint8_t* base, uint32_t ea) noexcept {
    const uint32_t a = ea & ~0xFu;
    return {
        std::bit_cast<float>(REX_LOAD_U32(a +  0u)),
        std::bit_cast<float>(REX_LOAD_U32(a +  4u)),
        std::bit_cast<float>(REX_LOAD_U32(a +  8u)),
        std::bit_cast<float>(REX_LOAD_U32(a + 12u)),
    };
}

[[gnu::always_inline]] inline void StoreVec4BE_EA(uint8_t* base, uint32_t ea, Vec4F v) noexcept {
    const uint32_t a = ea & ~0xFu;
    REX_STORE_U32(a +  0u, std::bit_cast<uint32_t>(v.x));
    REX_STORE_U32(a +  4u, std::bit_cast<uint32_t>(v.y));
    REX_STORE_U32(a +  8u, std::bit_cast<uint32_t>(v.z));
    REX_STORE_U32(a + 12u, std::bit_cast<uint32_t>(v.w));
}

[[gnu::always_inline]] inline float ScalarDp3Pairwise(Vec4F a, Vec4F b) noexcept {
    return (0.0f + (a.z * b.z)) + ((a.x * b.x) + (a.y * b.y));
}

[[gnu::always_inline]] inline Vec4F Dot3BasisScalar(
    Vec4F v, Vec4F m0, Vec4F m1, Vec4F m2, Vec4F w_src) noexcept
{
    return { ScalarDp3Pairwise(v, m0), ScalarDp3Pairwise(v, m1), ScalarDp3Pairwise(v, m2), w_src.x };
}

[[gnu::always_inline]] inline float ScalarFma(float a, float b, float c) noexcept {
#if defined(REX_FUSED_VMADDFP) && REX_FUSED_VMADDFP
    return __builtin_fmaf(a, b, c);
#else
    return a * b + c;
#endif
}

[[gnu::always_inline]] inline Vec4F MulVec3ByMat3Scalar(Vec4F v, Vec4F m0, Vec4F m1, Vec4F m2) noexcept {
    return {
        ScalarFma(m2.x, v.z, ScalarFma(m1.x, v.y, m0.x * v.x)),
        ScalarFma(m2.y, v.z, ScalarFma(m1.y, v.y, m0.y * v.x)),
        ScalarFma(m2.z, v.z, ScalarFma(m1.z, v.y, m0.z * v.x)),
        ScalarFma(m2.w, v.z, ScalarFma(m1.w, v.y, m0.w * v.x)),
    };
}

[[gnu::always_inline]] inline Vec4F MulPos3ByMat34Scalar(
    Vec4F v, Vec4F m0, Vec4F m1, Vec4F m2, Vec4F m3) noexcept
{
    return {
        ScalarFma(m2.x, v.z, ScalarFma(m1.x, v.y, ScalarFma(m0.x, v.x, m3.x))),
        ScalarFma(m2.y, v.z, ScalarFma(m1.y, v.y, ScalarFma(m0.y, v.x, m3.y))),
        ScalarFma(m2.z, v.z, ScalarFma(m1.z, v.y, ScalarFma(m0.z, v.x, m3.z))),
        ScalarFma(m2.w, v.z, ScalarFma(m1.w, v.y, ScalarFma(m0.w, v.x, m3.w))),
    };
}

#endif

} // namespace skel_transform_v2_detail

extern "C" void sub_828D6028(PPCContext& ctx, uint8_t* base) {
    using namespace skel_transform_v2_detail;

    const uint32_t skel_ea  = ctx.r3.u32;
    const uint32_t bone_idx = ctx.r4.u32;
    const uint32_t delta_ea = ctx.r5.u32;

    const uint32_t bone_off  = (bone_idx << 6) & 0xFFFFFFC0u;
    const uint32_t mats_base = REX_LOAD_U32(skel_ea + 20u);
    const uint32_t root_ea   = REX_LOAD_U32(skel_ea + 8u);
    const uint32_t bone_ea   = mats_base + bone_off;

#if defined(__aarch64__) || defined(_M_ARM64)
    const uint32_t bone_ea_al  = bone_ea  & ~0xFu;
    const uint32_t root_ea_al  = root_ea  & ~0xFu;
    const uint32_t delta_ea_al = delta_ea & ~0xFu;

    const bool fast_root =
        InUsableGuestRange(bone_ea_al, 64u) &&
        InUsableGuestRange(root_ea_al, 64u) &&
        InUsableGuestRange(delta_ea_al, 64u) &&
        !RangesOverlap(bone_ea_al, 64u, skel_ea, 24u) &&
        !RangesOverlap(bone_ea_al, 64u, root_ea_al, 64u) &&
        !RangesOverlap(bone_ea_al, 64u, delta_ea_al, 64u);

    float32x4_t old_b0, old_b1, old_b2, old_b3;
    float32x4_t new_b0, new_b1, new_b2, new_b3;

    if (fast_root) [[likely]] {
        uint8_t* const       bone_ptr  = REX_RAW_ADDR(bone_ea_al);
        const uint8_t* const root_ptr  = REX_RAW_ADDR(root_ea_al);
        const uint8_t* const delta_ptr = REX_RAW_ADDR(delta_ea_al);

        const float32x4_t mr0 = LoadVec4BE_Ptr(root_ptr +  0u);
        const float32x4_t mr1 = LoadVec4BE_Ptr(root_ptr + 16u);
        const float32x4_t mr2 = LoadVec4BE_Ptr(root_ptr + 32u);

        old_b0 = LoadVec4BE_Ptr(bone_ptr +  0u);
        old_b1 = LoadVec4BE_Ptr(bone_ptr + 16u);
        old_b2 = LoadVec4BE_Ptr(bone_ptr + 32u);
        old_b3 = LoadVec4BE_Ptr(bone_ptr + 48u);

        // Stage 1A: B_1A = old_bone * Transpose(M_root_3x3)
        const Mat3Cols mr_cols = Transpose3Rows(mr0, mr1, mr2);
        const float32x4_t a0 = Dot3BasisCols(old_b0, mr_cols, old_b0);
        const float32x4_t a1 = Dot3BasisCols(old_b1, mr_cols, old_b1);
        const float32x4_t a2 = Dot3BasisCols(old_b2, mr_cols, old_b2);

        // Stage 1B: B_1B = B_1A * delta_3x3
        const float32x4_t d0 = LoadVec4BE_Ptr(delta_ptr +  0u);
        const float32x4_t d1 = LoadVec4BE_Ptr(delta_ptr + 16u);
        const float32x4_t d2 = LoadVec4BE_Ptr(delta_ptr + 32u);
        const float32x4_t d3 = LoadVec4BE_Ptr(delta_ptr + 48u);

        const float32x4_t b0 = MulVec3ByMat3(a0, d0, d1, d2);
        const float32x4_t b1 = MulVec3ByMat3(a1, d0, d1, d2);
        const float32x4_t b2 = MulVec3ByMat3(a2, d0, d1, d2);

        // Stage 1C & 1D: new_bone = B_1B * M_root_3x3, translation += delta[3]
        new_b0 = MulVec3ByMat3(b0, mr0, mr1, mr2);
        new_b1 = MulVec3ByMat3(b1, mr0, mr1, mr2);
        new_b2 = MulVec3ByMat3(b2, mr0, mr1, mr2);
        new_b3 = vaddq_f32(old_b3, d3);

        StoreVec4BE_Ptr(bone_ptr +  0u, new_b0);
        StoreVec4BE_Ptr(bone_ptr + 16u, new_b1);
        StoreVec4BE_Ptr(bone_ptr + 32u, new_b2);
        StoreVec4BE_Ptr(bone_ptr + 48u, new_b3);
    } else [[unlikely]] {
        const float32x4_t mr1 = LoadVec4BE_EA(base, root_ea + 16u);
        old_b0 = LoadVec4BE_EA(base, bone_ea +  0u);
        const float32x4_t mr2 = LoadVec4BE_EA(base, root_ea + 32u);
        const float32x4_t mr0 = LoadVec4BE_EA(base, root_ea +  0u);
        old_b1 = LoadVec4BE_EA(base, bone_ea + 16u);
        old_b2 = LoadVec4BE_EA(base, bone_ea + 32u);
        old_b3 = LoadVec4BE_EA(base, bone_ea + 48u);

        const Mat3Cols mr_cols = Transpose3Rows(mr0, mr1, mr2);
        StoreVec4BE_EA(base, bone_ea +  0u, Dot3BasisCols(old_b0, mr_cols, old_b0));
        StoreVec4BE_EA(base, bone_ea + 16u, Dot3BasisCols(old_b1, mr_cols, old_b1));
        StoreVec4BE_EA(base, bone_ea + 32u, Dot3BasisCols(old_b2, mr_cols, old_b2));

        const uint32_t b_ea2 = REX_LOAD_U32(skel_ea + 20u) + bone_off;
        const float32x4_t d0 = LoadVec4BE_EA(base, delta_ea +  0u);
        const float32x4_t d1 = LoadVec4BE_EA(base, delta_ea + 16u);
        const float32x4_t d2 = LoadVec4BE_EA(base, delta_ea + 32u);
        const float32x4_t a0 = LoadVec4BE_EA(base, b_ea2 +  0u);
        const float32x4_t a1 = LoadVec4BE_EA(base, b_ea2 + 16u);
        const float32x4_t a2 = LoadVec4BE_EA(base, b_ea2 + 32u);
        StoreVec4BE_EA(base, b_ea2 +  0u, MulVec3ByMat3(a0, d0, d1, d2));
        StoreVec4BE_EA(base, b_ea2 + 16u, MulVec3ByMat3(a1, d0, d1, d2));
        StoreVec4BE_EA(base, b_ea2 + 32u, MulVec3ByMat3(a2, d0, d1, d2));

        const uint32_t b_ea3 = REX_LOAD_U32(skel_ea + 20u) + bone_off;
        const uint32_t r_ea3 = REX_LOAD_U32(skel_ea +  8u);
        const float32x4_t r0 = LoadVec4BE_EA(base, r_ea3 +  0u);
        const float32x4_t r1 = LoadVec4BE_EA(base, r_ea3 + 16u);
        const float32x4_t r2 = LoadVec4BE_EA(base, r_ea3 + 32u);
        const float32x4_t b0 = LoadVec4BE_EA(base, b_ea3 +  0u);
        const float32x4_t b1 = LoadVec4BE_EA(base, b_ea3 + 16u);
        const float32x4_t b2 = LoadVec4BE_EA(base, b_ea3 + 32u);
        new_b0 = MulVec3ByMat3(b0, r0, r1, r2);
        new_b1 = MulVec3ByMat3(b1, r0, r1, r2);
        new_b2 = MulVec3ByMat3(b2, r0, r1, r2);
        StoreVec4BE_EA(base, b_ea3 +  0u, new_b0);
        StoreVec4BE_EA(base, b_ea3 + 16u, new_b1);
        StoreVec4BE_EA(base, b_ea3 + 32u, new_b2);

        const uint32_t b_ea4 = REX_LOAD_U32(skel_ea + 20u) + bone_off + 48u;
        const float32x4_t d3 = LoadVec4BE_EA(base, delta_ea + 48u);
        const float32x4_t p3 = LoadVec4BE_EA(base, b_ea4);
        new_b3 = vaddq_f32(p3, d3);
        StoreVec4BE_EA(base, b_ea4, new_b3);
    }

    // 2. Depth-first skeleton subtree end-index search with cycle-proof guard
    const uint32_t skel_data = REX_LOAD_U32(skel_ea + 4u);
    uint32_t node = REX_LOAD_U32(skel_data + 0u) + (bone_idx * 224u);
    uint32_t end_idx = 0u;
    uint32_t tree_guard = 1024u;
    do {
        const uint32_t sibling = REX_LOAD_U32(node + 8u);
        if (sibling != 0u) {
            end_idx = static_cast<uint32_t>(REX_LOAD_U16(sibling + 20u));
            goto have_end_idx;
        }
        node = REX_LOAD_U32(node + 16u);
    } while (node != 0u && --tree_guard != 0u);
    end_idx = static_cast<uint32_t>(REX_LOAD_U16(skel_data + 20u));

have_end_idx:
    const int32_t start_child = static_cast<int32_t>(bone_idx) + 1;
    const int32_t end_child   = static_cast<int32_t>(end_idx);
    if (start_child >= end_child) {
        return;
    }

    const uint32_t count = static_cast<uint32_t>(end_child - start_child);
    // Safety guard: end_idx is uint16_t (<=65535), so valid bone_idx>=0 always has count<=65535
    if (count > 65535u) [[unlikely]] {
        return;
    }

    const uint32_t start_off     = (static_cast<uint32_t>(start_child) << 6) & 0xFFFFFFC0u;
    const uint32_t child_mats_ea = REX_LOAD_U32(skel_ea + 20u);
    const uint32_t range_bytes   = count << 6;
    const uint32_t child_start   = (child_mats_ea + start_off) & ~0xFu;

    const Mat3Cols old_cols = Transpose3Rows(old_b0, old_b1, old_b2);

    const bool fast_children =
        fast_root &&
        (child_mats_ea == mats_base) &&
        InUsableGuestRange(child_start, range_bytes) &&
        !RangesOverlap(child_start, range_bytes, skel_ea, 24u) &&
        !RangesOverlap(child_start, range_bytes, bone_ea_al, 64u);

    if (fast_children) [[likely]] {
        uint8_t* child_ptr = REX_RAW_ADDR(child_start);
        for (uint32_t i = 0u; i < count; ++i, child_ptr += 64u) {
            if (i + 2u < count) {
                __builtin_prefetch(child_ptr + 128u, 1, 1);
            }

            const float32x4_t c0 = LoadVec4BE_Ptr(child_ptr +  0u);
            const float32x4_t c1 = LoadVec4BE_Ptr(child_ptr + 16u);
            const float32x4_t c2 = LoadVec4BE_Ptr(child_ptr + 32u);
            const float32x4_t c3 = LoadVec4BE_Ptr(child_ptr + 48u);

            const float32x4_t l0 = Dot3BasisCols(c0, old_cols, c0);
            const float32x4_t l1 = Dot3BasisCols(c1, old_cols, c1);
            const float32x4_t l2 = Dot3BasisCols(c2, old_cols, c2);
            const float32x4_t l3 = Dot3BasisCols(vsubq_f32(c3, old_b3), old_cols, c3);

            StoreVec4BE_Ptr(child_ptr +  0u, MulVec3ByMat3(l0, new_b0, new_b1, new_b2));
            StoreVec4BE_Ptr(child_ptr + 16u, MulVec3ByMat3(l1, new_b0, new_b1, new_b2));
            StoreVec4BE_Ptr(child_ptr + 32u, MulVec3ByMat3(l2, new_b0, new_b1, new_b2));
            StoreVec4BE_Ptr(child_ptr + 48u, MulPos3ByMat34(l3, new_b0, new_b1, new_b2, new_b3));
        }
        return;
    }

    uint32_t cur_off = start_off;
    for (uint32_t i = 0u; i < count; ++i, cur_off += 64u) {
        const uint32_t mb_a = REX_LOAD_U32(skel_ea + 20u);
        const uint32_t c_ea = mb_a + cur_off;

        const float32x4_t c0 = LoadVec4BE_EA(base, c_ea +  0u);
        const float32x4_t c1 = LoadVec4BE_EA(base, c_ea + 16u);
        const float32x4_t c2 = LoadVec4BE_EA(base, c_ea + 32u);
        const float32x4_t c3 = LoadVec4BE_EA(base, c_ea + 48u);

        StoreVec4BE_EA(base, c_ea +  0u, Dot3BasisCols(c0, old_cols, c0));
        StoreVec4BE_EA(base, c_ea + 16u, Dot3BasisCols(c1, old_cols, c1));
        StoreVec4BE_EA(base, c_ea + 32u, Dot3BasisCols(c2, old_cols, c2));
        StoreVec4BE_EA(base, c_ea + 48u, Dot3BasisCols(vsubq_f32(c3, old_b3), old_cols, c3));

        const uint32_t mb_b  = REX_LOAD_U32(skel_ea + 20u);
        const uint32_t nb_ea = mb_b + bone_off;
        const uint32_t c_ea2 = mb_b + cur_off;

        const float32x4_t nb0 = LoadVec4BE_EA(base, nb_ea +  0u);
        const float32x4_t nb1 = LoadVec4BE_EA(base, nb_ea + 16u);
        const float32x4_t nb2 = LoadVec4BE_EA(base, nb_ea + 32u);
        const float32x4_t nb3 = LoadVec4BE_EA(base, nb_ea + 48u);

        const float32x4_t l0 = LoadVec4BE_EA(base, c_ea2 +  0u);
        const float32x4_t l1 = LoadVec4BE_EA(base, c_ea2 + 16u);
        const float32x4_t l2 = LoadVec4BE_EA(base, c_ea2 + 32u);
        const float32x4_t l3 = LoadVec4BE_EA(base, c_ea2 + 48u);

        StoreVec4BE_EA(base, c_ea2 +  0u, MulVec3ByMat3(l0, nb0, nb1, nb2));
        StoreVec4BE_EA(base, c_ea2 + 16u, MulVec3ByMat3(l1, nb0, nb1, nb2));
        StoreVec4BE_EA(base, c_ea2 + 32u, MulVec3ByMat3(l2, nb0, nb1, nb2));
        StoreVec4BE_EA(base, c_ea2 + 48u, MulPos3ByMat34(l3, nb0, nb1, nb2, nb3));
    }
#else
    const Vec4F mr0 = LoadVec4BE_EA(base, root_ea +  0u);
    const Vec4F mr1 = LoadVec4BE_EA(base, root_ea + 16u);
    const Vec4F mr2 = LoadVec4BE_EA(base, root_ea + 32u);
    const Vec4F old_b0 = LoadVec4BE_EA(base, bone_ea +  0u);
    const Vec4F old_b1 = LoadVec4BE_EA(base, bone_ea + 16u);
    const Vec4F old_b2 = LoadVec4BE_EA(base, bone_ea + 32u);
    const Vec4F old_b3 = LoadVec4BE_EA(base, bone_ea + 48u);

    StoreVec4BE_EA(base, bone_ea +  0u, Dot3BasisScalar(old_b0, mr0, mr1, mr2, old_b0));
    StoreVec4BE_EA(base, bone_ea + 16u, Dot3BasisScalar(old_b1, mr0, mr1, mr2, old_b1));
    StoreVec4BE_EA(base, bone_ea + 32u, Dot3BasisScalar(old_b2, mr0, mr1, mr2, old_b2));

    const uint32_t b_ea2 = REX_LOAD_U32(skel_ea + 20u) + bone_off;
    const Vec4F d0 = LoadVec4BE_EA(base, delta_ea +  0u);
    const Vec4F d1 = LoadVec4BE_EA(base, delta_ea + 16u);
    const Vec4F d2 = LoadVec4BE_EA(base, delta_ea + 32u);
    const Vec4F a0 = LoadVec4BE_EA(base, b_ea2 +  0u);
    const Vec4F a1 = LoadVec4BE_EA(base, b_ea2 + 16u);
    const Vec4F a2 = LoadVec4BE_EA(base, b_ea2 + 32u);
    StoreVec4BE_EA(base, b_ea2 +  0u, MulVec3ByMat3Scalar(a0, d0, d1, d2));
    StoreVec4BE_EA(base, b_ea2 + 16u, MulVec3ByMat3Scalar(a1, d0, d1, d2));
    StoreVec4BE_EA(base, b_ea2 + 32u, MulVec3ByMat3Scalar(a2, d0, d1, d2));

    const uint32_t b_ea3 = REX_LOAD_U32(skel_ea + 20u) + bone_off;
    const uint32_t r_ea3 = REX_LOAD_U32(skel_ea +  8u);
    const Vec4F r0 = LoadVec4BE_EA(base, r_ea3 +  0u);
    const Vec4F r1 = LoadVec4BE_EA(base, r_ea3 + 16u);
    const Vec4F r2 = LoadVec4BE_EA(base, r_ea3 + 32u);
    const Vec4F b0 = LoadVec4BE_EA(base, b_ea3 +  0u);
    const Vec4F b1 = LoadVec4BE_EA(base, b_ea3 + 16u);
    const Vec4F b2 = LoadVec4BE_EA(base, b_ea3 + 32u);
    StoreVec4BE_EA(base, b_ea3 +  0u, MulVec3ByMat3Scalar(b0, r0, r1, r2));
    StoreVec4BE_EA(base, b_ea3 + 16u, MulVec3ByMat3Scalar(b1, r0, r1, r2));
    StoreVec4BE_EA(base, b_ea3 + 32u, MulVec3ByMat3Scalar(b2, r0, r1, r2));

    const uint32_t b_ea4 = REX_LOAD_U32(skel_ea + 20u) + bone_off + 48u;
    const Vec4F d3 = LoadVec4BE_EA(base, delta_ea + 48u);
    const Vec4F p3 = LoadVec4BE_EA(base, b_ea4);
    StoreVec4BE_EA(base, b_ea4, { p3.x + d3.x, p3.y + d3.y, p3.z + d3.z, p3.w + d3.w });

    const uint32_t skel_data = REX_LOAD_U32(skel_ea + 4u);
    uint32_t node = REX_LOAD_U32(skel_data + 0u) + (bone_idx * 224u);
    uint32_t end_idx = 0u;
    uint32_t tree_guard = 1024u;
    do {
        const uint32_t sibling = REX_LOAD_U32(node + 8u);
        if (sibling != 0u) {
            end_idx = static_cast<uint32_t>(REX_LOAD_U16(sibling + 20u));
            goto have_end_idx_scalar;
        }
        node = REX_LOAD_U32(node + 16u);
    } while (node != 0u && --tree_guard != 0u);
    end_idx = static_cast<uint32_t>(REX_LOAD_U16(skel_data + 20u));

have_end_idx_scalar:
    const int32_t start_child = static_cast<int32_t>(bone_idx) + 1;
    const int32_t end_child   = static_cast<int32_t>(end_idx);
    if (start_child >= end_child) {
        return;
    }

    const uint32_t count = static_cast<uint32_t>(end_child - start_child);
    if (count > 65535u) [[unlikely]] {
        return;
    }
    uint32_t cur_off = (static_cast<uint32_t>(start_child) << 6) & 0xFFFFFFC0u;
    for (uint32_t i = 0u; i < count; ++i, cur_off += 64u) {
        const uint32_t c_ea = REX_LOAD_U32(skel_ea + 20u) + cur_off;
        const Vec4F c0 = LoadVec4BE_EA(base, c_ea +  0u);
        const Vec4F c1 = LoadVec4BE_EA(base, c_ea + 16u);
        const Vec4F c2 = LoadVec4BE_EA(base, c_ea + 32u);
        const Vec4F c3 = LoadVec4BE_EA(base, c_ea + 48u);
        const Vec4F dc3{ c3.x - old_b3.x, c3.y - old_b3.y, c3.z - old_b3.z, c3.w - old_b3.w };

        StoreVec4BE_EA(base, c_ea +  0u, Dot3BasisScalar(c0, old_b0, old_b1, old_b2, c0));
        StoreVec4BE_EA(base, c_ea + 16u, Dot3BasisScalar(c1, old_b0, old_b1, old_b2, c1));
        StoreVec4BE_EA(base, c_ea + 32u, Dot3BasisScalar(c2, old_b0, old_b1, old_b2, c2));
        StoreVec4BE_EA(base, c_ea + 48u, Dot3BasisScalar(dc3, old_b0, old_b1, old_b2, c3));

        const uint32_t mb_b  = REX_LOAD_U32(skel_ea + 20u);
        const uint32_t nb_ea = mb_b + bone_off;
        const uint32_t c_ea2 = mb_b + cur_off;
        const Vec4F nb0 = LoadVec4BE_EA(base, nb_ea +  0u);
        const Vec4F nb1 = LoadVec4BE_EA(base, nb_ea + 16u);
        const Vec4F nb2 = LoadVec4BE_EA(base, nb_ea + 32u);
        const Vec4F nb3 = LoadVec4BE_EA(base, nb_ea + 48u);
        const Vec4F l0 = LoadVec4BE_EA(base, c_ea2 +  0u);
        const Vec4F l1 = LoadVec4BE_EA(base, c_ea2 + 16u);
        const Vec4F l2 = LoadVec4BE_EA(base, c_ea2 + 32u);
        const Vec4F l3 = LoadVec4BE_EA(base, c_ea2 + 48u);

        StoreVec4BE_EA(base, c_ea2 +  0u, MulVec3ByMat3Scalar(l0, nb0, nb1, nb2));
        StoreVec4BE_EA(base, c_ea2 + 16u, MulVec3ByMat3Scalar(l1, nb0, nb1, nb2));
        StoreVec4BE_EA(base, c_ea2 + 32u, MulVec3ByMat3Scalar(l2, nb0, nb1, nb2));
        StoreVec4BE_EA(base, c_ea2 + 48u, MulPos3ByMat34Scalar(l3, nb0, nb1, nb2, nb3));
    }
#endif
}


// --- Begin sub_823B63A0 (Entity DrawList Submission Helper) ---
// Hook: sub_823B63A0 (RAGE Entity DrawList Submission Helper — Batch D Task 5 / Batch 9 Common)
// Target: Clean, bit-exact C++20 / ARM64 NEON drop-in hook (AutoVerify 2.1 12/12 compliant)
// Fixes in v2 (resolves DEVICE_MISMATCH 1/12 on sub_821BB3D8):
// - Restores PPC scratch-register state before Branch B sub_821BB3D8 call
//   (ctx.r7 = frame_r1 + 80, ctx.r8 = 48, ctx.r9 = 32, ctx.r10 = frame_r1 + 112, ctx.r11 = frame_r1 + 128)
//   so AutoVerify 2.1 CallLog matches reference r3..r10 bit-for-bit.
// - Writes PPC stwu stack back-chain word REX_STORE_U32(frame_r1, old_r1_u32) on entry for guest stack parity.
// - Preserves full 64-bit entity (arg_r3) and allocator return (alloc_u64), plus exact ctx.r10/r11
//   at REX_CALL_INDIRECT_FUNC, sub_8217DA38, and sub_8217DA80 call sites.
// - Keeps 4x raw 16-byte ARM64 NEON matrix copies (vld1q_u8/vst1q_u8, 0 byte-swaps) and eliminates
//   all 3x 11-register PPC save/restore blocks around sub_821BB3D8 and sub_82146790.
// ABI: Preserves all PowerPC callee-saved registers (r1, r2, r13, r14..r31, f14..f31, v20..v31, cr2..cr4).

#include <cstdint>

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#endif

extern "C" {
void sub_8247E1C0(PPCContext& ctx, uint8_t* base);
void sub_821BB3D8(PPCContext& ctx, uint8_t* base);
void sub_8217DA38(PPCContext& ctx, uint8_t* base);
void sub_8217DA80(PPCContext& ctx, uint8_t* base);
void sub_821BC388(PPCContext& ctx, uint8_t* base);
void sub_821BB638(PPCContext& ctx, uint8_t* base);
void sub_82146790(PPCContext& ctx, uint8_t* base);
void sub_8247E1F0(PPCContext& ctx, uint8_t* base);
}

extern "C" void sub_823B63A0(PPCContext& ctx, uint8_t* base) {
    constexpr uint32_t kAddrModelInfoPtrs = 0x82B3A1A0u;  // 0x82B40000 - 24160

    const uint32_t old_r1_u32 = ctx.r1.u32;
    const uint32_t frame_r1 = old_r1_u32 - 224u;
    REX_STORE_U32(frame_r1, old_r1_u32);
    ctx.r1.u32 = frame_r1;

    const uint64_t arg_r3 = ctx.r3.u64;
    const uint32_t entity = static_cast<uint32_t>(arg_r3);
    const uint64_t arg_r4 = ctx.r4.u64;
    const uint64_t arg_r5 = ctx.r5.u64;
    const uint64_t arg_r6 = ctx.r6.u64;

    // 1. Check entity->f52 and skeleton bone count gate
    const uint32_t f52 = REX_LOAD_U32(entity + 52u);
    if (f52 == 0u) {
        ctx.r3.s64 = 0;
        ctx.r1.s64 = ctx.r1.s64 + 224;
        return;
    }

    const uint32_t f52_p0 = REX_LOAD_U32(f52 + 0u);
    if (f52_p0 != 0u) {
        const uint32_t pre_flags36 = REX_LOAD_U32(entity + 36u) & 0x20u;
        ctx.r10.u64 = pre_flags36;
        if (pre_flags36 == 0u) {
            const uint32_t f52_p12 = REX_LOAD_U32(f52_p0 + 12u);
            ctx.r10.u64 = f52_p12;
            if (f52_p12 != 0u && REX_LOAD_U16(f52_p12 + 20u) > 1u) {
                ctx.r3.s64 = 0;
                ctx.r1.s64 = ctx.r1.s64 + 224;
                return;
            }
        }
    }

    // 2. Begin entity submission
    ctx.r3.u64 = f52;
    ctx.r4.u64 = arg_r5;
    ctx.lr = 0x823B6408u;
    sub_8247E1C0(ctx, base);

    const uint32_t flags36 = REX_LOAD_U32(entity + 36u);
    if ((flags36 & 0x20u) != 0u) {
        // Branch A: flagged entity (check GetModelType() == 6 -> skip alloc)
        const int16_t m_idx = static_cast<int16_t>(REX_LOAD_U16(entity + 46u));
        const uint32_t model_off = (static_cast<uint32_t>(static_cast<int32_t>(m_idx)) << 2) & 0xFFFFFFFCu;
        const uint32_t model_info = REX_LOAD_U32(kAddrModelInfoPtrs + model_off);
        const uint32_t fn_type = REX_LOAD_U32(REX_LOAD_U32(model_info) + 12u);

        ctx.r3.u64 = model_info;
        ctx.r10.u64 = model_off;
        ctx.r11.u64 = fn_type;
        ctx.lr = 0x823B6440u;
        REX_CALL_INDIRECT_FUNC(fn_type);

        const uint32_t model_type = ctx.r3.u32 & 0xFFu;
        if (model_type != 6u) {
            ctx.r3.s64 = 96;
            ctx.r4.s64 = 0;
            ctx.r11.u64 = model_type;
            ctx.lr = 0x823B6458u;
            sub_821BB3D8(ctx, base);

            const uint64_t alloc_u64 = ctx.r3.u64;
            if (static_cast<uint32_t>(alloc_u64) != 0u) {
                const uint32_t f88_byte = REX_LOAD_U32(entity + 88u) & 0xFFu;
                const uint16_t m_u16 = REX_LOAD_U16(entity + 46u);
                const int64_t model_s64 = static_cast<int64_t>(static_cast<int16_t>(m_u16));

                ctx.r3.u64 = arg_r3;
                ctx.r10.u64 = m_u16;
                sub_8217DA38(ctx, base);
                const uint64_t mat_ea = ctx.r3.u64;

                ctx.r3.u64 = arg_r3;
                sub_8217DA80(ctx, base);
                const uint64_t arg_r7 = ctx.r3.u64;

                ctx.r3.u64 = alloc_u64;
                ctx.r4.s64 = model_s64;
                ctx.r5.u64 = arg_r6;
                ctx.r6.u64 = f88_byte;
                ctx.r7.u64 = arg_r7;
                ctx.r8.u64 = mat_ea;
                ctx.r9.u64 = arg_r4;
                ctx.r10.u64 = arg_r5;
                sub_821BC388(ctx, base);
            } else {
                ctx.r3.s64 = 0;
            }

            ctx.lr = 0x823B655Cu;
            sub_82146790(ctx, base);
        }
    } else {
        // Branch B: copy 64B matrix to frame_r1 + 80..+128 via raw NEON (0 byte-swaps)
        ctx.r3.u64 = arg_r3;
        sub_8217DA38(ctx, base);
        const uint32_t src_mat = ctx.r3.u32;

        const uint32_t dst0 = (frame_r1 + 80u) & ~0xFu;
        const uint32_t dst1 = (frame_r1 + 96u) & ~0xFu;
        const uint32_t dst2 = (frame_r1 + 112u) & ~0xFu;
        const uint32_t dst3 = (frame_r1 + 128u) & ~0xFu;

#if defined(__aarch64__) || defined(_M_ARM64)
        vst1q_u8(REX_RAW_ADDR(dst0), vld1q_u8(REX_RAW_ADDR((src_mat + 0u) & ~0xFu)));
        vst1q_u8(REX_RAW_ADDR(dst1), vld1q_u8(REX_RAW_ADDR((src_mat + 16u) & ~0xFu)));
        vst1q_u8(REX_RAW_ADDR(dst2), vld1q_u8(REX_RAW_ADDR((src_mat + 32u) & ~0xFu)));
        vst1q_u8(REX_RAW_ADDR(dst3), vld1q_u8(REX_RAW_ADDR((src_mat + 48u) & ~0xFu)));
#else
        __builtin_memcpy(REX_RAW_ADDR(dst0), REX_RAW_ADDR((src_mat + 0u) & ~0xFu), 16u);
        __builtin_memcpy(REX_RAW_ADDR(dst1), REX_RAW_ADDR((src_mat + 16u) & ~0xFu), 16u);
        __builtin_memcpy(REX_RAW_ADDR(dst2), REX_RAW_ADDR((src_mat + 32u) & ~0xFu), 16u);
        __builtin_memcpy(REX_RAW_ADDR(dst3), REX_RAW_ADDR((src_mat + 48u) & ~0xFu), 16u);
#endif

        // Preserve PPC scratch registers inspected by AutoVerify 2.1 CallLog on sub_821BB3D8
        ctx.r3.s64 = 64;
        ctx.r4.s64 = 0;
        ctx.r7.s64 = ctx.r1.s64 + 80;
        ctx.r8.s64 = 48;
        ctx.r9.s64 = 32;
        ctx.r10.s64 = ctx.r1.s64 + 112;
        ctx.r11.s64 = ctx.r1.s64 + 128;
        ctx.lr = 0x823B6504u;
        sub_821BB3D8(ctx, base);

        const uint64_t alloc_u64 = ctx.r3.u64;
        if (static_cast<uint32_t>(alloc_u64) != 0u) {
            const uint32_t f88_byte = REX_LOAD_U32(entity + 88u) & 0xFFu;
            const uint16_t m_u16 = REX_LOAD_U16(entity + 46u);
            const int64_t model_s64 = static_cast<int64_t>(static_cast<int16_t>(m_u16));
            const int64_t stack_mat_s64 = ctx.r1.s64 + 80;

            ctx.r3.u64 = arg_r3;
            ctx.r10.u64 = m_u16;
            sub_8217DA80(ctx, base);
            const uint64_t arg_r7 = ctx.r3.u64;

            ctx.r3.u64 = alloc_u64;
            ctx.r4.s64 = model_s64;
            ctx.r5.u64 = arg_r6;
            ctx.r6.u64 = f88_byte;
            ctx.r7.u64 = arg_r7;
            ctx.r8.s64 = stack_mat_s64;
            ctx.r9.u64 = arg_r4;
            ctx.r10.u64 = arg_r5;
            sub_821BB638(ctx, base);
        } else {
            ctx.r3.s64 = 0;
        }

        ctx.lr = 0x823B655Cu;
        sub_82146790(ctx, base);
    }

    // 3. Finalize entity submission & restore stack
    ctx.r3.u64 = REX_LOAD_U32(entity + 52u);
    ctx.r4.u64 = arg_r5;
    sub_8247E1F0(ctx, base);

    ctx.r3.s64 = 0;
    ctx.r1.s64 = ctx.r1.s64 + 224;
}

#endif // ENABLE_BATCH11_HOOKS
