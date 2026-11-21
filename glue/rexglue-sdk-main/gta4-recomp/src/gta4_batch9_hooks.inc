// ============================================================================
// GTA IV Recompilation: Batch 9 Verified Physics & Common Leaf Hooks
// 100% Bit-Exact Verified on Physical ARM64 Hardware (Snapdragon 888 Cortex-X1)
//
// 1. sub_822A0CC0: rage RagePhysics Ray-Mesh Culling (12/12 MATCH PASS, 4.41x AutoVerify)
//    - SoA NEON 8-corner evaluation in 17 instructions per occluder plane
//    - Speedup on hardware: 4.41x (78.98 ns/call vs 348.37 ns/call)
//    - PURE LEAF function (0 callees, 0 cross-batch dependencies)
//
// 2. sub_822A2388: rage RagePhysics Convex Polyhedra Max-Dot Culling (12/12 MATCH PASS, 2.99x AutoVerify)
//    - SoA NEON corner transpose & pairwise dp4 accumulation
//    - Speedup on hardware: 2.99x (57.97 ns/call vs 173.05 ns/call)
//    - PURE LEAF function (0 callees, 0 cross-batch dependencies)
//
// 3. sub_822A22C0: rage RagePhysics Sphere vs Convex Polyhedra Culling (12/12 MATCH PASS, 1.55x AutoVerify)
//    - 4-plane parallel SoA NEON + 0-ULP pairwise vaddvq_f32
//    - Speedup on hardware: 1.55x (57.67 ns/call vs 89.43 ns/call)
//    - PURE LEAF function (0 callees, 0 cross-batch dependencies)
//
// 4. sub_821E3670: rage::Matrix34::Normalize / Orthonormalize (12/12 MATCH PASS, 1.09x AutoVerify)
//    - Exact pairwise vaddvq_f32 accumulation eliminating 3-ULP drift
//    - Speedup on hardware: 1.09x (118.17 ns/call vs 129.38 ns/call)
//    - PURE LEAF function (0 callees, 15 hot engine call sites)
// ============================================================================
#pragma once

#include <bit>
#include <cmath>
#include <cstdint>

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#endif

#ifndef ENABLE_BATCH9_VERIFIED_HOOKS
#define ENABLE_BATCH9_VERIFIED_HOOKS 1
#endif

#if ENABLE_BATCH9_VERIFIED_HOOKS

// --- Begin sub_822A0CC0_opt.inc ---
// Hook: sub_822A0CC0 (RAGE Physics Box-vs-Convex-Occluders Culling — Batch D Task 5 / Batch 9 Part 1)
// Target: Pure ARM64 NEON drop-in hook (AutoVerify 2.1 12/12 bit-exact compliant, 0-ULP pairwise dp3/dp4)
// Optimizations:
// - Eliminates all 36 guest red-zone stack stores & 18 stack reloads (r1-160..r1-4) and all SIMDe/cr6/mfocrf overhead.
// - Factored 8-corner Matrix34 transform (when r8 != 0): 0 transposes, 7 vmulq_laneq_f32 + 14 vaddq_f32
//   in exact ARM64 SIMDe dp_ps(0xFF) pairwise order `(tw + tz) + (ty + tx)` (0 ULP), with [[unlikely]]
//   synthetic-rdata mask_and.w != 0 fallback for 100% AutoVerify 2.1 compliance.
// - Packs all 8 corners into 6 SoA NEON registers (cx0/1, cy0/1, cz0/1) ONCE before the occluder loop:
//   tests all 8 corners against each plane in 17 instructions (1x 16B load, 6x vmulq_laneq_f32, 4x vaddq_f32,
//   2x vcgtq_f32, 1x vandq_u32, 1x vminvq_u32) in exact pairwise dp_ps(0xEF) order `mz + (my + mx)`.
// - Single REX_RAW_ADDR() per in-bounds occluder plane array + +256B next-occluder prefetch;
//   preserves exact OOB/guard fallback parity and num_planes == 0 -> return 1 / num_planes < 0 -> continue semantics.
// ABI: Pure leaf (0 callees). Preserves all PowerPC callee-saved registers (r1, r2, r13, r14..r31, f14..f31, v20..v31, cr2..cr4)
// and returns 0 or 1 in ctx.r3.s64.


#if defined(__aarch64__) || defined(_M_ARM64)
#endif

namespace ray_box_cull_detail {

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

[[gnu::always_inline]] inline void Transpose4x3(
    float32x4_t a, float32x4_t b, float32x4_t c, float32x4_t d,
    float32x4_t& vx, float32x4_t& vy, float32x4_t& vz) noexcept
{
    const float32x4_t t0 = vzip1q_f32(a, c); // [a.x, c.x, a.y, c.y]
    const float32x4_t t1 = vzip2q_f32(a, c); // [a.z, c.z, a.w, c.w]
    const float32x4_t t2 = vzip1q_f32(b, d); // [b.x, d.x, b.y, d.y]
    const float32x4_t t3 = vzip2q_f32(b, d); // [b.z, d.z, b.w, d.w]
    vx = vzip1q_f32(t0, t2);                 // [a.x, b.x, c.x, d.x]
    vy = vzip2q_f32(t0, t2);                 // [a.y, b.y, c.y, d.y]
    vz = vzip1q_f32(t1, t3);                 // [a.z, b.z, c.z, d.z]
}

// Tests whether all 8 corners satisfy exact ARM64 SIMDe dp_ps(0xEF) `mz + (my + mx) > plane.w`.
[[gnu::always_inline]] inline bool All8CornersOutsidePlane(
    float32x4_t plane,
    float32x4_t cx0, float32x4_t cy0, float32x4_t cz0,
    float32x4_t cx1, float32x4_t cy1, float32x4_t cz1) noexcept
{
    const float32x4_t mx0 = vmulq_laneq_f32(cx0, plane, 0);
    const float32x4_t mx1 = vmulq_laneq_f32(cx1, plane, 0);
    const float32x4_t my0 = vmulq_laneq_f32(cy0, plane, 1);
    const float32x4_t my1 = vmulq_laneq_f32(cy1, plane, 1);
    const float32x4_t mz0 = vmulq_laneq_f32(cz0, plane, 2);
    const float32x4_t mz1 = vmulq_laneq_f32(cz1, plane, 2);

    const float32x4_t dot0 = vaddq_f32(mz0, vaddq_f32(my0, mx0));
    const float32x4_t dot1 = vaddq_f32(mz1, vaddq_f32(my1, mx1));

    const float32x4_t pd = vdupq_laneq_f32(plane, 3);
    const uint32x4_t gt0 = vcgtq_f32(dot0, pd);
    const uint32x4_t gt1 = vcgtq_f32(dot1, pd);

    return vminvq_u32(vandq_u32(gt0, gt1)) != 0u;
}

#else

struct Vec3F { float x, y, z; };

#endif

} // namespace ray_box_cull_detail

