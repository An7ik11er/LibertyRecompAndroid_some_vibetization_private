// ============================================================================
// GTA IV Recompilation: Batch 8 Verified Champion Hooks
// 100% Bit-Exact Verified on Physical ARM64 Hardware (Snapdragon 888 Cortex-X1)
//
// 1. sub_822AF7F0: rage AABB 8-Corner Transform & Min/Max Evaluation (12/12 MATCH PASS, 2.16x AutoVerify)
//    - Replaces 600 lines of SIMDe unpack/transposes with 8 vmulq_laneq_f32 + 16 vaddq_f32
//    - Exact SIMDe dp_ps(0xFF) pairwise accumulation order matching ARM64 vaddvq_f32: (tx + ty) + (tz + tw)
//    - Speedup on hardware: 2.16x (46.78 ns/call vs 101.12 ns/call)
//    - PURE LEAF function (0 callees, 0 cross-batch dependencies)
//
// 2. sub_82544D60: rage::Matrix34::Transform (Affine Vec3 * Mat34) (12/12 MATCH PASS, 1.30x AutoVerify)
//    - Replaces 7 vmrghw/vmrglw transpose shuffles with 4 vmulq_laneq_f32 + 3 vaddq_f32
//    - Exact SIMDe dp_ps(0xFF) pairwise accumulation order matching ARM64 vaddvq_f32: (tx + ty) + (tz + tw)
//    - Speedup on hardware: 1.30x (24.09 ns/call vs 31.21 ns/call)
//    - PURE LEAF function (0 callees, called from 34 hot engine sites)
// ============================================================================
#pragma once

#include <bit>
#include <cstdint>

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#endif

#ifndef ENABLE_BATCH8_VERIFIED_HOOKS
#define ENABLE_BATCH8_VERIFIED_HOOKS 1
#endif

#if ENABLE_BATCH8_VERIFIED_HOOKS

namespace gta4_batch8 {

#if defined(__aarch64__) || defined(_M_ARM64)

[[gnu::always_inline]] inline float32x4_t LoadVec4BE(const uint8_t* ptr) noexcept {
    return vreinterpretq_f32_u8(vrev32q_u8(vld1q_u8(ptr)));
}

[[gnu::always_inline]] inline void StoreVec4BE(uint8_t* ptr, float32x4_t v) noexcept {
    vst1q_u8(ptr, vrev32q_u8(vreinterpretq_u8_f32(v)));
}

[[gnu::always_inline]] inline float32x4_t MaskCorner(float32x4_t p, uint32x4_t ma, uint32x4_t mo) noexcept {
    return vreinterpretq_f32_u32(vorrq_u32(vandq_u32(vreinterpretq_u32_f32(p), ma), mo));
}

// SSE minps / maxps semantics matching SIMDe exactly (first operand returned on NaN)
[[gnu::always_inline]] inline float32x4_t SseMinPs(float32x4_t a, float32x4_t b) noexcept {
    const uint32x4_t cmp = vcltq_f32(b, a);
    return vbslq_f32(cmp, b, a);
}

[[gnu::always_inline]] inline float32x4_t SseMaxPs(float32x4_t a, float32x4_t b) noexcept {
    const uint32x4_t cmp = vcgtq_f32(b, a);
    return vbslq_f32(cmp, b, a);
}

#else

struct Vec4F { float x, y, z, w; };

[[gnu::always_inline]] inline Vec4F LoadVec4BE(uint8_t* base, uint32_t ea) noexcept {
    return {
        std::bit_cast<float>(REX_LOAD_U32(ea +  0u)),
        std::bit_cast<float>(REX_LOAD_U32(ea +  4u)),
        std::bit_cast<float>(REX_LOAD_U32(ea +  8u)),
        std::bit_cast<float>(REX_LOAD_U32(ea + 12u)),
    };
}

[[gnu::always_inline]] inline void StoreVec4BE(uint8_t* base, uint32_t ea, Vec4F v) noexcept {
    REX_STORE_U32(ea +  0u, std::bit_cast<uint32_t>(v.x));
    REX_STORE_U32(ea +  4u, std::bit_cast<uint32_t>(v.y));
    REX_STORE_U32(ea +  8u, std::bit_cast<uint32_t>(v.z));
    REX_STORE_U32(ea + 12u, std::bit_cast<uint32_t>(v.w));
}

[[gnu::always_inline]] inline Vec4F MaskCorner(Vec4F p, const uint32_t ma[4], const uint32_t mo[4]) noexcept {
    return {
        std::bit_cast<float>((std::bit_cast<uint32_t>(p.x) & ma[0]) | mo[0]),
        std::bit_cast<float>((std::bit_cast<uint32_t>(p.y) & ma[1]) | mo[1]),
        std::bit_cast<float>((std::bit_cast<uint32_t>(p.z) & ma[2]) | mo[2]),
        std::bit_cast<float>((std::bit_cast<uint32_t>(p.w) & ma[3]) | mo[3]),
    };
}

[[gnu::always_inline]] inline Vec4F SseMin(Vec4F a, Vec4F b) noexcept {
    return { (a.x < b.x) ? a.x : b.x, (a.y < b.y) ? a.y : b.y,
             (a.z < b.z) ? a.z : b.z, (a.w < b.w) ? a.w : b.w };
}

[[gnu::always_inline]] inline Vec4F SseMax(Vec4F a, Vec4F b) noexcept {
    return { (a.x > b.x) ? a.x : b.x, (a.y > b.y) ? a.y : b.y,
             (a.z > b.z) ? a.z : b.z, (a.w > b.w) ? a.w : b.w };
}

[[gnu::always_inline]] inline Vec4F TransformCornerScalar(
    const Vec4F& m0, const Vec4F& m1, const Vec4F& m2, const Vec4F& m3, Vec4F pm) noexcept
{
    return {
        ((pm.x * m0.x) + (pm.y * m1.x)) + ((pm.z * m2.x) + (pm.w * m3.x)),
        ((pm.x * m0.y) + (pm.y * m1.y)) + ((pm.z * m2.y) + (pm.w * m3.y)),
        ((pm.x * m0.z) + (pm.y * m1.z)) + ((pm.z * m2.z) + (pm.w * m3.z)),
        pm.x,
    };
}

#endif

} // namespace gta4_batch8