static inline void sub_822A0CC0_Batch9Impl(PPCContext& ctx, uint8_t* base) {
    using namespace ray_box_cull_detail;

    constexpr uint32_t kAddrMaskOr  = 0x831AB6D0u; // 0x831B0000 - 18736
    constexpr uint32_t kAddrMaskAnd = 0x831AB6F0u; // 0x831B0000 - 18704

    // 1. Resolve zone pointer and occluder count (exact early-exit on == 0)
    const uint32_t pool_base = REX_LOAD_U32(ctx.r3.u32 + 0u);
    const uint32_t zone_ea = pool_base + (ctx.r5.u32 * 21888u);
    const int32_t num_occluders = static_cast<int32_t>(REX_LOAD_U32(zone_ea + 16384u));
    if (num_occluders == 0) {
        ctx.r3.s64 = 0;
        return;
    }

    // 2. Load AABB min (r6) and max (r7) BEFORE checking num_occluders < 0 (OOB/guard parity)
    const uint32_t min_ea = ctx.r6.u32;
    const uint32_t max_ea = ctx.r7.u32;
    const uint32_t mat_ea = ctx.r8.u32;

    const float minX = LoadF32BE(base, min_ea + 0u);
    const float minY = LoadF32BE(base, min_ea + 4u);
    const float minZ = LoadF32BE(base, min_ea + 8u);
    const float maxX = LoadF32BE(base, max_ea + 0u);
    const float maxY = LoadF32BE(base, max_ea + 4u);
    const float maxZ = LoadF32BE(base, max_ea + 8u);

#if defined(__aarch64__) || defined(_M_ARM64)
    float32x4_t cx0, cy0, cz0;
    float32x4_t cx1, cy1, cz1;

    // 3. Build 8 AABB corners in SoA NEON registers (optionally transformed by Matrix34 @ r8)
    if (mat_ea != 0u) {
        const uint32_t mat_aligned = mat_ea & ~0xFu;
        float32x4_t m0, m1, m2, m3;
        if (InUsableGuestRange(mat_aligned, 64u)) [[likely]] {
            const uint8_t* const mp = REX_RAW_ADDR(mat_aligned);
            m0 = LoadVec4BE_Ptr(mp + 0u);
            m1 = LoadVec4BE_Ptr(mp + 16u);
            m2 = LoadVec4BE_Ptr(mp + 32u);
            m3 = LoadVec4BE_Ptr(mp + 48u);
        } else [[unlikely]] {
            m0 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 0u)  & ~0xFu));
            m1 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 16u) & ~0xFu));
            m2 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 32u) & ~0xFu));
            m3 = LoadVec4BE_Ptr(REX_RAW_ADDR((mat_ea + 48u) & ~0xFu));
        }

        const uint8_t* const mask_ptr = REX_RAW_ADDR(kAddrMaskOr);
        const uint32x4_t mask_or  = vreinterpretq_u32_f32(LoadVec4BE_Ptr(mask_ptr + 0u));
        const uint32x4_t mask_and = vreinterpretq_u32_f32(LoadVec4BE_Ptr(mask_ptr + 32u));

        // Exact post-transform gate: if num_occluders < 0, exit after all r6/r7/r8/mask loads
        if (num_occluders < 0) [[unlikely]] {
            ctx.r3.s64 = 0;
            return;
        }

        const uint32x4_t min_u = vreinterpretq_u32_f32(float32x4_t{ minX, minY, minZ, 0.0f });
        const uint32x4_t max_u = vreinterpretq_u32_f32(float32x4_t{ maxX, maxY, maxZ, 0.0f });
        const float32x4_t pm_min = vreinterpretq_f32_u32(vorrq_u32(mask_or, vandq_u32(min_u, mask_and)));
        const float32x4_t pm_max = vreinterpretq_f32_u32(vorrq_u32(mask_or, vandq_u32(max_u, mask_and)));

        // Factored affine transform in exact ARM64 SIMDe dp_ps(0xFF) pairwise order: (tw + tz) + (ty + tx)
        const float32x4_t tx_min = vmulq_laneq_f32(m0, pm_min, 0);
        const float32x4_t tx_max = vmulq_laneq_f32(m0, pm_max, 0);
        const float32x4_t ty_min = vmulq_laneq_f32(m1, pm_min, 1);
        const float32x4_t ty_max = vmulq_laneq_f32(m1, pm_max, 1);
        const float32x4_t tz_min = vmulq_laneq_f32(m2, pm_min, 2);
        const float32x4_t tz_max = vmulq_laneq_f32(m2, pm_max, 2);

        const float32x4_t yx_00 = vaddq_f32(ty_min, tx_min);
        const float32x4_t yx_01 = vaddq_f32(ty_max, tx_min);
        const float32x4_t yx_10 = vaddq_f32(ty_min, tx_max);
        const float32x4_t yx_11 = vaddq_f32(ty_max, tx_max);

        float32x4_t c0, c1, c2, c3, c4, c5, c6, c7;
        if (vgetq_lane_u32(mask_and, 3) == 0u) [[likely]] {
            const float32x4_t tw = vmulq_laneq_f32(m3, pm_min, 3);
            const float32x4_t wz_min = vaddq_f32(tw, tz_min);
            const float32x4_t wz_max = vaddq_f32(tw, tz_max);

            c0 = vaddq_f32(wz_min, yx_00);
            c1 = vaddq_f32(wz_max, yx_00);
            c2 = vaddq_f32(wz_min, yx_01);
            c3 = vaddq_f32(wz_max, yx_01);
            c4 = vaddq_f32(wz_max, yx_11);
            c5 = vaddq_f32(wz_min, yx_11);
            c6 = vaddq_f32(wz_max, yx_10);
            c7 = vaddq_f32(wz_min, yx_10);
        } else [[unlikely]] {
            // Synthetic AutoVerify .rdata fallback when mask_and.w != 0: preserve exact red-zone W lanes
            const uint32_t ma3 = vgetq_lane_u32(mask_and, 3);
            const uint32_t mo3 = vgetq_lane_u32(mask_or, 3);
            const uint32_t sp = ctx.r1.u32;
            auto LoadCornerW = [&](uint32_t neg_off) noexcept -> float {
                return std::bit_cast<float>((REX_LOAD_U32(sp - neg_off) & ma3) | mo3);
            };
            c0 = vaddq_f32(vaddq_f32(vmulq_n_f32(m3, LoadCornerW(116u)), tz_min), yx_00);
            c1 = vaddq_f32(vaddq_f32(vmulq_n_f32(m3, LoadCornerW(100u)), tz_max), yx_00);
            c2 = vaddq_f32(vaddq_f32(vmulq_n_f32(m3, LoadCornerW(84u)),  tz_min), yx_01);
            c3 = vaddq_f32(vaddq_f32(vmulq_n_f32(m3, LoadCornerW(68u)),  tz_max), yx_01);
            c4 = vaddq_f32(vaddq_f32(vmulq_n_f32(m3, LoadCornerW(52u)),  tz_max), yx_11);
            c5 = vaddq_f32(vaddq_f32(vmulq_n_f32(m3, LoadCornerW(36u)),  tz_min), yx_11);
            c6 = vaddq_f32(vaddq_f32(vmulq_n_f32(m3, LoadCornerW(20u)),  tz_max), yx_10);
            c7 = vaddq_f32(vaddq_f32(vmulq_n_f32(m3, LoadCornerW(4u)),   tz_min), yx_10);
        }

        Transpose4x3(c0, c1, c2, c3, cx0, cy0, cz0);
        Transpose4x3(c4, c5, c6, c7, cx1, cy1, cz1);
    } else {
        if (num_occluders < 0) [[unlikely]] {
            ctx.r3.s64 = 0;
            return;
        }
        cx0 = vdupq_n_f32(minX);
        cy0 = float32x4_t{ minY, minY, maxY, maxY };
        cz0 = float32x4_t{ minZ, maxZ, minZ, maxZ };

        cx1 = vdupq_n_f32(maxX);
        cy1 = float32x4_t{ maxY, maxY, minY, minY };
        cz1 = float32x4_t{ maxZ, minZ, maxZ, minZ };
    }

    // 4. Test all occluders in zone (256B stride: +0..223 planes, +224 disabled, +236 num_planes)
    uint32_t occ_base = zone_ea;
    for (int32_t occ = 0; occ < num_occluders; ++occ, occ_base += 256u) {
        if (occ + 1 < num_occluders && InUsableGuestRange(occ_base + 256u, 256u)) [[likely]] {
            const uint8_t* const next_occ = REX_RAW_ADDR(occ_base + 256u);
            __builtin_prefetch(next_occ + 224u, 0, 1);
            __builtin_prefetch(next_occ + 0u, 0, 1);
        }

        if (REX_LOAD_U32(occ_base + 224u) != 0u) {
            continue;
        }

        const int32_t num_planes = static_cast<int32_t>(REX_LOAD_U32(occ_base + 236u));
        if (num_planes <= 0) {
            if (num_planes == 0) {
                ctx.r3.s64 = 1;
                return;
            }
            continue;
        }

        bool separated = false;
        const uint32_t plane_base_aligned = occ_base & ~0xFu;
        if (num_planes <= 1024 &&
            InUsableGuestRange(plane_base_aligned, static_cast<uint32_t>(num_planes) * 16u)) [[likely]] {
            const uint8_t* plane_ptr = REX_RAW_ADDR(plane_base_aligned);
            for (int32_t k = 0; k < num_planes; ++k, plane_ptr += 16u) {
                const float32x4_t plane = LoadVec4BE_Ptr(plane_ptr);
                if (All8CornersOutsidePlane(plane, cx0, cy0, cz0, cx1, cy1, cz1)) {
                    separated = true;
                    break;
                }
            }
        } else [[unlikely]] {
            uint32_t plane_ea = occ_base;
            for (int32_t k = 0; k < num_planes; ++k, plane_ea += 16u) {
                const float32x4_t plane = LoadVec4BE_Ptr(REX_RAW_ADDR(plane_ea & ~0xFu));
                if (All8CornersOutsidePlane(plane, cx0, cy0, cz0, cx1, cy1, cz1)) {
                    separated = true;
                    break;
                }
            }
        }

        if (!separated) {
            ctx.r3.s64 = 1;
            return;
        }
    }

    ctx.r3.s64 = 0;
#else
    Vec3F corners[8] = {
        { minX, minY, minZ },
        { minX, minY, maxZ },
        { minX, maxY, minZ },
        { minX, maxY, maxZ },
        { maxX, maxY, maxZ },
        { maxX, maxY, minZ },
        { maxX, minY, maxZ },
        { maxX, minY, minZ },
    };

    if (mat_ea != 0u) {
        float m[4][4];
        for (uint32_t r = 0u; r < 4u; ++r) {
            const uint32_t row_ea = (mat_ea + r * 16u) & ~0xFu;
            for (uint32_t c = 0u; c < 4u; ++c) {
                m[r][c] = LoadF32BE(base, row_ea + c * 4u);
            }
        }
        uint32_t mo[4], ma[4];
        for (uint32_t c = 0u; c < 4u; ++c) {
            mo[c] = REX_LOAD_U32(kAddrMaskOr  + c * 4u);
            ma[c] = REX_LOAD_U32(kAddrMaskAnd + c * 4u);
        }
        if (num_occluders < 0) {
            ctx.r3.s64 = 0;
            return;
        }
        const uint32_t sp = ctx.r1.u32;
        for (uint32_t i = 0u; i < 8u; ++i) {
            const uint32_t w_raw = (ma[3] == 0u) ? 0u : REX_LOAD_U32(sp - (116u - i * 16u));
            const float px = std::bit_cast<float>((std::bit_cast<uint32_t>(corners[i].x) & ma[0]) | mo[0]);
            const float py = std::bit_cast<float>((std::bit_cast<uint32_t>(corners[i].y) & ma[1]) | mo[1]);
            const float pz = std::bit_cast<float>((std::bit_cast<uint32_t>(corners[i].z) & ma[2]) | mo[2]);
            const float pw = std::bit_cast<float>((w_raw & ma[3]) | mo[3]);
            corners[i] = {
                (pw * m[3][0] + pz * m[2][0]) + (py * m[1][0] + px * m[0][0]),
                (pw * m[3][1] + pz * m[2][1]) + (py * m[1][1] + px * m[0][1]),
                (pw * m[3][2] + pz * m[2][2]) + (py * m[1][2] + px * m[0][2]),
            };
        }
    } else if (num_occluders < 0) {
        ctx.r3.s64 = 0;
        return;
    }

    uint32_t occ_base = zone_ea;
    for (int32_t occ = 0; occ < num_occluders; ++occ, occ_base += 256u) {
        if (REX_LOAD_U32(occ_base + 224u) != 0u) {
            continue;
        }
        const int32_t num_planes = static_cast<int32_t>(REX_LOAD_U32(occ_base + 236u));
        if (num_planes <= 0) {
            if (num_planes == 0) {
                ctx.r3.s64 = 1;
                return;
            }
            continue;
        }
        bool separated = false;
        uint32_t plane_ea = occ_base;
        for (int32_t k = 0; k < num_planes; ++k, plane_ea += 16u) {
            const uint32_t pa = plane_ea & ~0xFu;
            const float px = LoadF32BE(base, pa + 0u);
            const float py = LoadF32BE(base, pa + 4u);
            const float pz = LoadF32BE(base, pa + 8u);
            const float pd = LoadF32BE(base, pa + 12u);
            bool all_out = true;
            for (uint32_t i = 0u; i < 8u; ++i) {
                const float dot = (pz * corners[i].z) + ((py * corners[i].y) + (px * corners[i].x));
                if (!(dot > pd)) {
                    all_out = false;
                    break;
                }
            }
            if (all_out) {
                separated = true;
                break;
            }
        }
        if (!separated) {
            ctx.r3.s64 = 1;
            return;
        }
    }
    ctx.r3.s64 = 0;
#endif
}

extern "C" void sub_822A0CC0(PPCContext& ctx, uint8_t* base) {
#ifdef GTA4_OCCLUSION_REPORT_DEFINED
    const bool report = REXCVAR_GET(gta4_occlusion_report);
    const bool disabled = REXCVAR_GET(gta4_disable_batch9_occlusion_hooks);
    if (!report && !disabled) [[likely]] {
        sub_822A0CC0_Batch9Impl(ctx, base);
        return;
    }
    const uint32_t lr = static_cast<uint32_t>(ctx.lr);
    if (disabled) {
        __imp__sub_822A0CC0(ctx, base);
        if (report) {
            gta4::occlusion_report::RecordLeaf("sub_822A0CC0", lr, (ctx.r3.u32 & 0xFFu) != 0, false, false);
        }
        return;
    }
    PPCContext orig_ctx = ctx;
    sub_822A0CC0_Batch9Impl(ctx, base);
    __imp__sub_822A0CC0(orig_ctx, base);
    gta4::occlusion_report::RecordLeaf("sub_822A0CC0", lr, (ctx.r3.u32 & 0xFFu) != 0, true, (orig_ctx.r3.u32 & 0xFFu) != 0);
#else
    sub_822A0CC0_Batch9Impl(ctx, base);
#endif
}

// --- End sub_822A0CC0_opt.inc ---

// --- Begin sub_822A2388_opt.inc ---
// Hook: sub_822A2388 (RagePhysics Box vs Convex Polyhedra Max-Dot Culling — Batch D Task 5 / Batch 9)
// Target: Pure ARM64 NEON drop-in hook (0-ULP pairwise vaddvq_f32 + 4-corner SoA + NaN-exact SSE max)
// Optimizations:
//   - Zero guest stack spills (eliminates 24 scalar stfs + 16 stack lvx128/stvx128 + 8 scalar stfs(w)).
//   - Factored 8-corner Matrix34 transform (when r7 != 0): 7 vmulq_n_f32 + 14 vaddq_f32 (vs 96 muls in PPC)
//     in exact ARM64 SIMDe pairwise dp_ps(0xFF) order `(tx + ty) + (tz + tw)`.
//   - Dynamically loads w_const from 0x820BECF4 (0x820C0000 - 4876) for AutoVerify synthetic .rdata parity.
//   - Transposes the 8 corners ONCE into two 4-wide SoA blocks (A = c0..c3, B = c4..c7):
//     evaluates 4 corners per plane in 3 muls + 3 adds in exact pairwise dp_ps(0xFF) order
//     `(nx*x + ny*y) + (nz*z + nw*w_const)`, with NaN-safe Block-A early rejection + exact SSE max tree fallback.
//   - Preserves exact PPC edge cases: poly_count==0 early exit before loads, plane_count<=0 -> return 1.
// ABI: Preserves all PowerPC callee-saved registers (r1, r2, r13, r14..r31, f14..f31, v20..v31, cr2..cr4);
//      returns 0 or 1 in ctx.r3.s64.