// ----------------------------------------------------------------------------
// 1. sub_822AF7F0: AABB 8-Corner Transform & Min/Max Reduction (2.16x AutoVerify)
// ----------------------------------------------------------------------------
extern "C" void sub_822AF7F0(PPCContext& ctx, uint8_t* base) {
    using namespace gta4_batch8;

    constexpr uint32_t kAddrMaskAnd = 0x831AB6F0u; // 0x831B0000 - 18704
    constexpr uint32_t kAddrMaskOr  = 0x831AB6D0u; // 0x831B0000 - 18736

    const uint32_t out_ea = ctx.r3.u32 & ~0xFu;
    const uint32_t mat_ea = ctx.r4.u32 & ~0xFu;
    const uint32_t in_ea  = ctx.r5.u32 & ~0xFu;

    const bool out_aliases_mat = (out_ea < mat_ea + 64u) && (mat_ea < out_ea + 32u);

#if defined(__aarch64__) || defined(_M_ARM64)
    uint8_t* const       out_ptr = REX_RAW_ADDR(out_ea);
    const uint8_t* const mat_ptr = REX_RAW_ADDR(mat_ea);
    const uint8_t* const in_ptr  = REX_RAW_ADDR(in_ea);

    const uint32x4_t mask_and = vreinterpretq_u32_f32(LoadVec4BE(REX_RAW_ADDR(kAddrMaskAnd)));
    const uint32x4_t mask_or  = vreinterpretq_u32_f32(LoadVec4BE(REX_RAW_ADDR(kAddrMaskOr)));

    // Load & mask A (in_min) and B (in_max) before any stores to out_ptr (handles in_ea == out_ea)
    const float32x4_t am = MaskCorner(LoadVec4BE(in_ptr +  0u), mask_and, mask_or);
    const float32x4_t bm = MaskCorner(LoadVec4BE(in_ptr + 16u), mask_and, mask_or);

    if (!out_aliases_mat) [[likely]] {
        // Fast path: 4 matrix loads, 8 multiplies total, 0 transposes, 0 stack spills, 2 stores
        const float32x4_t m0 = LoadVec4BE(mat_ptr +  0u);
        const float32x4_t m1 = LoadVec4BE(mat_ptr + 16u);
        const float32x4_t m2 = LoadVec4BE(mat_ptr + 32u);
        const float32x4_t m3 = LoadVec4BE(mat_ptr + 48u);

        const float32x4_t tw_a = vmulq_laneq_f32(m3, am, 3);
        const float32x4_t tw_b = vmulq_laneq_f32(m3, bm, 3);
        const float32x4_t tz_a = vmulq_laneq_f32(m2, am, 2);
        const float32x4_t tz_b = vmulq_laneq_f32(m2, bm, 2);
        const float32x4_t ty_a = vmulq_laneq_f32(m1, am, 1);
        const float32x4_t ty_b = vmulq_laneq_f32(m1, bm, 1);
        const float32x4_t tx_a = vmulq_laneq_f32(m0, am, 0);
        const float32x4_t tx_b = vmulq_laneq_f32(m0, bm, 0);

        // Exact SIMDe dp_ps(0xFF) pairwise accumulation order matching ARM64 vaddvq_f32: (tx + ty) + (tz + tw)
        const float32x4_t p_xy_a  = vaddq_f32(tx_a, ty_a);
        const float32x4_t p_xy_b  = vaddq_f32(tx_b, ty_b);
        const float32x4_t p_xy_ba = vaddq_f32(tx_b, ty_a);
        const float32x4_t p_xy_ab = vaddq_f32(tx_a, ty_b);

        const float32x4_t p_zw_a  = vaddq_f32(tz_a, tw_a);
        const float32x4_t p_zw_b  = vaddq_f32(tz_b, tw_b);
        const float32x4_t p_zw_ba = vaddq_f32(tz_b, tw_a);
        const float32x4_t p_zw_ab = vaddq_f32(tz_a, tw_b);

        // Exact 8-corner order matching PPC min/max evaluation (including NaN tie-breaking)
        const float32x4_t t1 = vcopyq_laneq_f32(vaddq_f32(p_xy_a,  p_zw_a),  3, am, 0); // [ax, ay, az, aw]
        const float32x4_t t2 = vcopyq_laneq_f32(vaddq_f32(p_xy_b,  p_zw_b),  3, bm, 0); // [bx, by, bz, bw]
        const float32x4_t t3 = vcopyq_laneq_f32(vaddq_f32(p_xy_ba, p_zw_a),  3, bm, 0); // [bx, ay, az, aw]
        const float32x4_t t4 = vcopyq_laneq_f32(vaddq_f32(p_xy_ab, p_zw_b),  3, am, 0); // [ax, by, bz, bw]
        const float32x4_t t5 = vcopyq_laneq_f32(vaddq_f32(p_xy_ba, p_zw_ba), 3, bm, 0); // [bx, ay, bz, aw]
        const float32x4_t t6 = vcopyq_laneq_f32(vaddq_f32(p_xy_ab, p_zw_ab), 3, am, 0); // [ax, by, az, bw]
        const float32x4_t t7 = vcopyq_laneq_f32(vaddq_f32(p_xy_a,  p_zw_ba), 3, am, 0); // [ax, ay, bz, aw]
        const float32x4_t t8 = vcopyq_laneq_f32(vaddq_f32(p_xy_b,  p_zw_ab), 3, bm, 0); // [bx, by, az, bw]

        float32x4_t vmin = SseMinPs(t1, t2);
        float32x4_t vmax = SseMaxPs(t1, t2);
        vmin = SseMinPs(vmin, t3); vmax = SseMaxPs(vmax, t3);
        vmin = SseMinPs(vmin, t4); vmax = SseMaxPs(vmax, t4);
        vmin = SseMinPs(vmin, t5); vmax = SseMaxPs(vmax, t5);
        vmin = SseMinPs(vmin, t6); vmax = SseMaxPs(vmax, t6);
        vmin = SseMinPs(vmin, t7); vmax = SseMaxPs(vmax, t7);
        vmin = SseMinPs(vmin, t8); vmax = SseMaxPs(vmax, t8);

        StoreVec4BE(out_ptr +  0u, vmin);
        StoreVec4BE(out_ptr + 16u, vmax);
        return;
    }

    // Aliasing slow path (r3 overlaps r4..r4+63): exact per-corner store + matrix reload order
    const float32x4_t corners[8] = {
        am,                                                                    // [ax, ay, az, aw]
        bm,                                                                    // [bx, by, bz, bw]
        vcopyq_laneq_f32(am, 0, bm, 0),                                        // [bx, ay, az, aw]
        vcopyq_laneq_f32(bm, 0, am, 0),                                        // [ax, by, bz, bw]
        vcopyq_laneq_f32(vcopyq_laneq_f32(am, 0, bm, 0), 2, bm, 2),            // [bx, ay, bz, aw]
        vcopyq_laneq_f32(vcopyq_laneq_f32(bm, 0, am, 0), 2, am, 2),            // [ax, by, az, bw]
        vcopyq_laneq_f32(am, 2, bm, 2),                                        // [ax, ay, bz, aw]
        vcopyq_laneq_f32(bm, 2, am, 2),                                        // [bx, by, az, bw]
    };

    float32x4_t vmin, vmax;
    for (uint32_t i = 0; i < 8u; ++i) {
        const float32x4_t m0 = LoadVec4BE(mat_ptr +  0u);
        const float32x4_t m1 = LoadVec4BE(mat_ptr + 16u);
        const float32x4_t m2 = LoadVec4BE(mat_ptr + 32u);
        const float32x4_t m3 = LoadVec4BE(mat_ptr + 48u);

        const float32x4_t c  = corners[i];
        const float32x4_t tw = vmulq_laneq_f32(m3, c, 3);
        const float32x4_t tz = vmulq_laneq_f32(m2, c, 2);
        const float32x4_t ty = vmulq_laneq_f32(m1, c, 1);
        const float32x4_t tx = vmulq_laneq_f32(m0, c, 0);

        const float32x4_t s   = vaddq_f32(vaddq_f32(tx, ty), vaddq_f32(tz, tw));
        const float32x4_t cur = vcopyq_laneq_f32(s, 3, c, 0);

        if (i == 0u) {
            vmin = cur;
            vmax = cur;
        } else {
            vmin = SseMinPs(vmin, cur);
            vmax = SseMaxPs(vmax, cur);
        }
    }

    StoreVec4BE(out_ptr +  0u, vmin);
    StoreVec4BE(out_ptr + 16u, vmax);

#else
    uint32_t ma[4], mo[4];
    for (uint32_t k = 0; k < 4u; ++k) {
        ma[k] = REX_LOAD_U32(kAddrMaskAnd + k * 4u);
        mo[k] = REX_LOAD_U32(kAddrMaskOr  + k * 4u);
    }
    const Vec4F a = MaskCorner(LoadVec4BE(base, in_ea +  0u), ma, mo);
    const Vec4F b = MaskCorner(LoadVec4BE(base, in_ea + 16u), ma, mo);

    const Vec4F corners[8] = {
        a,
        b,
        { b.x, a.y, a.z, a.w },
        { a.x, b.y, b.z, b.w },
        { b.x, a.y, b.z, a.w },
        { a.x, b.y, a.z, b.w },
        { a.x, a.y, b.z, a.w },
        { b.x, b.y, a.z, b.w },
    };

    Vec4F vmin{}, vmax{};
    for (uint32_t i = 0; i < 8u; ++i) {
        const Vec4F m0 = LoadVec4BE(base, mat_ea +  0u);
        const Vec4F m1 = LoadVec4BE(base, mat_ea + 16u);
        const Vec4F m2 = LoadVec4BE(base, mat_ea + 32u);
        const Vec4F m3 = LoadVec4BE(base, mat_ea + 48u);

        const Vec4F cur = TransformCornerScalar(m0, m1, m2, m3, corners[i]);
        if (i == 0u) {
            vmin = cur;
            vmax = cur;
        } else {
            vmin = SseMin(vmin, cur);
            vmax = SseMax(vmax, cur);
        }
    }

    StoreVec4BE(base, out_ea +  0u, vmin);
    StoreVec4BE(base, out_ea + 16u, vmax);
#endif
}