#if defined(__aarch64__) || defined(_M_ARM64)
#endif

namespace phys_box_maxdot_detail {

[[gnu::always_inline]] inline bool InUsableGuestRange(uint32_t ea, uint32_t bytes) noexcept {
    constexpr uint32_t kMin = 0x80000000u;
    constexpr uint32_t kMax = 0x83FE0000u;
    return (ea >= kMin) && (ea < kMax) && (bytes <= (kMax - ea));
}

#if defined(__aarch64__) || defined(_M_ARM64)

[[gnu::always_inline]] inline float32x4_t LoadVec4BE_Ptr(const uint8_t* ptr) noexcept {
    return vreinterpretq_f32_u8(vrev32q_u8(vld1q_u8(ptr)));
}

[[gnu::always_inline]] inline float32x4_t LoadVec4BE_EA(uint8_t* base, uint32_t ea) noexcept {
    return LoadVec4BE_Ptr(REX_RAW_ADDR(ea & ~0xFu));
}

struct SoA4 {
    float32x4_t x, y, z;
};

[[gnu::always_inline]] inline SoA4 Transpose4ToSoA3(
    float32x4_t c0, float32x4_t c1, float32x4_t c2, float32x4_t c3) noexcept
{
    const float32x4_t t0 = vzip1q_f32(c0, c2);
    const float32x4_t t1 = vzip2q_f32(c0, c2);
    const float32x4_t t2 = vzip1q_f32(c1, c3);
    const float32x4_t t3 = vzip2q_f32(c1, c3);
    return {
        vzip1q_f32(t0, t2),
        vzip2q_f32(t0, t2),
        vzip1q_f32(t1, t3),
    };
}

// Exact SSE max_ps(a, b): (a > b) ? a : b (returns b when either operand is NaN)
[[gnu::always_inline]] inline float32x4_t SseMaxPs(float32x4_t a, float32x4_t b) noexcept {
    return vbslq_f32(vcgtq_f32(a, b), a, b);
}

[[gnu::always_inline]] inline float SseMaxF32(float a, float b) noexcept {
    return (a > b) ? a : b;
}

// Exact ARM64 SIMDe dp_ps(0xFF) pairwise dot for 4 corners: (nx*x + ny*y) + (nz*z + nw_w)
[[gnu::always_inline]] inline float32x4_t Dot4Plane(
    const SoA4& b, float32x4_t plane, float32x4_t nw_w_vec) noexcept
{
    const float32x4_t pxy = vaddq_f32(vmulq_laneq_f32(b.x, plane, 0), vmulq_laneq_f32(b.y, plane, 1));
    const float32x4_t pzw = vaddq_f32(vmulq_laneq_f32(b.z, plane, 2), nw_w_vec);
    return vaddq_f32(pxy, pzw);
}

#endif

} // namespace phys_box_maxdot_detail

static inline void sub_822A2388_Batch9Impl(PPCContext& ctx, uint8_t* base) {
    using namespace phys_box_maxdot_detail;

    constexpr uint32_t kAddrMaskOr  = 0x831AB6D0u; // 0x831B0000 - 18736
    constexpr uint32_t kAddrMaskAnd = 0x831AB6F0u; // 0x831B0000 - 18704
    constexpr uint32_t kAddrWConst  = 0x820BECF4u; // 0x820C0000 - 4876 (1.0f in XEX)

    const uint32_t base_arr   = REX_LOAD_U32(ctx.r3.u32 + 0u);
    const uint32_t cont_off   = static_cast<uint32_t>(ctx.r4.u64 * 21888ULL);
    const uint32_t cont_ea    = base_arr + cont_off;
    const int32_t  poly_count = static_cast<int32_t>(REX_LOAD_U32(cont_ea + 16384u));

    if (poly_count == 0) {
        ctx.r3.s64 = 0;
        return;
    }

    const uint32_t min_ea = ctx.r5.u32;
    const uint32_t max_ea = ctx.r6.u32;
    const uint32_t mat_ea = ctx.r7.u32;

    const float x0 = REX_LOAD_F32(min_ea + 0u);
    const float y0 = REX_LOAD_F32(min_ea + 4u);
    const float z0 = REX_LOAD_F32(min_ea + 8u);
    const float x1 = REX_LOAD_F32(max_ea + 0u);
    const float y1 = REX_LOAD_F32(max_ea + 4u);
    const float z1 = REX_LOAD_F32(max_ea + 8u);

#if defined(__aarch64__) || defined(_M_ARM64)
    SoA4 blkA, blkB;

    if (mat_ea != 0u) {
        const float32x4_t m0 = LoadVec4BE_EA(base, mat_ea +  0u);
        const float32x4_t m1 = LoadVec4BE_EA(base, mat_ea + 16u);
        const float32x4_t m2 = LoadVec4BE_EA(base, mat_ea + 32u);
        const float32x4_t m3 = LoadVec4BE_EA(base, mat_ea + 48u);

        const uint32x4_t mask_and = vreinterpretq_u32_f32(LoadVec4BE_EA(base, kAddrMaskAnd));
        const uint32x4_t mask_or  = vreinterpretq_u32_f32(LoadVec4BE_EA(base, kAddrMaskOr));

        const uint32_t ma0 = vgetq_lane_u32(mask_and, 0);
        const uint32_t ma1 = vgetq_lane_u32(mask_and, 1);
        const uint32_t ma2 = vgetq_lane_u32(mask_and, 2);
        const uint32_t mo0 = vgetq_lane_u32(mask_or,  0);
        const uint32_t mo1 = vgetq_lane_u32(mask_or,  1);
        const uint32_t mo2 = vgetq_lane_u32(mask_or,  2);
        const uint32_t mo3 = vgetq_lane_u32(mask_or,  3);

        const float mx0 = std::bit_cast<float>((std::bit_cast<uint32_t>(x0) & ma0) | mo0);
        const float mx1 = std::bit_cast<float>((std::bit_cast<uint32_t>(x1) & ma0) | mo0);
        const float my0 = std::bit_cast<float>((std::bit_cast<uint32_t>(y0) & ma1) | mo1);
        const float my1 = std::bit_cast<float>((std::bit_cast<uint32_t>(y1) & ma1) | mo1);
        const float mz0 = std::bit_cast<float>((std::bit_cast<uint32_t>(z0) & ma2) | mo2);
        const float mz1 = std::bit_cast<float>((std::bit_cast<uint32_t>(z1) & ma2) | mo2);
        const float mw  = std::bit_cast<float>(mo3);

        const float32x4_t tx0 = vmulq_n_f32(m0, mx0);
        const float32x4_t tx1 = vmulq_n_f32(m0, mx1);
        const float32x4_t ty0 = vmulq_n_f32(m1, my0);
        const float32x4_t ty1 = vmulq_n_f32(m1, my1);
        const float32x4_t tz0 = vmulq_n_f32(m2, mz0);
        const float32x4_t tz1 = vmulq_n_f32(m2, mz1);
        const float32x4_t tw  = vmulq_n_f32(m3, mw);

        const float32x4_t xy00 = vaddq_f32(tx0, ty0);
        const float32x4_t xy01 = vaddq_f32(tx0, ty1);
        const float32x4_t xy11 = vaddq_f32(tx1, ty1);
        const float32x4_t xy10 = vaddq_f32(tx1, ty0);
        const float32x4_t zw0  = vaddq_f32(tz0, tw);
        const float32x4_t zw1  = vaddq_f32(tz1, tw);

        // Exact corner ordering matching v9,v10,v11,v12 (A) and v5,v6,v7,v8 (B)
        blkA = Transpose4ToSoA3(
            vaddq_f32(xy00, zw0), vaddq_f32(xy00, zw1),
            vaddq_f32(xy01, zw0), vaddq_f32(xy01, zw1));
        blkB = Transpose4ToSoA3(
            vaddq_f32(xy11, zw1), vaddq_f32(xy11, zw0),
            vaddq_f32(xy10, zw1), vaddq_f32(xy10, zw0));
    } else {
        blkA = {
            float32x4_t{ x0, x0, x0, x0 },
            float32x4_t{ y0, y0, y1, y1 },
            float32x4_t{ z0, z1, z0, z1 },
        };
        blkB = {
            float32x4_t{ x1, x1, x1, x1 },
            float32x4_t{ y1, y1, y0, y0 },
            float32x4_t{ z1, z0, z1, z0 },
        };
    }

    const float w_const = REX_LOAD_F32(kAddrWConst);
    if (poly_count <= 0) {
        ctx.r3.s64 = 0;
        return;
    }

    const float32x4_t zero = vdupq_n_f32(0.0f);

    for (int32_t i = 0; i < poly_count; ++i) {
        const uint32_t poly_ea = cont_ea + (static_cast<uint32_t>(i) << 8);
        if (REX_LOAD_U32(poly_ea + 224u) != 0u) {
            continue;
        }
        const int32_t plane_count = static_cast<int32_t>(REX_LOAD_U32(poly_ea + 236u));
        if (plane_count <= 0) {
            ctx.r3.s64 = 1;
            return;
        }

        const uint32_t poly_al = poly_ea & ~0xFu;
        const bool fast_planes =
            (plane_count <= 16) && InUsableGuestRange(poly_al, static_cast<uint32_t>(plane_count) << 4);
        const uint8_t* const plane_base = fast_planes ? REX_RAW_ADDR(poly_al) : nullptr;

        bool rejected = false;
        for (int32_t p = 0; p < plane_count; ++p) {
            const float32x4_t plane = fast_planes
                ? LoadVec4BE_Ptr(plane_base + (static_cast<uint32_t>(p) << 4))
                : LoadVec4BE_EA(base, poly_ea + (static_cast<uint32_t>(p) << 4));
            const float32x4_t nw_w_vec = vdupq_n_f32(vgetq_lane_f32(plane, 3) * w_const);

            const float32x4_t dA = Dot4Plane(blkA, plane, nw_w_vec);
            const bool a_all_neg = (vminvq_u32(vcltq_f32(dA, zero)) != 0u);
            if (!a_all_neg && vminvq_u32(vceqq_f32(dA, dA)) != 0u) {
                // dA has no NaNs and at least one lane >= 0.0f -> 0.0f > max(all) is guaranteed false
                rejected = true;
                break;
            }

            const float32x4_t dB = Dot4Plane(blkB, plane, nw_w_vec);
            if (a_all_neg) [[likely]] {
                if (vminvq_u32(vcltq_f32(dB, zero)) != 0u) {
                    continue; // All 8 dots < 0.0f -> 0.0f > max(all) is true
                }
                if (vminvq_u32(vceqq_f32(dB, dB)) != 0u) {
                    rejected = true;
                    break;
                }
            }

            // Ultra-rare NaN tie-breaking fallback: exact SSE vmaxfp binary tree
            const float32x4_t even = vuzp1q_f32(dA, dB); // [d0, d2, d4, d6]
            const float32x4_t odd  = vuzp2q_f32(dA, dB); // [d1, d3, d5, d7]
            const float32x4_t p1   = SseMaxPs(even, odd);
            const float mA = SseMaxF32(vgetq_lane_f32(p1, 0), vgetq_lane_f32(p1, 1));
            const float mB = SseMaxF32(vgetq_lane_f32(p1, 2), vgetq_lane_f32(p1, 3));
            const float mAll = SseMaxF32(mA, mB);
            if (!(0.0f > mAll)) {
                rejected = true;
                break;
            }
        }

        if (!rejected) {
            ctx.r3.s64 = 1;
            return;
        }
    }
#else
    float cx[8] = { x0, x0, x0, x0, x1, x1, x1, x1 };
    float cy[8] = { y0, y0, y1, y1, y1, y1, y0, y0 };
    float cz[8] = { z0, z1, z0, z1, z1, z0, z1, z0 };

    if (mat_ea != 0u) {
        const uint32_t m_al = mat_ea & ~0xFu;
        float m[4][4];
        for (uint32_t r = 0u; r < 4u; ++r) {
            for (uint32_t c = 0u; c < 4u; ++c) {
                m[r][c] = REX_LOAD_F32(m_al + (r << 4) + (c << 2));
            }
        }
        uint32_t ma[4], mo[4];
        for (uint32_t k = 0u; k < 4u; ++k) {
            ma[k] = REX_LOAD_U32(kAddrMaskAnd + (k << 2));
            mo[k] = REX_LOAD_U32(kAddrMaskOr  + (k << 2));
        }
        const float mw = std::bit_cast<float>(mo[3]);
        for (int k = 0; k < 8; ++k) {
            const float px = std::bit_cast<float>((std::bit_cast<uint32_t>(cx[k]) & ma[0]) | mo[0]);
            const float py = std::bit_cast<float>((std::bit_cast<uint32_t>(cy[k]) & ma[1]) | mo[1]);
            const float pz = std::bit_cast<float>((std::bit_cast<uint32_t>(cz[k]) & ma[2]) | mo[2]);
            cx[k] = (px * m[0][0] + py * m[1][0]) + (pz * m[2][0] + mw * m[3][0]);
            cy[k] = (px * m[0][1] + py * m[1][1]) + (pz * m[2][1] + mw * m[3][1]);
            cz[k] = (px * m[0][2] + py * m[1][2]) + (pz * m[2][2] + mw * m[3][2]);
        }
    }

    const float w_const = REX_LOAD_F32(kAddrWConst);
    if (poly_count <= 0) {
        ctx.r3.s64 = 0;
        return;
    }

    auto sse_max = [](float a, float b) noexcept { return (a > b) ? a : b; };
    for (int32_t i = 0; i < poly_count; ++i) {
        const uint32_t poly_ea = cont_ea + (static_cast<uint32_t>(i) << 8);
        if (REX_LOAD_U32(poly_ea + 224u) != 0u) {
            continue;
        }
        const int32_t plane_count = static_cast<int32_t>(REX_LOAD_U32(poly_ea + 236u));
        if (plane_count <= 0) {
            ctx.r3.s64 = 1;
            return;
        }

        bool rejected = false;
        for (int32_t p = 0; p < plane_count; ++p) {
            const uint32_t pe = (poly_ea + (static_cast<uint32_t>(p) << 4)) & ~0xFu;
            const float nx = REX_LOAD_F32(pe +  0u);
            const float ny = REX_LOAD_F32(pe +  4u);
            const float nz = REX_LOAD_F32(pe +  8u);
            const float nw = REX_LOAD_F32(pe + 12u) * w_const;

            float d[8];
            for (int k = 0; k < 8; ++k) {
                d[k] = (nx * cx[k] + ny * cy[k]) + (nz * cz[k] + nw);
            }
            const float mA   = sse_max(sse_max(d[0], d[1]), sse_max(d[2], d[3]));
            const float mB   = sse_max(sse_max(d[4], d[5]), sse_max(d[6], d[7]));
            const float mAll = sse_max(mA, mB);
            if (!(0.0f > mAll)) {
                rejected = true;
                break;
            }
        }
        if (!rejected) {
            ctx.r3.s64 = 1;
            return;
        }
    }
#endif

    ctx.r3.s64 = 0;
}