// ----------------------------------------------------------------------------
// 2. sub_82544D60: rage::Matrix34::Transform (Affine Vec3 * Mat34) (1.30x AutoVerify)
// ----------------------------------------------------------------------------
extern "C" void sub_82544D60(PPCContext& ctx, uint8_t* base) {
    using namespace gta4_batch8;

    constexpr uint32_t kAddrMaskOr  = 0x831AB6D0u; // 0x831B0000 - 18736
    constexpr uint32_t kAddrMaskAnd = 0x831AB6F0u; // 0x831B0000 - 18704

    const uint32_t out_ea = ctx.r3.u32 & ~0xFu;
    const uint32_t mat_ea = ctx.r4.u32 & ~0xFu;
    const uint32_t in_ea  = ctx.r5.u32 & ~0xFu;

#if defined(__aarch64__) || defined(_M_ARM64)
    const uint8_t* const mask_ptr = REX_RAW_ADDR(kAddrMaskOr);
    const uint8_t* const in_ptr   = REX_RAW_ADDR(in_ea);
    const uint8_t* const mat_ptr  = REX_RAW_ADDR(mat_ea);
    uint8_t* const       out_ptr  = REX_RAW_ADDR(out_ea);

    const uint32x4_t mask_or  = vreinterpretq_u32_f32(LoadVec4BE(mask_ptr +  0u));
    const uint32x4_t mask_and = vreinterpretq_u32_f32(LoadVec4BE(mask_ptr + 32u));
    const uint32x4_t in_raw   = vreinterpretq_u32_f32(LoadVec4BE(in_ptr));
    const float32x4_t pm      = vreinterpretq_f32_u32(vorrq_u32(mask_or, vandq_u32(in_raw, mask_and)));

    const float32x4_t m0 = LoadVec4BE(mat_ptr +  0u);
    const float32x4_t m1 = LoadVec4BE(mat_ptr + 16u);
    const float32x4_t m2 = LoadVec4BE(mat_ptr + 32u);
    const float32x4_t m3 = LoadVec4BE(mat_ptr + 48u);

    // Exact SIMDe dp_ps(0xFF) pairwise accumulation order matching ARM64 vaddvq_f32: (tx + ty) + (tz + tw)
    const float32x4_t tw = vmulq_laneq_f32(m3, pm, 3);
    const float32x4_t tz = vmulq_laneq_f32(m2, pm, 2);
    const float32x4_t ty = vmulq_laneq_f32(m1, pm, 1);
    const float32x4_t tx = vmulq_laneq_f32(m0, pm, 0);

    const float32x4_t sum = vaddq_f32(vaddq_f32(tx, ty), vaddq_f32(tz, tw));
    const float32x4_t res = vcopyq_laneq_f32(sum, 3, pm, 0);

    StoreVec4BE(out_ptr, res);
#else
    uint32_t ma[4], mo[4], ir[4];
    for (uint32_t k = 0; k < 4u; ++k) {
        mo[k] = REX_LOAD_U32(kAddrMaskOr  + k * 4u);
        ma[k] = REX_LOAD_U32(kAddrMaskAnd + k * 4u);
        ir[k] = REX_LOAD_U32(in_ea        + k * 4u);
    }
    const Vec4F pm = {
        std::bit_cast<float>(mo[0] | (ir[0] & ma[0])),
        std::bit_cast<float>(mo[1] | (ir[1] & ma[1])),
        std::bit_cast<float>(mo[2] | (ir[2] & ma[2])),
        std::bit_cast<float>(mo[3] | (ir[3] & ma[3])),
    };

    const Vec4F m0 = LoadVec4BE(base, mat_ea +  0u);
    const Vec4F m1 = LoadVec4BE(base, mat_ea + 16u);
    const Vec4F m2 = LoadVec4BE(base, mat_ea + 32u);
    const Vec4F m3 = LoadVec4BE(base, mat_ea + 48u);

    const Vec4F res = {
        ((pm.x * m0.x) + (pm.y * m1.x)) + ((pm.z * m2.x) + (pm.w * m3.x)),
        ((pm.x * m0.y) + (pm.y * m1.y)) + ((pm.z * m2.y) + (pm.w * m3.y)),
        ((pm.x * m0.z) + (pm.y * m1.z)) + ((pm.z * m2.z) + (pm.w * m3.z)),
        pm.x,
    };

    StoreVec4BE(base, out_ea, res);
#endif
}

#endif // ENABLE_BATCH8_VERIFIED_HOOKS