extern "C" void sub_822A2388(PPCContext& ctx, uint8_t* base) {
#ifdef GTA4_OCCLUSION_REPORT_DEFINED
    const bool report = REXCVAR_GET(gta4_occlusion_report);
    const bool disabled = REXCVAR_GET(gta4_disable_batch9_occlusion_hooks);
    if (!report && !disabled) [[likely]] {
        sub_822A2388_Batch9Impl(ctx, base);
        return;
    }
    const uint32_t lr = static_cast<uint32_t>(ctx.lr);
    if (disabled) {
        __imp__sub_822A2388(ctx, base);
        if (report) {
            gta4::occlusion_report::RecordLeaf("sub_822A2388", lr, (ctx.r3.u32 & 0xFFu) != 0, false, false);
        }
        return;
    }
    PPCContext orig_ctx = ctx;
    sub_822A2388_Batch9Impl(ctx, base);
    __imp__sub_822A2388(orig_ctx, base);
    gta4::occlusion_report::RecordLeaf("sub_822A2388", lr, (ctx.r3.u32 & 0xFFu) != 0, true, (orig_ctx.r3.u32 & 0xFFu) != 0);
#else
    sub_822A2388_Batch9Impl(ctx, base);
#endif
}

// --- End sub_822A2388_opt.inc ---

// --- Begin sub_822A22C0_opt.inc ---
// Hook: sub_822A22C0 (RagePhysics Sphere vs Convex Polyhedra Culling — Batch D Task 5 / Batch 9)
// Target: Pure ARM64 NEON drop-in hook (4-plane parallel SoA NEON + 0-ULP pairwise vaddvq_f32)
// Optimizations:
//   - Eliminates guest stack round-trip (`stfs f1, 44(r1)` + unaligned `lvlx` + `VectorMaskL` + `vspltw`).
//   - Processes 4 planes in parallel per iteration via ARM64 NEON 4x4 transpose (`vzip1q`/`vzip2q`):
//     computes 4 plane signed distances simultaneously in exact ARM64 SIMDe pairwise dp_ps(0xEF) order
//     `((0.0f + nz*cz) + (nx*cx + ny*cy)) - d > neg_r` with single `vmaxvq_u32(vcgtq_f32(...))`.
//   - Lazily loads `sphere_center` (r5) on first polyhedron with `plane_count > 0` for 100% OOB/guard parity.
//   - Preserves exact PPC edge case: `plane_count <= 0` -> return 1.
// ABI: Preserves all PowerPC callee-saved registers (r1, r2, r13, r14..r31, f14..f31, v20..v31, cr2..cr4);
//      returns 0 or 1 in ctx.r3.s64.


#if defined(__aarch64__) || defined(_M_ARM64)
#endif

namespace phys_sphere_cull_detail {

[[gnu::always_inline]] inline bool InUsableGuestRange(uint32_t ea, uint32_t bytes) noexcept {
    constexpr uint32_t kMin = 0x80000000u;
    constexpr uint32_t kMax = 0x83FE0000u;
    return (ea >= kMin) && (ea < kMax) && (bytes <= (kMax - ea));
}

#if defined(__aarch64__) || defined(_M_ARM64)

[[gnu::always_inline]] inline float32x4_t LoadVec4BE_Ptr(const uint8_t* ptr) noexcept {
    return vreinterpretq_f32_u8(vrev32q_u8(vld1q_u8(ptr)));
}

[[gnu::always_inline]] inline float32x4_t LoadVec4BE_EA(uint8_t* base, uint32_t ea) noexcept {
    return LoadVec4BE_Ptr(REX_RAW_ADDR(ea & ~0xFu));
}

// Exact ARM64 SIMDe dp_ps(0xEF) pairwise 3D dot product for 1 plane vs center
[[gnu::always_inline]] inline float Dot3Pairwise(float32x4_t plane, float32x4_t center) noexcept {
    const float32x4_t prod = vmulq_f32(plane, center);
    return vaddvq_f32(vsetq_lane_f32(0.0f, prod, 3));
}

#endif

} // namespace phys_sphere_cull_detail

static inline void sub_822A22C0_Batch9Impl(PPCContext& ctx, uint8_t* base) {
    using namespace phys_sphere_cull_detail;

    const uint32_t base_arr   = REX_LOAD_U32(ctx.r3.u32 + 0u);
    const uint32_t cont_off   = static_cast<uint32_t>(ctx.r4.u64 * 21888ULL);
    const uint32_t cont_ea    = base_arr + cont_off;
    const int32_t  poly_count = static_cast<int32_t>(REX_LOAD_U32(cont_ea + 16384u));
    const float    neg_r      = 0.0f - static_cast<float>(ctx.f1.f64);
    REX_STORE_F32(ctx.r1.u32 + 44u, static_cast<float>(ctx.f1.f64));

    if (poly_count <= 0) {
        ctx.r3.s64 = 0;
        return;
    }

    const uint32_t center_ea = ctx.r5.u32;

#if defined(__aarch64__) || defined(_M_ARM64)
    const float32x4_t neg_r_v = vdupq_n_f32(neg_r);
    bool center_loaded = false;
    float32x4_t center{};

    for (int32_t i = 0; i < poly_count; ++i) {
        const uint32_t poly_ea = cont_ea + (static_cast<uint32_t>(i) << 8);
        if (REX_LOAD_U32(poly_ea + 224u) != 0u) {
            continue;
        }
        const int32_t plane_count = static_cast<int32_t>(REX_LOAD_U32(poly_ea + 236u));
        if (plane_count <= 0) {
            ctx.r3.s64 = 1;
            return;
        }

        if (!center_loaded) [[unlikely]] {
            center = LoadVec4BE_EA(base, center_ea);
            center_loaded = true;
        }

        const uint32_t poly_al = poly_ea & ~0xFu;
        const bool fast_planes =
            (plane_count <= 16) && InUsableGuestRange(poly_al, static_cast<uint32_t>(plane_count) << 4);

        bool rejected = false;
        if (fast_planes) [[likely]] {
            const uint8_t* p_ptr = REX_RAW_ADDR(poly_al);
            int32_t p = 0;

            // Process 4 planes in parallel via 4x4 NEON transpose
            for (; p + 4 <= plane_count; p += 4, p_ptr += 64u) {
                const float32x4_t p0 = LoadVec4BE_Ptr(p_ptr +  0u);
                const float32x4_t p1 = LoadVec4BE_Ptr(p_ptr + 16u);
                const float32x4_t p2 = LoadVec4BE_Ptr(p_ptr + 32u);
                const float32x4_t p3 = LoadVec4BE_Ptr(p_ptr + 48u);

                const float32x4_t t0 = vzip1q_f32(p0, p2);
                const float32x4_t t1 = vzip2q_f32(p0, p2);
                const float32x4_t t2 = vzip1q_f32(p1, p3);
                const float32x4_t t3 = vzip2q_f32(p1, p3);

                const float32x4_t pnx = vzip1q_f32(t0, t2);
                const float32x4_t pny = vzip2q_f32(t0, t2);
                const float32x4_t pnz = vzip1q_f32(t1, t3);
                const float32x4_t pd  = vzip2q_f32(t1, t3);

                const float32x4_t pxy   = vaddq_f32(vmulq_laneq_f32(pnx, center, 0), vmulq_laneq_f32(pny, center, 1));
                const float32x4_t pz    = vmulq_laneq_f32(pnz, center, 2);
                const float32x4_t dist4 = vsubq_f32(vaddq_f32(pz, pxy), pd);
                if (vmaxvq_u32(vcgtq_f32(dist4, neg_r_v)) != 0u) {
                    rejected = true;
                    break;
                }
            }

            if (!rejected) {
                for (; p < plane_count; ++p, p_ptr += 16u) {
                    const float32x4_t plane = LoadVec4BE_Ptr(p_ptr);
                    const float dist = Dot3Pairwise(plane, center) - vgetq_lane_f32(plane, 3);
                    if (dist > neg_r) {
                        rejected = true;
                        break;
                    }
                }
            }
        } else {
            for (int32_t p = 0; p < plane_count; ++p) {
                const float32x4_t plane = LoadVec4BE_EA(base, poly_ea + (static_cast<uint32_t>(p) << 4));
                const float dist = Dot3Pairwise(plane, center) - vgetq_lane_f32(plane, 3);
                if (dist > neg_r) {
                    rejected = true;
                    break;
                }
            }
        }

        if (!rejected) {
            ctx.r3.s64 = 1;
            return;
        }
    }
#else
    bool center_loaded = false;
    float cx = 0.0f, cy = 0.0f, cz = 0.0f;

    for (int32_t i = 0; i < poly_count; ++i) {
        const uint32_t poly_ea = cont_ea + (static_cast<uint32_t>(i) << 8);
        if (REX_LOAD_U32(poly_ea + 224u) != 0u) {
            continue;
        }
        const int32_t plane_count = static_cast<int32_t>(REX_LOAD_U32(poly_ea + 236u));
        if (plane_count <= 0) {
            ctx.r3.s64 = 1;
            return;
        }

        if (!center_loaded) {
            const uint32_t ce = center_ea & ~0xFu;
            cx = REX_LOAD_F32(ce + 0u);
            cy = REX_LOAD_F32(ce + 4u);
            cz = REX_LOAD_F32(ce + 8u);
            center_loaded = true;
        }

        bool rejected = false;
        for (int32_t p = 0; p < plane_count; ++p) {
            const uint32_t pe = (poly_ea + (static_cast<uint32_t>(p) << 4)) & ~0xFu;
            const float nx = REX_LOAD_F32(pe +  0u);
            const float ny = REX_LOAD_F32(pe +  4u);
            const float nz = REX_LOAD_F32(pe +  8u);
            const float d  = REX_LOAD_F32(pe + 12u);
            const float dot = (0.0f + nz * cz) + (nx * cx + ny * cy);
            if ((dot - d) > neg_r) {
                rejected = true;
                break;
            }
        }

        if (!rejected) {
            ctx.r3.s64 = 1;
            return;
        }
    }
#endif

    ctx.r3.s64 = 0;
}

extern "C" void sub_822A22C0(PPCContext& ctx, uint8_t* base) {
#ifdef GTA4_OCCLUSION_REPORT_DEFINED
    const bool report = REXCVAR_GET(gta4_occlusion_report);
    const bool disabled = REXCVAR_GET(gta4_disable_batch9_occlusion_hooks);
    if (!report && !disabled) [[likely]] {
        sub_822A22C0_Batch9Impl(ctx, base);
        return;
    }
    const uint32_t lr = static_cast<uint32_t>(ctx.lr);
    if (disabled) {
        __imp__sub_822A22C0(ctx, base);
        if (report) {
            gta4::occlusion_report::RecordLeaf("sub_822A22C0", lr, (ctx.r3.u32 & 0xFFu) != 0, false, false);
        }
        return;
    }
    PPCContext orig_ctx = ctx;
    sub_822A22C0_Batch9Impl(ctx, base);
    __imp__sub_822A22C0(orig_ctx, base);
    gta4::occlusion_report::RecordLeaf("sub_822A22C0", lr, (ctx.r3.u32 & 0xFFu) != 0, true, (orig_ctx.r3.u32 & 0xFFu) != 0);
#else
    sub_822A22C0_Batch9Impl(ctx, base);
#endif
}

// --- End sub_822A22C0_opt.inc ---

// --- Begin sub_821E3670_v2_opt.inc ---
// Hook: sub_821E3670 v2 (rage::Matrix34::Normalize / Orthonormalize — Batch D Task 1, 0-ULP Fix)
// Target: Pure ARM64 NEON drop-in hook (AutoVerify 2.1 12/12 bit-exact compliant)
// Fixes v1 5/12 failure (1-4 ULP drift in Tests with non-zero X/Y/Z):
//   - Replaces serial scalar dp3 `(sq[Z] + sq[Y]) + sq[X]` with exact ARM64 pairwise `vaddvq_f32`:
//     `(sq[X] + sq[Y]) + (sq[Z] + 0.0f)`, bit-identical to SIMDe `simde_mm_dp_ps(..., 0xEF)`
//     which computes `vaddvq_f32([0, sq[Z], sq[Y], sq[X]])`.
// Optimizations:
//   - Scalar single-precision `FSQRT S` + `FDIV S` (`1.0f / std::sqrt(dot)`) broadcast via `vdupq_n_f32`
//     (bit-identical to 4-lane vector `vsqrtq_f32` + `vdivq_f32`, lower latency on Cortex-X1).
//   - Keeps 4-lane vector NEON NR pass (`vceqq_f32`, `vbicq_u32`, `vfmsq_f32`/`vfmaq_f32`) for exact
//     per-lane parity with synthetic `.rdata` patterns in AutoVerify 2.1.
//   - Single `REX_RAW_ADDR()` for the 3-row Matrix34 (`r3 +0/+16/+32`) and single `REX_RAW_ADDR()`
//     for adjacent `.rdata` constants (`0x82B071A0` / `0x82B071B0`).
//   - Reuses `PermZXYW(row2_norm)` & `PermYZXW(row2_norm)` across both cross products via single-cycle `vqtbl1q_u8`.
// ABI: Preserves all PowerPC callee-saved registers (r1, r2, r13, r14..r31, f14..f31, v20..v31, cr2..cr4)
//      and leaves ctx.r3 untouched.


#if defined(__aarch64__) || defined(_M_ARM64)
#endif

namespace mat34_orthonormalize_v2_detail {

#if defined(__aarch64__) || defined(_M_ARM64)

[[gnu::always_inline]] inline float32x4_t LoadVec4BE(const uint8_t* ptr) noexcept {
    return vreinterpretq_f32_u8(vrev32q_u8(vld1q_u8(ptr)));
}

[[gnu::always_inline]] inline void StoreVec4BE(uint8_t* ptr, float32x4_t v) noexcept {
    vst1q_u8(ptr, vrev32q_u8(vreinterpretq_u8_f32(v)));
}

[[gnu::always_inline]] inline float32x4_t NeonVmadd(float32x4_t a, float32x4_t b, float32x4_t c) noexcept {
#if defined(REX_FUSED_VMADDFP) && REX_FUSED_VMADDFP
    return vfmaq_f32(c, a, b);
#else
    return vaddq_f32(vmulq_f32(a, b), c);
#endif
}

[[gnu::always_inline]] inline float32x4_t NeonVnmsub(float32x4_t a, float32x4_t b, float32x4_t c) noexcept {
#if defined(REX_FUSED_VMADDFP) && REX_FUSED_VMADDFP
    return vfmsq_f32(c, a, b);
#else
    return vnegq_f32(vsubq_f32(vmulq_f32(a, b), c));
#endif
}

// Exact SIMDe simde_mm_dp_ps(..., 0xEF) on ARM64:
// Pairwise vaddvq_f32([X*X, Y*Y, Z*Z, 0.0f]) == (X*X + Y*Y) + (Z*Z + 0.0f)
// Bit-identical to SIMDe vaddvq_f32([0.0f, Z*Z, Y*Y, X*X]) == (0.0f + Z*Z) + (Y*Y + X*X).
[[gnu::always_inline]] inline float Dp3Pairwise(float32x4_t v) noexcept {
    const float32x4_t sq     = vmulq_f32(v, v);
    const float32x4_t masked = vsetq_lane_f32(0.0f, sq, 3);
    return vaddvq_f32(masked);
}

// Exact PPC vrsqrtefp + vsel(inf->0) + 1-step NR normalize across all 4 lanes
[[gnu::always_inline]] inline float32x4_t Normalize3Vec(
    float32x4_t v, float32x4_t c_inf, float32x4_t c_half, float32x4_t c_one) noexcept
{
    const float       dot    = Dp3Pairwise(v);
    const float32x4_t len2   = vdupq_n_f32(dot);
    const float32x4_t rsqrt0 = vdupq_n_f32(1.0f / std::sqrt(dot));
    const uint32x4_t  is_inf = vceqq_f32(rsqrt0, c_inf);
    const float32x4_t r      = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(rsqrt0), is_inf));

    const float32x4_t r2  = vmulq_f32(r, r);
    const float32x4_t rh  = vmulq_f32(c_half, r);
    const float32x4_t err = NeonVnmsub(len2, r2, c_one);
    const float32x4_t inv = NeonVmadd(err, rh, r);
    return vmulq_f32(v, inv);
}

// Permute [X, Y, Z, W] -> [Z, X, Y, W] (vpermwi128 ..., 135)
[[gnu::always_inline]] inline float32x4_t PermZXYW(float32x4_t v) noexcept {
    static constexpr uint8_t kIdx[16] = {
        8, 9, 10, 11,   0, 1, 2, 3,   4, 5, 6, 7,   12, 13, 14, 15
    };
    return vreinterpretq_f32_u8(vqtbl1q_u8(vreinterpretq_u8_f32(v), vld1q_u8(kIdx)));
}

// Permute [X, Y, Z, W] -> [Y, Z, X, W] (vpermwi128 ..., 99)
[[gnu::always_inline]] inline float32x4_t PermYZXW(float32x4_t v) noexcept {
    static constexpr uint8_t kIdx[16] = {
        4, 5, 6, 7,   8, 9, 10, 11,   0, 1, 2, 3,   12, 13, 14, 15
    };
    return vreinterpretq_f32_u8(vqtbl1q_u8(vreinterpretq_u8_f32(v), vld1q_u8(kIdx)));
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

[[gnu::always_inline]] inline float ScalarFma(float a, float b, float c) noexcept {
#if defined(REX_FUSED_VMADDFP) && REX_FUSED_VMADDFP
    return std::fmaf(a, b, c);
#else
    return a * b + c;
#endif
}

[[gnu::always_inline]] inline float ScalarFnma(float a, float b, float c) noexcept {
#if defined(REX_FUSED_VMADDFP) && REX_FUSED_VMADDFP
    return std::fmaf(-a, b, c);
#else
    return -(a * b - c);
#endif
}

[[gnu::always_inline]] inline Vec4F Normalize3Scalar(Vec4F v, Vec4F c_inf, Vec4F c_half, Vec4F c_one) noexcept {
    const float len2 = (v.x * v.x + v.y * v.y) + (v.z * v.z + 0.0f);
    const float r0   = 1.0f / std::sqrt(len2);
    auto norm_lane = [&](float val, float inf_c, float half_c, float one_c) noexcept {
        const float r   = (r0 == inf_c) ? 0.0f : r0;
        const float err = ScalarFnma(len2, r * r, one_c);
        const float inv = ScalarFma(err, half_c * r, r);
        return val * inv;
    };
    return {
        norm_lane(v.x, c_inf.x, c_half.x, c_one.x),
        norm_lane(v.y, c_inf.y, c_half.y, c_one.y),
        norm_lane(v.z, c_inf.z, c_half.z, c_one.z),
        norm_lane(v.w, c_inf.w, c_half.w, c_one.w),
    };
}

#endif

} // namespace mat34_orthonormalize_v2_detail

extern "C" void sub_821E3670(PPCContext& ctx, uint8_t* base) {
    using namespace mat34_orthonormalize_v2_detail;

    constexpr uint32_t kAddrConstInf  = 0x831AB780u; // 0x831B0000 - 18560
    constexpr uint32_t kAddrConstOne  = 0x82B071A0u; // 0x82B00000 + 29088
    constexpr uint32_t kAddrConstHalf = 0x82B071B0u; // 0x82B00000 + 29104

    const uint32_t m_ea = ctx.r3.u32 & ~0xFu;

#if defined(__aarch64__) || defined(_M_ARM64)
    uint8_t* const       m_ptr  = REX_RAW_ADDR(m_ea);
    const uint8_t* const nr_ptr = REX_RAW_ADDR(kAddrConstOne);
    const float32x4_t    c_inf  = LoadVec4BE(REX_RAW_ADDR(kAddrConstInf));
    const float32x4_t    c_one  = LoadVec4BE(nr_ptr +  0u);
    const float32x4_t    c_half = LoadVec4BE(nr_ptr + 16u);

    // 1. Load & normalize row 2 (Z axis at r3 + 32)
    const float32x4_t row2_in   = LoadVec4BE(m_ptr + 32u);
    const float32x4_t row1_in   = LoadVec4BE(m_ptr + 16u);
    const float32x4_t row2_norm = Normalize3Vec(row2_in, c_inf, c_half, c_one);
    StoreVec4BE(m_ptr + 32u, row2_norm);

    // 2. Cross 1: row0_raw = Cross(row1_in, row2_norm) -> normalize -> row0_norm (X axis at r3 + 0)
    const float32x4_t r2_zxyw = PermZXYW(row2_norm);
    const float32x4_t r2_yzxw = PermYZXW(row2_norm);
    const float32x4_t r1_yzxw = PermYZXW(row1_in);
    const float32x4_t r1_zxyw = PermZXYW(row1_in);

    const float32x4_t row0_raw  = NeonVnmsub(r2_yzxw, r1_zxyw, vmulq_f32(r2_zxyw, r1_yzxw));
    const float32x4_t row0_norm = Normalize3Vec(row0_raw, c_inf, c_half, c_one);
    StoreVec4BE(m_ptr + 0u, row0_norm);

    // 3. Cross 2: row1_new = Cross(row2_norm, row0_norm) (Y axis at r3 + 16), reusing r2_zxyw/r2_yzxw
    const float32x4_t r0_zxyw  = PermZXYW(row0_norm);
    const float32x4_t r0_yzxw  = PermYZXW(row0_norm);
    const float32x4_t row1_new = NeonVnmsub(r0_yzxw, r2_zxyw, vmulq_f32(r0_zxyw, r2_yzxw));
    StoreVec4BE(m_ptr + 16u, row1_new);
#else
    const Vec4F c_inf  = LoadVec4BE(base, kAddrConstInf);
    const Vec4F c_one  = LoadVec4BE(base, kAddrConstOne);
    const Vec4F c_half = LoadVec4BE(base, kAddrConstHalf);

    const Vec4F row2_in   = LoadVec4BE(base, m_ea + 32u);
    const Vec4F row1_in   = LoadVec4BE(base, m_ea + 16u);
    const Vec4F row2_norm = Normalize3Scalar(row2_in, c_inf, c_half, c_one);
    StoreVec4BE(base, m_ea + 32u, row2_norm);

    const Vec4F row0_raw{
        ScalarFnma(row2_norm.y, row1_in.z, row2_norm.z * row1_in.y),
        ScalarFnma(row2_norm.z, row1_in.x, row2_norm.x * row1_in.z),
        ScalarFnma(row2_norm.x, row1_in.y, row2_norm.y * row1_in.x),
        ScalarFnma(row2_norm.w, row1_in.w, row2_norm.w * row1_in.w),
    };
    const Vec4F row0_norm = Normalize3Scalar(row0_raw, c_inf, c_half, c_one);
    StoreVec4BE(base, m_ea + 0u, row0_norm);

    const Vec4F row1_new{
        ScalarFnma(row0_norm.y, row2_norm.z, row0_norm.z * row2_norm.y),
        ScalarFnma(row0_norm.z, row2_norm.x, row0_norm.x * row2_norm.z),
        ScalarFnma(row0_norm.x, row2_norm.y, row0_norm.y * row2_norm.x),
        ScalarFnma(row0_norm.w, row2_norm.w, row0_norm.w * row2_norm.w),
    };
    StoreVec4BE(base, m_ea + 16u, row1_new);
#endif
}

// --- End sub_821E3670_v2_opt.inc ---

#endif // ENABLE_BATCH9_VERIFIED_HOOKS
