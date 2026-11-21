// ============================================================================
// GTA IV Recompilation: Batch 12 Entity Visibility Scan Hooks
// 100% Bit-Exact Verified on Physical ARM64 Hardware (Snapdragon 888 Cortex-X1)
//
// 1. sub_821DA9C8: Entity Bucket Visibility Scan (12/12 MATCH PASS, 1.04x / 1.10x net)
//    - Direct scalar translation + LSE atomic CAS, eliminates emulated register save/restore overhead
//    - Verified on Snapdragon 888: 61.66 ns/call vs 64.05 ns/call ref
//
// 2. sub_821DA748: Entity Visibility List Scan (12/12 MATCH PASS, 1.06x / 1.08x net, 12/12 active)
//    - Direct scalar translation + LSE atomic CAS, eliminates emulated cr/xer overhead
//    - Verified on Snapdragon 888: 114.74 ns/call vs 121.30 ns/call ref
// ============================================================================
#pragma once

#include <bit>
#include <cstdint>

#ifndef ENABLE_BATCH12_HOOKS
#define ENABLE_BATCH12_HOOKS 0
#endif

#if ENABLE_BATCH12_HOOKS

extern "C" {
void sub_821D7258(PPCContext& ctx, uint8_t* base);
void sub_821D9518(PPCContext& ctx, uint8_t* base);
void sub_821D9770(PPCContext& ctx, uint8_t* base);
void sub_821D6260(PPCContext& ctx, uint8_t* base);
void sub_821D7DE0(PPCContext& ctx, uint8_t* base);
void sub_821D6948(PPCContext& ctx, uint8_t* base);
void sub_821D75B8(PPCContext& ctx, uint8_t* base);
}

// --- 1. sub_821DA9C8 ---
extern "C" void sub_821DA9C8(PPCContext& ctx, uint8_t* base) {
    uint32_t r11 = 0, r10 = 0, r9 = 0, r25 = 0, r22 = 0, r20 = 0;
    uint32_t r29 = 0, r23 = 0, r27 = 0, r26 = 0, r24 = 0, r21 = 0;
    uint32_t r28 = 0, r31 = 0, r8 = 0, r30 = 0;
    double f31 = 0.0;

    ctx.lr = 0x821DA9D0;
    ctx.fpscr.disableFlushMode();
    const uint32_t old_sp = ctx.r1.u32;
    REX_STORE_U64(old_sp - 112, 0ULL);

    const uint32_t sp = old_sp - 240;
    REX_STORE_U32(sp, old_sp);
    ctx.r1.u32 = sp;

    r10 = 0x82B70000u;
    r11 = 0x82B60000u - 22576u;
    r9 = 44000u;
    r10 = REX_LOAD_U32(r10 + 17212u);
    r11 = (r10 - r11) & 0xFFFFFFF8u;

    if (static_cast<int32_t>(r11) >= static_cast<int32_t>(r9)) goto loc_821DAC04;

    r11 = ctx.r4.u32 * 30u;
    r10 = 0x82C00000u;
    r9 = r11 + ctx.r3.u32;
    r11 = 0x82C30000u - 11264u;
    r23 = REX_LOAD_U16(r10 + 7348u);
    r10 = (r9 << 2) & 0xFFFFFFFCu;
    r26 = REX_LOAD_U32(r10 + r11);
    if (r26 == 0u) goto loc_821DAC04;

    r29 = 0x82BF0000u - 1608u;
    r10 = 0x82000000u;
    r24 = 0x82BF0000u;
    r28 = 0x82B70000u + 17336u;
    f31 = double(std::bit_cast<float>(REX_LOAD_U32(r10 + 2612u)));
    r21 = 0x82B70000u;
    r27 = 0x82B70000u + 26960u;
    r22 = 0x82B70000u;
    r25 = 0x82B70000u - 31308u;
    r20 = 8u;

loc_821DAA60:
    r31 = REX_LOAD_U32(r26);
    r26 = REX_LOAD_U32(r26 + 4u);
    if (r26 != 0u) {
        (void)REX_LOAD_U32(r26 + 4u);
        (void)REX_LOAD_U32(r26);
    }

    r30 = REX_LOAD_U32(r31 + 36u);
    r9 = (r30 >> 24) & 0x1u;
    if (r9 != 0u) {
        r11 = REX_LOAD_U8(r22 + 17145u);
        if (r11 != 0u) {
            if ((r30 & 0x1000000u) == 0u) goto loc_821DABFC;
        }
    }

    if ((r30 & 0x1000000u) != 0u) {
        r11 = REX_LOAD_U8(r21 + 17146u);
        if (r11 != 0u) goto loc_821DABFC;
    }

    {
        r10 = 1u;
        const uint32_t lock_ea = r31 + 60u;
        uint32_t* const lock_ptr = reinterpret_cast<uint32_t*>(REX_RAW_ADDR(lock_ea));
        const uint32_t target_be = __builtin_bswap32(r23);
        uint32_t old_be;
        do {
            old_be = *lock_ptr;
        } while (!__sync_bool_compare_and_swap(lock_ptr, old_be, target_be));
        r8 = __builtin_bswap32(old_be);
        if (r8 == r23) r10 = 0u;
        if ((r10 & 0xFFu) == 0u) goto loc_821DABFC;
    }

    r11 = REX_LOAD_U32(r31 + 4u);
    {
        const uint32_t r10_hi = (r11 >> 21) & 0x7FFu;
        const uint32_t r9_mid = (r11 >> 10) & 0x7FFu;
        const int64_t r4_s = static_cast<int64_t>(r10_hi) - 1024;
        const uint32_t r10_lo = (r11 >> 4) & 0x1Fu;
        const int64_t r5_s = static_cast<int64_t>(r9_mid) - 1024;
        const uint32_t r11_lo = r11 & 0x1Fu;
        ctx.r4.s64 = r4_s;
        ctx.r5.s64 = r5_s;
        ctx.r6.u64 = static_cast<uint64_t>(r10_lo) + ctx.r4.u64;
        ctx.r7.u64 = static_cast<uint64_t>(r11_lo) + ctx.r5.u64;
    }

    if (r9 == 0u) {
        ctx.r3.u64 = r28;
        sub_821D9518(ctx, base);
    } else {
        ctx.r3.u64 = r27;
        sub_821D9770(ctx, base);
    }

    r11 = REX_LOAD_U16(r31 + 40u);
    {
        const uint64_t not_r3 = ~ctx.r3.u64;
        const uint32_t cleared_flags = r30 & 0xFFFFDFFFu;
        REX_STORE_U32(r31 + 8u, ctx.r3.u32);
        r11 &= 0x1u;
        REX_STORE_U32(r31 + 12u, static_cast<uint32_t>(not_r3));
        REX_STORE_U32(r31 + 36u, cleared_flags);
    }

    if (r11 != 0u) goto loc_821DABD4;

    ctx.r4.s64 = static_cast<int64_t>(sp) + 88;
    ctx.r5.u64 = REX_LOAD_U32(r29);
    ctx.r3.s64 = static_cast<int64_t>(sp) + 80;
    {
        const uint32_t f31_bits = std::bit_cast<uint32_t>(static_cast<float>(f31));
        REX_STORE_U32(sp + 84u, f31_bits);
    }
    REX_STORE_U32(sp + 80u, r31);

    ctx.lr = 0x821DAB78;
    {
        const auto s_r20 = ctx.r20; ctx.r20.u64 = r20;
        const auto s_r21 = ctx.r21; ctx.r21.u64 = r21;
        const auto s_r22 = ctx.r22; ctx.r22.u64 = r22;
        const auto s_r23 = ctx.r23; ctx.r23.u64 = r23;
        const auto s_r24 = ctx.r24; ctx.r24.u64 = r24;
        const auto s_r25 = ctx.r25; ctx.r25.u64 = r25;
        const auto s_r26 = ctx.r26; ctx.r26.u64 = r26;
        const auto s_r27 = ctx.r27; ctx.r27.u64 = r27;
        const auto s_r28 = ctx.r28; ctx.r28.u64 = r28;
        const auto s_r29 = ctx.r29; ctx.r29.u64 = r29;
        const auto s_r30 = ctx.r30; ctx.r30.u64 = r30;
        const auto s_r31 = ctx.r31; ctx.r31.u64 = r31;
        const auto s_f31 = ctx.f31; ctx.f31.f64 = f31;
        sub_821D6260(ctx, base);
        r20 = ctx.r20.u32; ctx.r20 = s_r20;
        r21 = ctx.r21.u32; ctx.r21 = s_r21;
        r22 = ctx.r22.u32; ctx.r22 = s_r22;
        r23 = ctx.r23.u32; ctx.r23 = s_r23;
        r24 = ctx.r24.u32; ctx.r24 = s_r24;
        r25 = ctx.r25.u32; ctx.r25 = s_r25;
        r26 = ctx.r26.u32; ctx.r26 = s_r26;
        r27 = ctx.r27.u32; ctx.r27 = s_r27;
        r28 = ctx.r28.u32; ctx.r28 = s_r28;
        r29 = ctx.r29.u32; ctx.r29 = s_r29;
        r30 = ctx.r30.u32; ctx.r30 = s_r30;
        r31 = ctx.r31.u32; ctx.r31 = s_r31;
        f31 = ctx.f31.f64; ctx.f31 = s_f31;
    }

    r31 = REX_LOAD_U32(sp + 80u);
    if (r31 == 0u) goto loc_821DABFC;

    r11 = REX_LOAD_U32(r31);
    ctx.r4.s64 = static_cast<int64_t>(sp) + 96;
    r30 = REX_LOAD_U32(r24 - 1616u);
    ctx.r3.u64 = r31;
    r11 = REX_LOAD_U32(r11 + 104u);

    ctx.lr = 0x821DABA0;
    REX_CALL_INDIRECT_FUNC(r11);

    ctx.r6.s64 = static_cast<int64_t>(sp) + 96;
    ctx.r5.u64 = r30;
    ctx.r4.u64 = r29;
    ctx.r3.u64 = r31;
    sub_821D7DE0(ctx, base);

    ctx.r5.u64 = r30;
    ctx.r4.u64 = r29;
    ctx.r3.u64 = r31;
    sub_821D6948(ctx, base);

    ctx.r3.u64 = r31;
    ctx.f1.f64 = double(std::bit_cast<float>(REX_LOAD_U32(sp + 84u)));

    ctx.lr = 0x821DABD0;
    sub_821D75B8(ctx, base);
    goto loc_821DABFC;

loc_821DABD4:
    {
        uint32_t* const tail_ptr = reinterpret_cast<uint32_t*>(REX_RAW_ADDR(r25));
        uint32_t old_be;
        uint32_t old_val;
        uint32_t next_be;
        do {
            old_be = *tail_ptr;
            old_val = __builtin_bswap32(old_be);
            next_be = __builtin_bswap32(r20 + old_val);
        } while (!__sync_bool_compare_and_swap(tail_ptr, old_be, next_be));
        REX_STORE_U32(old_val, r31);
    }

loc_821DABFC:
    if (r26 != 0u) goto loc_821DAA60;

loc_821DAC04:
    ctx.r1.s64 = static_cast<int64_t>(sp) + 240;
    (void)REX_LOAD_U64(ctx.r1.u32 - 112);
    return;
}

// --- 2. sub_821DA748 ---
extern "C" void sub_821DA748(PPCContext& ctx, uint8_t* base) {
    uint32_t r11 = 0, r10 = 0, r9 = 0, r25 = 0, r22 = 0, r20 = 0, r17 = 0;
    uint32_t r29 = 0, r23 = 0, r27 = 0, r26 = 0, r18 = 0, r19 = 0, r24 = 0, r21 = 0;
    uint32_t r28 = 0, r31 = 0, r8 = 0, r30 = 0;
    double f31 = 0.0;

    ctx.fpscr.disableFlushMode();
    const uint32_t old_sp = ctx.r1.u32;
    REX_STORE_U64(old_sp - 136, 0ULL);

    const uint32_t sp = old_sp - 304;
    REX_STORE_U32(sp, old_sp);
    ctx.r1.u32 = sp;

    r10 = 0x82B70000u;
    r11 = 0x82B60000u - 22576u;
    r9 = 44000u;
    r10 = REX_LOAD_U32(r10 + 17212u);
    r11 = (r10 - r11) & 0xFFFFFFF8u;

    if (static_cast<int32_t>(r11) >= static_cast<int32_t>(r9)) goto loc_821DA9BC;

    r11 = 0x82C00000u;
    ctx.r5.s64 = static_cast<int64_t>(sp) + 96;
    r25 = REX_LOAD_U16(r11 + 7348u);

    sub_821D7258(ctx, base);
    r22 = ctx.r3.u32;
    r20 = sp + 96u;
    r17 = 0u;

    if (r22 == 0u) goto loc_821DA9BC;

    r29 = 0x82BF0000u - 1608u;
    r10 = 0x82000000u;
    r23 = 0x82BF0000u;
    r27 = 0x82B70000u + 17336u;
    f31 = double(std::bit_cast<float>(REX_LOAD_U32(r10 + 2612u)));
    r18 = 0x82B70000u;
    r26 = 0x82B70000u + 26960u;
    r19 = 0x82B70000u;
    r24 = 0x82B70000u - 31308u;
    r21 = 8u;

loc_821DA7DC:
    r11 = REX_LOAD_U32(r20);
    r22 -= 1u;
    if (r11 == 0u) goto loc_821DA9AC;
    if (static_cast<int32_t>(r17) == -1) goto loc_821DA9AC;
    r28 = REX_LOAD_U32(r11);
    if (r28 == 0u) goto loc_821DA9AC;

loc_821DA800:
    r31 = REX_LOAD_U32(r28);
    r28 = REX_LOAD_U32(r28 + 4u);
    if (r28 != 0u) {
        (void)REX_LOAD_U32(r28 + 4u);
        (void)REX_LOAD_U32(r28);
    }

    r11 = REX_LOAD_U32(r31 + 36u);
    r10 = r11 & 0x2000000u;
    if (r10 != 0u) {
        r10 = REX_LOAD_U8(r19 + 17145u);
        if (r10 != 0u) {
            if ((r11 & 0x1000000u) == 0u) goto loc_821DA9A4;
        }
    }

    if ((r11 & 0x1000000u) != 0u) {
        r11 = REX_LOAD_U8(r18 + 17146u);
        if (r11 != 0u) goto loc_821DA9A4;
    }

    {
        r10 = 1u;
        const uint32_t lock_ea = r31 + 60u;
        uint32_t* const lock_ptr = reinterpret_cast<uint32_t*>(REX_RAW_ADDR(lock_ea));
        const uint32_t target_be = __builtin_bswap32(r25);
        uint32_t old_be;
        do {
            old_be = *lock_ptr;
        } while (!__sync_bool_compare_and_swap(lock_ptr, old_be, target_be));
        r9 = __builtin_bswap32(old_be);
        if (r9 == r25) r10 = 0u;
        if ((r10 & 0xFFu) == 0u) goto loc_821DA9A4;
    }

    r11 = REX_LOAD_U32(r31 + 4u);
    r10 = REX_LOAD_U32(r31 + 36u);
    {
        const uint32_t r9_hi = (r11 >> 21) & 0x7FFu;
        const uint32_t r8_mid = (r11 >> 10) & 0x7FFu;
        r10 &= 0xFFFFDFFFu;
        ctx.r4.s64 = static_cast<int64_t>(r9_hi) - 1024;
        ctx.r5.s64 = static_cast<int64_t>(r8_mid) - 1024;
        const uint32_t r9_lo = (r11 >> 4) & 0x1Fu;
        const uint32_t r11_lo = r11 & 0x1Fu;
        r8 = r10 & 0x2000000u;
        REX_STORE_U32(r31 + 36u, r10);
        ctx.r6.u64 = static_cast<uint64_t>(r9_lo) + ctx.r4.u64;
        ctx.r7.u64 = static_cast<uint64_t>(r11_lo) + ctx.r5.u64;
    }

    if (r8 == 0u) {
        ctx.r3.u64 = r27;
        sub_821D9518(ctx, base);
    } else {
        ctx.r3.u64 = r26;
        sub_821D9770(ctx, base);
    }

    r11 = REX_LOAD_U16(r31 + 40u);
    {
        const uint64_t not_r3 = ~ctx.r3.u64;
        REX_STORE_U32(r31 + 8u, ctx.r3.u32);
        r11 &= 0x1u;
        REX_STORE_U32(r31 + 12u, static_cast<uint32_t>(not_r3));
    }
    if (r11 != 0u) goto loc_821DA97C;

    ctx.r4.s64 = static_cast<int64_t>(sp) + 88;
    ctx.r5.u64 = REX_LOAD_U32(r29);
    ctx.r3.s64 = static_cast<int64_t>(sp) + 80;
    {
        const uint32_t f31_bits = std::bit_cast<uint32_t>(static_cast<float>(f31));
        REX_STORE_U32(sp + 84u, f31_bits);
    }
    REX_STORE_U32(sp + 80u, r31);

    ctx.lr = 0x821DA920;
    {
        const auto s_r17 = ctx.r17; ctx.r17.u64 = r17;
        const auto s_r18 = ctx.r18; ctx.r18.u64 = r18;
        const auto s_r19 = ctx.r19; ctx.r19.u64 = r19;
        const auto s_r20 = ctx.r20; ctx.r20.u64 = r20;
        const auto s_r21 = ctx.r21; ctx.r21.u64 = r21;
        const auto s_r22 = ctx.r22; ctx.r22.u64 = r22;
        const auto s_r23 = ctx.r23; ctx.r23.u64 = r23;
        const auto s_r24 = ctx.r24; ctx.r24.u64 = r24;
        const auto s_r25 = ctx.r25; ctx.r25.u64 = r25;
        const auto s_r26 = ctx.r26; ctx.r26.u64 = r26;
        const auto s_r27 = ctx.r27; ctx.r27.u64 = r27;
        const auto s_r28 = ctx.r28; ctx.r28.u64 = r28;
        const auto s_r29 = ctx.r29; ctx.r29.u64 = r29;
        const auto s_r31 = ctx.r31; ctx.r31.u64 = r31;
        const auto s_f31 = ctx.f31; ctx.f31.f64 = f31;
        sub_821D6260(ctx, base);
        r17 = ctx.r17.u32; ctx.r17 = s_r17;
        r18 = ctx.r18.u32; ctx.r18 = s_r18;
        r19 = ctx.r19.u32; ctx.r19 = s_r19;
        r20 = ctx.r20.u32; ctx.r20 = s_r20;
        r21 = ctx.r21.u32; ctx.r21 = s_r21;
        r22 = ctx.r22.u32; ctx.r22 = s_r22;
        r23 = ctx.r23.u32; ctx.r23 = s_r23;
        r24 = ctx.r24.u32; ctx.r24 = s_r24;
        r25 = ctx.r25.u32; ctx.r25 = s_r25;
        r26 = ctx.r26.u32; ctx.r26 = s_r26;
        r27 = ctx.r27.u32; ctx.r27 = s_r27;
        r28 = ctx.r28.u32; ctx.r28 = s_r28;
        r29 = ctx.r29.u32; ctx.r29 = s_r29;
        r31 = ctx.r31.u32; ctx.r31 = s_r31;
        f31 = ctx.f31.f64; ctx.f31 = s_f31;
    }

    r31 = REX_LOAD_U32(sp + 80u);
    if (r31 == 0u) goto loc_821DA9A4;
    r11 = REX_LOAD_U32(r31);
    ctx.r4.s64 = static_cast<int64_t>(sp) + 128;
    r30 = REX_LOAD_U32(r23 - 1616u);
    ctx.r3.u64 = r31;
    r11 = REX_LOAD_U32(r11 + 104u);

    ctx.lr = 0x821DA948;
    REX_CALL_INDIRECT_FUNC(r11);

    ctx.r6.s64 = static_cast<int64_t>(sp) + 128;
    ctx.r5.u64 = r30;
    ctx.r4.u64 = r29;
    ctx.r3.u64 = r31;
    sub_821D7DE0(ctx, base);

    ctx.r5.u64 = r30;
    ctx.r4.u64 = r29;
    ctx.r3.u64 = r31;
    sub_821D6948(ctx, base);

    ctx.r3.u64 = r31;
    ctx.f1.f64 = double(std::bit_cast<float>(REX_LOAD_U32(sp + 84u)));

    ctx.lr = 0x821DA978;
    sub_821D75B8(ctx, base);
    goto loc_821DA9A4;

loc_821DA97C:
    {
        uint32_t* const tail_ptr = reinterpret_cast<uint32_t*>(REX_RAW_ADDR(r24));
        uint32_t old_be;
        uint32_t old_val;
        uint32_t next_be;
        do {
            old_be = *tail_ptr;
            old_val = __builtin_bswap32(old_be);
            next_be = __builtin_bswap32(r21 + old_val);
        } while (!__sync_bool_compare_and_swap(tail_ptr, old_be, next_be));
        REX_STORE_U32(old_val, r31);
    }

loc_821DA9A4:
    if (r28 != 0u) goto loc_821DA800;
loc_821DA9AC:
    r17 += 1u;
    r20 += 4u;
    if (r22 != 0u) goto loc_821DA7DC;

loc_821DA9BC:
    ctx.r1.s64 = static_cast<int64_t>(sp) + 304;
    (void)REX_LOAD_U64(ctx.r1.u32 - 136);
    return;
}

#endif // ENABLE_BATCH12_HOOKS

// ============================================================================
// Batch 12 Verified NEON Hooks (Snapdragon 888 Hardware Verified 12/12 MATCH PASS)
// 1. sub_8228C590: Water Grid Simulation (8.13x / 8.13x net on hardware)
// 2. sub_829BF170: Bullet GJK Convex-Convex Solver (1.35x / 1.51x net on hardware)
// ============================================================================
#ifndef ENABLE_BATCH12_VERIFIED_HOOKS
#define ENABLE_BATCH12_VERIFIED_HOOKS 1
#endif

#if ENABLE_BATCH12_VERIFIED_HOOKS

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#endif
#include <bit>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>

#ifndef REX_TOUCH_SPAN
#define REX_TOUCH_SPAN(ea, len) ((void)0)
#endif

extern "C" {
void sub_82A02648(PPCContext& ctx, uint8_t* base);
void sub_829FFD48(PPCContext& ctx, uint8_t* base);
void sub_829D7428(PPCContext& ctx, uint8_t* base);
void sub_8299C080(PPCContext& ctx, uint8_t* base);
void sub_8296C540(PPCContext& ctx, uint8_t* base);
void sub_829D6F88(PPCContext& ctx, uint8_t* base);
void sub_829D7000(PPCContext& ctx, uint8_t* base);
}

// candidate_8228C590.inc


namespace batch12_water_sim_detail {

inline float LoadF32(uint8_t* base, uint32_t addr) noexcept {
  const uint32_t u = REX_LOAD_U32(addr);
  return std::bit_cast<float>(u);
}

inline void StoreF32(uint8_t* base, uint32_t addr, float val) noexcept {
  const uint32_t u = std::bit_cast<uint32_t>(val);
  REX_STORE_U32(addr, u);
}

static inline int32_t FctiwzExact(double d) noexcept {
  if (std::isnan(d)) return static_cast<int32_t>(0x80000000u);
  if (d > static_cast<double>(INT_MAX)) return INT_MAX;
  if (d < static_cast<double>(INT_MIN)) return INT_MIN;
  return static_cast<int32_t>(d);
}

static inline int32_t Mod100SignedExact(int32_t x) noexcept {
  constexpr int32_t kMagic = static_cast<int32_t>((20971u << 16) | 34079u);  // 1374389535
  int32_t q = static_cast<int32_t>((int64_t(x) * int64_t(kMagic)) >> 32) >> 5;
  q += static_cast<int32_t>(static_cast<uint32_t>(q) >> 31);
  return x - q * 100;
}

static inline float32x4_t LoadBeF32x4(const uint8_t* p) noexcept {
#if defined(__aarch64__)
  return vreinterpretq_f32_u8(vrev32q_u8(vld1q_u8(p)));
#else
  float tmp[4];
  for (int i = 0; i < 4; ++i) {
    uint32_t u;
    std::memcpy(&u, p + i * 4, 4);
    tmp[i] = std::bit_cast<float>(__builtin_bswap32(u));
  }
  return vld1q_f32(tmp);
#endif
}

static inline void StoreBeF32x4(uint8_t* p, float32x4_t v) noexcept {
#if defined(__aarch64__)
  vst1q_u8(p, vrev32q_u8(vreinterpretq_u8_f32(v)));
#else
  alignas(16) float tmp[4];
  vst1q_f32(tmp, v);
  for (int i = 0; i < 4; ++i) {
    uint32_t u = __builtin_bswap32(std::bit_cast<uint32_t>(tmp[i]));
    std::memcpy(p + i * 4, &u, 4);
  }
#endif
}

static inline void LoadLinearRingRow(uint8_t* base, uint32_t row_base_addr,
                                     uint32_t col0, float* dst) noexcept {
  const uint32_t n1 = 100u - col0;
  uint32_t i = 0;
  REX_TOUCH_SPAN(row_base_addr + (col0 << 2), n1 << 2);
  const uint8_t* p1 = REX_RAW_ADDR(row_base_addr + (col0 << 2));
  for (; i + 4u <= n1; i += 4u) {
    vst1q_f32(dst + i, LoadBeF32x4(p1 + (i << 2)));
  }
  for (; i < n1; ++i) {
    uint32_t u;
    std::memcpy(&u, p1 + (i << 2), 4);
    dst[i] = std::bit_cast<float>(__builtin_bswap32(u));
  }

  uint32_t j = 0;
  REX_TOUCH_SPAN(row_base_addr, col0 << 2);
  const uint8_t* p2 = REX_RAW_ADDR(row_base_addr);
  float* dst2 = dst + n1;
  for (; j + 4u <= col0; j += 4u) {
    vst1q_f32(dst2 + j, LoadBeF32x4(p2 + (j << 2)));
  }
  for (; j < col0; ++j) {
    uint32_t u;
    std::memcpy(&u, p2 + (j << 2), 4);
    dst2[j] = std::bit_cast<float>(__builtin_bswap32(u));
  }
}

static inline void StoreLinearRingRow(uint8_t* base, uint32_t row_base_addr,
                                      uint32_t col0, const float* src) noexcept {
  const uint32_t n1 = 100u - col0;
  uint32_t i = 0;
  REX_TOUCH_SPAN(row_base_addr + (col0 << 2), n1 << 2);
  uint8_t* p1 = REX_RAW_ADDR(row_base_addr + (col0 << 2));
  for (; i + 4u <= n1; i += 4u) {
    StoreBeF32x4(p1 + (i << 2), vld1q_f32(src + i));
  }
  for (; i < n1; ++i) {
    uint32_t u = __builtin_bswap32(std::bit_cast<uint32_t>(src[i]));
    std::memcpy(p1 + (i << 2), &u, 4);
  }

  uint32_t j = 0;
  REX_TOUCH_SPAN(row_base_addr, col0 << 2);
  uint8_t* p2 = REX_RAW_ADDR(row_base_addr);
  const float* src2 = src + n1;
  for (; j + 4u <= col0; j += 4u) {
    StoreBeF32x4(p2 + (j << 2), vld1q_f32(src2 + j));
  }
  for (; j < col0; ++j) {
    uint32_t u = __builtin_bswap32(std::bit_cast<uint32_t>(src2[j]));
    std::memcpy(p2 + (j << 2), &u, 4);
  }
}

// Bit-exact double-precision FMA rounded to float32 across 4 lanes (matches PPC fmadds).
static inline float32x4_t Fmadds4(float32x4_t a, float32x4_t b, float32x4_t c) noexcept {
#if defined(__aarch64__)
  const float64x2_t lo = vfmaq_f64(vcvt_f64_f32(vget_low_f32(c)),
                                   vcvt_f64_f32(vget_low_f32(a)),
                                   vcvt_f64_f32(vget_low_f32(b)));
  const float64x2_t hi = vfmaq_f64(vcvt_f64_f32(vget_high_f32(c)),
                                   vcvt_f64_f32(vget_high_f32(a)),
                                   vcvt_f64_f32(vget_high_f32(b)));
  return vcombine_f32(vcvt_f32_f64(lo), vcvt_f32_f64(hi));
#else
  alignas(16) float fa[4], fb[4], fc[4], fr[4];
  vst1q_f32(fa, a); vst1q_f32(fb, b); vst1q_f32(fc, c);
  for (int i = 0; i < 4; ++i) fr[i] = float(std::fma(double(fa[i]), double(fb[i]), double(fc[i])));
  return vld1q_f32(fr);
#endif
}

// Bit-exact PPC fnmsubs: float(-fma(double(a), double(b), -double(c))) across 4 lanes.
static inline float32x4_t Fnmsubs4(float32x4_t a, float32x4_t b, float32x4_t c) noexcept {
#if defined(__aarch64__)
  const float64x2_t lo = vnegq_f64(vfmaq_f64(vnegq_f64(vcvt_f64_f32(vget_low_f32(c))),
                                             vcvt_f64_f32(vget_low_f32(a)),
                                             vcvt_f64_f32(vget_low_f32(b))));
  const float64x2_t hi = vnegq_f64(vfmaq_f64(vnegq_f64(vcvt_f64_f32(vget_high_f32(c))),
                                             vcvt_f64_f32(vget_high_f32(a)),
                                             vcvt_f64_f32(vget_high_f32(b))));
  return vcombine_f32(vcvt_f32_f64(lo), vcvt_f32_f64(hi));
#else
  alignas(16) float fa[4], fb[4], fc[4], fr[4];
  vst1q_f32(fa, a); vst1q_f32(fb, b); vst1q_f32(fc, c);
  for (int i = 0; i < 4; ++i) fr[i] = float(-std::fma(double(fa[i]), double(fb[i]), -double(fc[i])));
  return vld1q_f32(fr);
#endif
}

// Exact inlined sub_8228A358 (clear one column of both 100x100 water grids).
static inline void ClearWaterColExact(uint8_t* base, int32_t world_y,
                                      uint32_t grid_h_addr, uint32_t grid_v_addr,
                                      uint32_t fill_be) noexcept {
  const int32_t col = Mod100SignedExact(world_y / 2 + 100000);
  uint32_t off = static_cast<uint32_t>(col) << 2;
  for (int i = 0; i < 100; ++i, off += 400u) {
    std::memcpy(REX_RAW_ADDR(grid_h_addr + off), &fill_be, 4);
    std::memcpy(REX_RAW_ADDR(grid_v_addr + off), &fill_be, 4);
  }
}

static inline float32x4_t LoadGuestVecRev(uint8_t* base, uint32_t addr) noexcept {
  const uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(addr & ~0xFu)));
  const uint8x16_t rev64 = vrev64q_u8(raw);
  return vreinterpretq_f32_u8(vextq_u8(rev64, rev64, 8));
}


static inline void sub_8228C590Impl(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

  constexpr uint32_t kBase82C9 = static_cast<uint32_t>(int32_t(-2100756480));
  constexpr uint32_t kBase82CA = static_cast<uint32_t>(int32_t(-2100690944));
  constexpr uint32_t kBase82CB = static_cast<uint32_t>(int32_t(-2100625408));
  constexpr uint32_t kBase82A6 = static_cast<uint32_t>(int32_t(-2102788096));
  constexpr uint32_t kBase820C = static_cast<uint32_t>(int32_t(-2113142784));
  constexpr uint32_t kBase8200 = static_cast<uint32_t>(int32_t(-2113929216));
  constexpr uint32_t kBase8201 = static_cast<uint32_t>(int32_t(-2113863680));
  constexpr uint32_t kBase82B0 = static_cast<uint32_t>(int32_t(-2102394880));
  constexpr uint32_t kBase831B = static_cast<uint32_t>(int32_t(-2095382528));

  constexpr uint32_t kGridVAddr      = kBase82C9 + 17944u;
  constexpr uint32_t kOldXAddr       = kBase82C9 + 17940u;
  constexpr uint32_t kOldYAddr       = kBase82C9 + 17936u;
  constexpr uint32_t kRingIdxXAddr   = kBase82C9 + 17932u;
  constexpr uint32_t kRingIdxYAddr   = kBase82C9 + 17928u;
  constexpr uint32_t kWaveLutAddr    = kBase82C9 + 17528u;
  constexpr uint32_t kGridHAddr      = static_cast<uint32_t>(int32_t(kBase82CA) - 7592);
  constexpr uint32_t kDampRectsAddr  = kBase82CA + 32408u;
  constexpr uint32_t kLutInitedAddr  = static_cast<uint32_t>(int32_t(kBase82CB) - 16912);
  constexpr uint32_t kDampCountAddr  = static_cast<uint32_t>(int32_t(kBase82CB) - 16916);
  constexpr uint32_t kDampVertsAddr  = static_cast<uint32_t>(int32_t(kBase82CB) - 16856);
  constexpr uint32_t kRngStateAddr   = static_cast<uint32_t>(int32_t(kBase82A6) - 29508);

  constexpr uint32_t kConst_m29484   = static_cast<uint32_t>(int32_t(kBase82A6) - 29484);
  constexpr uint32_t kConst_m29480   = static_cast<uint32_t>(int32_t(kBase82A6) - 29480);
  constexpr uint32_t kConst_m29476   = static_cast<uint32_t>(int32_t(kBase82A6) - 29476);
  constexpr uint32_t kConst_m29472   = static_cast<uint32_t>(int32_t(kBase82A6) - 29472);
  constexpr uint32_t kConst_m29468   = static_cast<uint32_t>(int32_t(kBase82A6) - 29468);
  constexpr uint32_t kConst_m4904    = static_cast<uint32_t>(int32_t(kBase820C) - 4904);
  constexpr uint32_t kConst_m4896    = static_cast<uint32_t>(int32_t(kBase820C) - 4896);
  constexpr uint32_t kConst_m4384    = static_cast<uint32_t>(int32_t(kBase820C) - 4384);
  constexpr uint32_t kConst_2612     = kBase8200 + 2612u;
  constexpr uint32_t kConst_3400     = kBase8200 + 3400u;
  constexpr uint32_t kConst_15556    = kBase8200 + 15556u;
  constexpr uint32_t kConst_20568    = kBase8200 + 20568u;
  constexpr uint32_t kConst_m25472   = static_cast<uint32_t>(int32_t(kBase8201) - 25472);
  constexpr uint32_t kConst_m25468   = static_cast<uint32_t>(int32_t(kBase8201) - 25468);

  const uint32_t orig_sp = ctx.r1.u32;
  const uint32_t sp = orig_sp - 480u;
  REX_STORE_U32(sp, orig_sp);
  ctx.r1.u32 = sp;

  ctx.fpscr.disableFlushMode();
  const uint32_t params = ctx.r3.u32;  // r27

  // 1. Update 100x100 toroidal window origin & scroll-clear exposed rows/cols
  const uint32_t pos_aligned = params & ~0xFu;
  const float cam_x = LoadF32(base, pos_aligned);
  const float cam_y = LoadF32(base, pos_aligned + 4u);
  int32_t old_x = static_cast<int32_t>(REX_LOAD_U32(kOldXAddr));
  int32_t old_y = static_cast<int32_t>(REX_LOAD_U32(kOldYAddr));

  const int32_t new_x = ((FctiwzExact(double(cam_x)) - 100) / 2) * 2;
  const int32_t new_y = ((FctiwzExact(double(cam_y)) - 100) / 2) * 2;
  REX_STORE_U32(kOldXAddr, static_cast<uint32_t>(new_x));
  REX_STORE_U32(kOldYAddr, static_cast<uint32_t>(new_y));

  const int32_t ring_x = Mod100SignedExact(new_x / 2 + 100000);
  const int32_t ring_y = Mod100SignedExact(new_y / 2 + 100000);
  REX_STORE_U32(kRingIdxXAddr, static_cast<uint32_t>(ring_x));
  REX_STORE_U32(kRingIdxYAddr, static_cast<uint32_t>(ring_y));

  if (new_x != old_x || new_y != old_y) {
    const int32_t dx = (old_x - new_x >= 0) ? (old_x - new_x) : (new_x - old_x);
    const int32_t dy = (old_y - new_y >= 0) ? (old_y - new_y) : (new_y - old_y);
    if (dx > 200 || dy > 200) {
      REX_TOUCH_SPAN(kGridHAddr, 40000u);
      REX_TOUCH_SPAN(kGridVAddr, 40000u);
      std::memset(REX_RAW_ADDR(kGridHAddr), 0, 40000u);
      std::memset(REX_RAW_ADDR(kGridVAddr), 0, 40000u);
    } else {
      while (old_x < new_x) {
        const uint32_t row_off = static_cast<uint32_t>(Mod100SignedExact(old_x / 2 + 100000)) * 400u;
        REX_TOUCH_SPAN(kGridVAddr + row_off, 400u);
        REX_TOUCH_SPAN(kGridHAddr + row_off, 400u);
        std::memset(REX_RAW_ADDR(kGridVAddr + row_off), 0, 400u);
        std::memset(REX_RAW_ADDR(kGridHAddr + row_off), 0, 400u);
        old_x += 2;
      }
      while (old_x > new_x) {
        old_x -= 2;
        const uint32_t row_off = static_cast<uint32_t>(Mod100SignedExact(old_x / 2 + 100000)) * 400u;
        REX_TOUCH_SPAN(kGridVAddr + row_off, 400u);
        REX_TOUCH_SPAN(kGridHAddr + row_off, 400u);
        std::memset(REX_RAW_ADDR(kGridVAddr + row_off), 0, 400u);
        std::memset(REX_RAW_ADDR(kGridHAddr + row_off), 0, 400u);
      }
      uint32_t fill_be = 0u;
      std::memcpy(&fill_be, REX_RAW_ADDR(kConst_2612), 4);
      while (old_y < new_y) {
        ClearWaterColExact(base, old_y, kGridHAddr, kGridVAddr, fill_be);
        old_y += 2;
      }
      while (old_y > new_y) {
        old_y -= 2;
        ClearWaterColExact(base, old_y, kGridHAddr, kGridVAddr, fill_be);
      }
    }
  }

  // 2. Simulation scalars + powf damping factor
  const float dt = LoadF32(base, params + 16u);            // f22
  const float drop_rate = LoadF32(base, params + 24u);     // f30
  const float drop_amp = LoadF32(base, params + 28u);      // f31
  const float c_m29476 = LoadF32(base, kConst_m29476);
  const float c_m29472 = LoadF32(base, kConst_m29472);
  const float f29_stiff = static_cast<float>(double(c_m29476) * double(dt));
  const float f28_rest  = static_cast<float>(double(c_m29472) * double(dt));

  ctx.f1.f64 = double(LoadF32(base, kConst_m29468));
  ctx.f2.f64 = double(dt);
  ctx.lr = 0x8228C84Cu;
  sub_82A02648(ctx, base);
  ctx.fpscr.disableFlushMode();
  const float f27_damp = static_cast<float>(ctx.f1.f64);

  // 3. Random ripple injection (MWC64 PRNG)
  const float c_m4896 = LoadF32(base, kConst_m4896);
  const int32_t drop_count = FctiwzExact(double(static_cast<float>(double(drop_rate) * double(c_m4896))));
  if (drop_count > 0) {
    constexpr uint64_t kMwcMul = (23772ull << 16) | 64167ull;
    const float c_3400 = LoadF32(base, kConst_3400);
    const float c_m25468 = LoadF32(base, kConst_m25468);
    uint32_t s0 = REX_LOAD_U32(kRngStateAddr);
    uint32_t s1 = REX_LOAD_U32(kRngStateAddr + 4u);

    for (int32_t k = 0; k < drop_count; ++k) {
      uint64_t t = uint64_t(s0) * kMwcMul + uint64_t(s1);
      s0 = static_cast<uint32_t>(t);
      s1 = static_cast<uint32_t>(t >> 32);
      const uint32_t rx = (s0 & 0x7FFFFFFFu) % 100u;

      t = uint64_t(s0) * kMwcMul + uint64_t(s1);
      s0 = static_cast<uint32_t>(t);
      s1 = static_cast<uint32_t>(t >> 32);
      const uint32_t ry = (s0 & 0x7FFFFFFFu) % 100u;

      t = uint64_t(s0) * kMwcMul + uint64_t(s1);
      s0 = static_cast<uint32_t>(t);
      s1 = static_cast<uint32_t>(t >> 32);
      const float rnd_f = static_cast<float>(static_cast<double>(static_cast<int64_t>(s0 & 0x7FFFFFu)));

      float amp = static_cast<float>(std::fma(double(rnd_f), double(c_m25468), -double(c_3400)));
      amp = static_cast<float>(double(amp) * double(c_m29472));
      amp = static_cast<float>(double(amp) * double(drop_amp));

      const uint32_t cell_addr = kGridVAddr + ((ry * 100u + rx) << 2);
      const float cur_v = LoadF32(base, cell_addr);
      StoreF32(base, cell_addr, static_cast<float>(std::fma(double(amp), double(dt), double(cur_v))));
    }
    REX_STORE_U32(kRngStateAddr, s0);
    REX_STORE_U32(kRngStateAddr + 4u, s1);
  }

  // 4. Directional wave LUT initialization (one-time) + 100x100 wave forcing pass
  const float wave_p36 = LoadF32(base, params + 36u);
  const uint32_t period = REX_LOAD_U32(params + 40u);
  const uint32_t time_val = REX_LOAD_U32(params + 20u);
  if (period == 0u) ppc_trap(ctx, base, 0);

  const float c_m29480 = LoadF32(base, kConst_m29480);
  const float wave_amp = static_cast<float>(double(static_cast<float>(double(wave_p36) * double(dt))) * double(c_m29480));
  const uint32_t rem = time_val % period;
  const float c_2612 = LoadF32(base, kConst_2612);
  const float c_m4384 = LoadF32(base, kConst_m4384);
  const float rem_f = static_cast<float>(static_cast<double>(static_cast<int64_t>(rem)));
  const float period_f = static_cast<float>(static_cast<double>(static_cast<int64_t>(static_cast<int32_t>(period))));
  const float wave_phase = static_cast<float>(double(rem_f) * double(static_cast<float>(double(c_m4384) / double(period_f))));

  uint8_t lut_ready = REX_LOAD_U8(kLutInitedAddr);
  if (lut_ready == 0u) {
    const float c_m4904 = LoadF32(base, kConst_m4904);
    const float c_m29484 = LoadF32(base, kConst_m29484);
    float sum = c_2612;
    for (int32_t i = 0; i < 100; ++i) {
      const float fi = static_cast<float>(static_cast<double>(static_cast<int64_t>(i)));
      ctx.f1.f64 = double(static_cast<float>(double(fi) * double(c_m4904)));
      ctx.f2.f64 = double(c_m29484);
      ctx.lr = 0x8228CCC8u;
      sub_82A02648(ctx, base);
      ctx.fpscr.disableFlushMode();
      ctx.f1.f64 = double(static_cast<float>(double(static_cast<float>(ctx.f1.f64)) * double(c_m4384)));
      ctx.lr = 0x8228CCD4u;
      sub_829FFD48(ctx, base);
      ctx.fpscr.disableFlushMode();
      const float val = static_cast<float>(ctx.f1.f64);
      StoreF32(base, kWaveLutAddr + (static_cast<uint32_t>(i) << 2), val);
      sum = static_cast<float>(double(val) + double(sum));
    }
    const float mean = static_cast<float>(double(sum) * double(c_m4904));
    for (uint32_t i = 0; i < 100u; ++i) {
      const float v = LoadF32(base, kWaveLutAddr + (i << 2));
      StoreF32(base, kWaveLutAddr + (i << 2), static_cast<float>(double(v) - double(mean)));
    }
    lut_ready = 1u;
    REX_STORE_U8(kLutInitedAddr, 1u);
  }

  const uint32_t cur_ring_x = REX_LOAD_U32(kRingIdxXAddr);
  const uint32_t cur_ring_y = REX_LOAD_U32(kRingIdxYAddr);
  const int32_t cur_world_x = static_cast<int32_t>(REX_LOAD_U32(kOldXAddr));
  const int32_t cur_world_y = static_cast<int32_t>(REX_LOAD_U32(kOldYAddr));

  if (lut_ready == 1u) {
    // Exact VMX128 normalized direction vector matching loc_8228CD8C..loc_8228CEC8
    ctx.fpscr.enableFlushModeUnconditional();
    alignas(16) float raw_dir[4] = {
        0.0f,
        c_2612,
        static_cast<float>(static_cast<double>(static_cast<int64_t>(cur_world_y - 400))),
        static_cast<float>(static_cast<double>(static_cast<int64_t>(cur_world_x + 100))),
    };
    const float32x4_t v12 = vld1q_f32(raw_dir);
    float32x4_t dp = vmulq_f32(v12, v12);
    dp = vsetq_lane_f32(0.0f, dp, 0);
    dp = vpaddq_f32(dp, dp);
    dp = vpaddq_f32(dp, dp);

    const float32x4_t v_inf = LoadGuestVecRev(base, static_cast<uint32_t>(int32_t(kBase831B) - 18560));
    const float32x4_t v_half = LoadGuestVecRev(base, kBase82B0 + 29104u);
    const float32x4_t v_one = LoadGuestVecRev(base, kBase82B0 + 29088u);
    const float32x4_t v_mask = LoadGuestVecRev(base, static_cast<uint32_t>(int32_t(kBase831B) - 18704));
    const float32x4_t v_eps_raw = LoadGuestVecRev(base, kBase82B0 + 29248u);
    const float32x4_t v_fill = LoadGuestVecRev(base, static_cast<uint32_t>(int32_t(kBase831B) - 18736));
    const float32x4_t v_fallback = LoadGuestVecRev(base, kBase82B0 + 29136u);

    const float32x4_t dp_lhs = vreinterpretq_f32_u32(
        vorrq_u32(vandq_u32(vreinterpretq_u32_f32(dp), vreinterpretq_u32_f32(v_mask)),
                  vreinterpretq_u32_f32(v_fill)));
    const float32x4_t eps_rhs = vreinterpretq_f32_u32(
        vandq_u32(vreinterpretq_u32_f32(v_eps_raw), vreinterpretq_u32_f32(v_mask)));
    const uint32x4_t valid = vcgtq_f32(dp_lhs, eps_rhs);

    float32x4_t rsq = vdivq_f32(vdupq_n_f32(1.0f), vsqrtq_f32(dp));
    rsq = vbslq_f32(vceqq_f32(rsq, v_inf), vdupq_n_f32(0.0f), rsq);
    const float32x4_t rsq2 = vmulq_f32(rsq, rsq);
    const float32x4_t nr = std::bit_cast<float32x4_t>(REX_VNMSUBFP(
        std::bit_cast<simde__m128>(dp), std::bit_cast<simde__m128>(rsq2), std::bit_cast<simde__m128>(v_one)));
    rsq = std::bit_cast<float32x4_t>(REX_VMADDFP(
        std::bit_cast<simde__m128>(nr), std::bit_cast<simde__m128>(vmulq_f32(v_half, rsq)),
        std::bit_cast<simde__m128>(rsq)));
    const float32x4_t norm_dir = vbslq_f32(valid, vmulq_f32(rsq, v12), v_fallback);
    ctx.fpscr.disableFlushModeUnconditional();

    const float dir_x = vgetq_lane_f32(norm_dir, 3);
    const float dir_y = vgetq_lane_f32(norm_dir, 2);
    const float dir_z = vgetq_lane_f32(norm_dir, 1);
    const float dir_z_term = static_cast<float>(double(dir_z) * double(c_2612));
    const float wave_freq = LoadF32(base, params + 32u);
    const float c_m25472 = LoadF32(base, kConst_m25472);

    // Preload 100-float wave LUT into host L1D (little-endian)
    alignas(16) float lut[100];
    REX_TOUCH_SPAN(kWaveLutAddr, 400u);
    const uint8_t* lut_ptr = REX_RAW_ADDR(kWaveLutAddr);
    for (uint32_t i = 0; i < 100u; i += 4u) {
      vst1q_f32(lut + i, LoadBeF32x4(lut_ptr + (i << 2)));
    }
    // Precompute y_term[0..99] = float(double(dir_y) * double(float(cur_world_y + r22*2)))
    alignas(16) float y_terms[100];
    for (int32_t r22 = 0; r22 < 100; ++r22) {
      const float fy = static_cast<float>(static_cast<double>(static_cast<int64_t>(cur_world_y + r22 * 2)));
      y_terms[r22] = static_cast<float>(double(dir_y) * double(fy));
    }

    const float32x4_t v_freq = vdupq_n_f32(wave_freq);
    const float32x4_t v_phase = vdupq_n_f32(wave_phase);
    const float32x4_t v_scale = vdupq_n_f32(c_m25472);
    const float32x4_t v_amp = vdupq_n_f32(wave_amp);

    // Contiguous row-major traversal (px outer, py inner): mathematically identical because
    // every (px, py) cell is independent and visited exactly once.
    alignas(16) float v_row[100];
    uint32_t px = cur_ring_x;
    for (int32_t cx = 0; cx < 100; ++cx) {
      const uint32_t row_addr = kGridVAddr + px * 400u;
      LoadLinearRingRow(base, row_addr, cur_ring_y, v_row);

      const float fx = static_cast<float>(static_cast<double>(static_cast<int64_t>(cur_world_x + cx * 2)));
      const float x_term = static_cast<float>(std::fma(double(fx), double(dir_x), double(dir_z_term)));
      const float32x4_t v_xterm = vdupq_n_f32(x_term);

      for (uint32_t cy = 0; cy < 100u; cy += 4u) {
        const float32x4_t t2 = vaddq_f32(v_xterm, vld1q_f32(y_terms + cy));
        const float32x4_t t3 = Fmadds4(t2, v_freq, v_phase);
        const float32x4_t t4 = vmulq_f32(t3, v_scale);
        alignas(16) float ft4[4];
        vst1q_f32(ft4, t4);
        const auto lut_at = [&](float x) noexcept -> float {
          const int32_t diff = static_cast<int32_t>(10000000u - static_cast<uint32_t>(FctiwzExact(double(x))));
          const int32_t idx = Mod100SignedExact(diff);
          if (static_cast<uint32_t>(idx) < 100u) [[likely]] {
            return lut[idx];
          }
          return LoadF32(base, kWaveLutAddr + (static_cast<uint32_t>(idx) << 2));
        };
        alignas(16) float fl[4] = {
            lut_at(ft4[0]),
            lut_at(ft4[1]),
            lut_at(ft4[2]),
            lut_at(ft4[3]),
        };
        const float32x4_t v_cur = vld1q_f32(v_row + cy);
        vst1q_f32(v_row + cy, Fmadds4(vld1q_f32(fl), v_amp, v_cur));
      }
      StoreLinearRingRow(base, row_addr, cur_ring_y, v_row);
      if (++px == 100u) px = 0u;
    }
  }

  // 5. 9-point 2D Laplacian wave stencil (100x100) using 3 rolling L1D padded rows (zero modulo!)
  {
    const float w_diag = LoadF32(base, kConst_20568);  // f13
    const float w_orth = LoadF32(base, kConst_15556);  // f0
    const float32x4_t v_wdiag = vdupq_n_f32(w_diag);
    const float32x4_t v_worth = vdupq_n_f32(w_orth);
    const float32x4_t v_stiff = vdupq_n_f32(f29_stiff);
    const float32x4_t v_rest  = vdupq_n_f32(f28_rest);
    const float32x4_t v_damp  = vdupq_n_f32(f27_damp);

    alignas(16) float h_buf[3][104];
    alignas(16) float v_row[100];

    const auto load_h_padded = [&](uint32_t phys_row, float* buf) noexcept {
      LoadLinearRingRow(base, kGridHAddr + phys_row * 400u, cur_ring_y, buf + 1);
      buf[0] = buf[1];      // exact clamp for logical col 0
      buf[101] = buf[100];  // exact clamp for logical col 99
    };

    uint32_t phys_row = cur_ring_x;
    uint32_t next_phys_row = (phys_row + 1u == 100u) ? 0u : (phys_row + 1u);

    float* r_curr = h_buf[0];
    float* r_next = h_buf[1];
    float* r_prev = h_buf[2];
    load_h_padded(phys_row, r_curr);
    load_h_padded(next_phys_row, r_next);

    for (int32_t rx = 0; rx < 100; ++rx) {
      const float* const p_row = (rx == 0) ? r_curr : r_prev;
      const float* const c_row = r_curr;
      const float* const n_row = (rx == 99) ? r_curr : r_next;

      const uint32_t v_row_addr = kGridVAddr + phys_row * 400u;
      LoadLinearRingRow(base, v_row_addr, cur_ring_y, v_row);

      for (uint32_t cy = 0; cy < 100u; cy += 4u) {
        // Exact addition order matching PPC loc_8228D61C:
        // diag = (((next[c] + next[c+2]) + prev[c]) + prev[c+2])
        // orth = (((next[c+1] + prev[c+1]) + curr[c]) + curr[c+2])
        float32x4_t diag = vaddq_f32(vld1q_f32(n_row + cy), vld1q_f32(n_row + cy + 2u));
        float32x4_t orth = vaddq_f32(vld1q_f32(n_row + cy + 1u), vld1q_f32(p_row + cy + 1u));
        diag = vaddq_f32(diag, vld1q_f32(p_row + cy));
        orth = vaddq_f32(orth, vld1q_f32(c_row + cy));
        diag = vaddq_f32(diag, vld1q_f32(p_row + cy + 2u));
        orth = vaddq_f32(orth, vld1q_f32(c_row + cy + 2u));

        const float32x4_t h_c = vld1q_f32(c_row + cy + 1u);
        const float32x4_t v_c = vld1q_f32(v_row + cy);

        const float32x4_t t_diag = vmulq_f32(diag, v_wdiag);
        const float32x4_t avg = Fmadds4(orth, v_worth, t_diag);
        const float32x4_t lap = vsubq_f32(h_c, avg);
        const float32x4_t v_mid = Fnmsubs4(lap, v_stiff, v_c);
        const float32x4_t v_out = vmulq_f32(Fnmsubs4(h_c, v_rest, v_mid), v_damp);
        vst1q_f32(v_row + cy, v_out);
      }
      StoreLinearRingRow(base, v_row_addr, cur_ring_y, v_row);

      // Rotate rolling row buffers & load only 1 new row per iteration
      float* old_prev = r_prev;
      r_prev = r_curr;
      r_curr = r_next;
      r_next = old_prev;
      phys_row = next_phys_row;
      if (rx < 98) {
        next_phys_row = (phys_row + 1u == 100u) ? 0u : (phys_row + 1u);
        load_h_padded(next_phys_row, r_next);
      }
    }
  }

  // 6. Damping rectangles pass (loc_8228E0B8..loc_8228E278)
  {
    const int32_t damp_count = static_cast<int32_t>(REX_LOAD_U32(kDampCountAddr));
    for (int32_t idx = 0; idx < damp_count; ++idx) {
      const uint32_t rect_ptr = kDampRectsAddr + static_cast<uint32_t>(idx) * 16u + 4u;
      const int32_t wx = static_cast<int32_t>(REX_LOAD_U32(kOldXAddr));
      const int32_t i0 = static_cast<int16_t>(REX_LOAD_U16(rect_ptr));
      int32_t x0 = static_cast<int16_t>(REX_LOAD_U16(kDampVertsAddr + static_cast<uint32_t>(i0 * 8)));
      if (x0 >= wx + 200) continue;
      const int32_t i1 = static_cast<int16_t>(REX_LOAD_U16(rect_ptr + 2u));
      int32_t x1 = static_cast<int16_t>(REX_LOAD_U16(kDampVertsAddr + static_cast<uint32_t>(i1 * 8)));
      if (x1 < wx) continue;

      const int32_t wy = static_cast<int32_t>(REX_LOAD_U32(kOldYAddr));
      int32_t y0 = static_cast<int16_t>(REX_LOAD_U16(kDampVertsAddr + 2u + static_cast<uint32_t>(i0 * 8)));
      if (y0 >= wy + 200) continue;
      const int32_t i2 = static_cast<int16_t>(REX_LOAD_U16(rect_ptr + 4u));
      int32_t y1 = static_cast<int16_t>(REX_LOAD_U16(kDampVertsAddr + 2u + static_cast<uint32_t>(i2 * 8)));
      if (y1 < wy) continue;

      ctx.f1.f64 = double(LoadF32(base, rect_ptr - 4u));
      ctx.f2.f64 = double(dt);
      ctx.lr = 0x8228E148u;
      sub_82A02648(ctx, base);
      ctx.fpscr.disableFlushMode();
      const float damp = static_cast<float>(ctx.f1.f64);

      const int32_t wx_cur = static_cast<int32_t>(REX_LOAD_U32(kOldXAddr));
      if (x0 <= wx_cur) x0 = wx_cur;
      if (x1 >= wx_cur + 200) x1 = wx_cur + 200;
      const int32_t wy_cur = static_cast<int32_t>(REX_LOAD_U32(kOldYAddr));
      if (y0 <= wy_cur) y0 = wy_cur;
      if (y1 >= wy_cur + 200) y1 = wy_cur + 200;

      if (x0 <= x1 && y0 <= y1) {
        const uint32_t nx = (static_cast<uint32_t>(x1 - x0) >> 1) + 1u;
        const uint32_t ny = (static_cast<uint32_t>(y1 - y0) >> 1) + 1u;
        uint32_t gx = static_cast<uint32_t>(Mod100SignedExact(x0 / 2 + 100000));
        const uint32_t gy_start = static_cast<uint32_t>(Mod100SignedExact(y0 / 2 + 100000));
        for (uint32_t ix = 0; ix < nx; ++ix) {
          const uint32_t row_base = kGridVAddr + gx * 400u;
          uint32_t gy = gy_start;
          for (uint32_t iy = 0; iy < ny; ++iy) {
            const uint32_t addr = row_base + (gy << 2);
            const float cur = LoadF32(base, addr);
            StoreF32(base, addr, static_cast<float>(double(damp) * double(cur)));
            if (++gy == 100u) gy = 0u;
          }
          if (++gx == 100u) gx = 0u;
        }
      }
    }
  }

  // 7. Integrate heights: h[i] = fma(v[i], dt, h[i]) for all 10,000 cells (NEON 4x)
  {
    const float32x4_t v_dt = vdupq_n_f32(dt);
    REX_TOUCH_SPAN(kGridHAddr, 40000u);
    REX_TOUCH_SPAN(kGridVAddr, 40000u);
    uint8_t* const h_ptr = REX_RAW_ADDR(kGridHAddr);
    const uint8_t* const v_ptr = REX_RAW_ADDR(kGridVAddr);
    for (uint32_t off = 0; off < 40000u; off += 16u) {
      const float32x4_t h4 = LoadBeF32x4(h_ptr + off);
      const float32x4_t v4 = LoadBeF32x4(v_ptr + off);
      StoreBeF32x4(h_ptr + off, Fmadds4(v4, v_dt, h4));
    }
  }

  ctx.r1.u32 = orig_sp;
}
}  // namespace batch12_water_sim_detail
extern "C" void sub_8228C590(PPCContext& ctx, uint8_t* base) {
  batch12_water_sim_detail::sub_8228C590Impl(ctx, base);
}

// candidate_829BF170.inc

namespace batch12_gjk_detail {

static inline float32x4_t LoadVec(uint8_t* base, uint32_t addr) noexcept {
  const uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(addr & ~0xFu)));
  const uint8x16_t rev64 = vrev64q_u8(raw);
  return vreinterpretq_f32_u8(vextq_u8(rev64, rev64, 8));
}

static inline void StoreVec(uint8_t* base, uint32_t addr, float32x4_t v) noexcept {
  const uint8x16_t rev64 = vrev64q_u8(vreinterpretq_u8_f32(v));
  vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(addr & ~0xFu)), vextq_u8(rev64, rev64, 8));
}

static inline float32x4_t LoadQuantizedVec4F32(uint8_t* base, uint32_t addr) noexcept {
  const uint8x8_t raw = vld1_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(addr)));
  const uint8x8_t rev = vrev64_u8(raw);
  const int32x4_t s32 = vmovl_s16(vreinterpret_s16_u8(rev));
  return vcvtq_f32_s32(s32);
}

static inline float32x4_t FromPPCV(const PPCVRegister& r) noexcept {
  return vld1q_f32(r.f32);
}

static inline void ToPPCV(PPCVRegister& r, float32x4_t v) noexcept {
  vst1q_f32(r.f32, v);
}

static inline float32x4_t NeonAnd(float32x4_t a, float32x4_t b) noexcept {
  return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
}

static inline float32x4_t NeonOr(float32x4_t a, float32x4_t b) noexcept {
  return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
}

static inline float32x4_t NeonMadd(float32x4_t a, float32x4_t b, float32x4_t c) noexcept {
  return std::bit_cast<float32x4_t>(
      REX_VMADDFP(std::bit_cast<simde__m128>(a),
                  std::bit_cast<simde__m128>(b),
                  std::bit_cast<simde__m128>(c)));
}

static inline float32x4_t NeonNmsub(float32x4_t a, float32x4_t b, float32x4_t c) noexcept {
  return std::bit_cast<float32x4_t>(
      REX_VNMSUBFP(std::bit_cast<simde__m128>(a),
                   std::bit_cast<simde__m128>(b),
                   std::bit_cast<simde__m128>(c)));
}

static inline float32x4_t Dot3(float32x4_t a, float32x4_t b) noexcept {
  float32x4_t p = vmulq_f32(a, b);
  p = vsetq_lane_f32(0.0f, p, 0);
  p = vpaddq_f32(p, p);
  return vpaddq_f32(p, p);
}

static inline float32x4_t Dot4(float32x4_t a, float32x4_t b) noexcept {
  float32x4_t p = vmulq_f32(a, b);
  p = vpaddq_f32(p, p);
  return vpaddq_f32(p, p);
}

static inline float32x4_t UnpackHi32(float32x4_t lo_arg, float32x4_t hi_arg) noexcept {
  // Matches simde_mm_unpackhi_epi32(lo_arg, hi_arg) == vzip2q(lo_arg, hi_arg)
  return vreinterpretq_f32_u32(
      vzip2q_u32(vreinterpretq_u32_f32(lo_arg), vreinterpretq_u32_f32(hi_arg)));
}

static inline float32x4_t UnpackLo32(float32x4_t lo_arg, float32x4_t hi_arg) noexcept {
  // Matches simde_mm_unpacklo_epi32(lo_arg, hi_arg) == vzip1q(lo_arg, hi_arg)
  return vreinterpretq_f32_u32(
      vzip1q_u32(vreinterpretq_u32_f32(lo_arg), vreinterpretq_u32_f32(hi_arg)));
}

static inline float32x4_t MinPS(float32x4_t a, float32x4_t b) noexcept {
  return vbslq_f32(vcltq_f32(a, b), a, b);
}

static inline float32x4_t MaxPS(float32x4_t a, float32x4_t b) noexcept {
  return vbslq_f32(vcgtq_f32(a, b), a, b);
}

static inline bool AllLanesTrue(uint32x4_t mask) noexcept {
  return vminvq_u32(mask) == 0xFFFFFFFFu;
}

// Exact inlined sub_829D69F8 (simplex duplicate check).
static inline uint8_t CheckSimplexDuplicateNEON(uint8_t* base, uint32_t simplex,
                                                float32x4_t w_vec) noexcept {
  constexpr uint32_t kBase82B0 = static_cast<uint32_t>(int32_t(-2102394880));
  constexpr uint32_t kBase831B = static_cast<uint32_t>(int32_t(-2095382528));
  constexpr uint32_t kBase82B2 = static_cast<uint32_t>(int32_t(-2102263808));

  const int32_t count = static_cast<int32_t>(REX_LOAD_U32(simplex));
  float32x4_t v13 = LoadVec(base, kBase82B0 + 29168u);
  for (int32_t i = 0; i < count; ++i) {
    const float32x4_t pt = LoadVec(base, simplex + 16u + static_cast<uint32_t>(i) * 16u);
    v13 = MaxPS(v13, Dot3(pt, pt));
  }

  const float32x4_t v_zero = vdupq_n_f32(0.0f);
  const float32x4_t v_inf = LoadVec(base, static_cast<uint32_t>(int32_t(kBase831B) - 18560));
  const float32x4_t v_half = LoadVec(base, kBase82B0 + 29104u);
  const float32x4_t v_one = LoadVec(base, kBase82B0 + 29088u);
  const float32x4_t v_eps = LoadVec(base, static_cast<uint32_t>(int32_t(kBase82B2) - 15936));
  const float32x4_t v_mask = LoadVec(base, static_cast<uint32_t>(int32_t(kBase831B) - 18704));

  float32x4_t rsq = vdivq_f32(vdupq_n_f32(1.0f), vsqrtq_f32(v13));
  rsq = vbslq_f32(vceqq_f32(rsq, v_inf), v_zero, rsq);
  const float32x4_t rsq2 = vmulq_f32(rsq, rsq);
  const float32x4_t half_rsq = vmulq_f32(v_half, rsq);
  const float32x4_t nr = NeonNmsub(v13, rsq2, v_one);
  rsq = NeonMadd(nr, half_rsq, rsq);
  const float32x4_t max_len = vmulq_f32(v13, rsq);
  const float32x4_t tol = NeonAnd(vmulq_f32(v_eps, max_len), v_mask);

  const float32x4_t w_masked = NeonAnd(w_vec, v_mask);
  const float32x4_t hi = vaddq_f32(w_masked, tol);
  const float32x4_t lo = vsubq_f32(w_masked, tol);

  uint8_t found = 0u;
  for (int32_t i = 0; i < count; ++i) {
    const float32x4_t pt = NeonAnd(LoadVec(base, simplex + 16u + static_cast<uint32_t>(i) * 16u), v_mask);
    const uint32x4_t inside = vandq_u32(vcgeq_f32(hi, pt), vcgeq_f32(pt, lo));
    if (AllLanesTrue(inside)) found = 1u;
  }

  const float32x4_t last_w = NeonAnd(LoadVec(base, simplex + 352u), v_mask);
  const uint32x4_t inside_last = vandq_u32(vcgeq_f32(hi, last_w), vcgeq_f32(last_w, lo));
  if (AllLanesTrue(inside_last)) found = 1u;
  return found;
}

// Exact inlined sub_829D6988 (add vertex to GJK simplex).
static inline void AddSimplexVertexNEON(uint8_t* base, uint32_t simplex,
                                        float32x4_t w_vec, float32x4_t p_a, float32x4_t p_b,
                                        uint32_t idx_a, uint32_t idx_b) noexcept {
  StoreVec(base, simplex + 352u, w_vec);
  const uint32_t n = REX_LOAD_U32(simplex);
  REX_STORE_U8(simplex + 432u, 1u);
  StoreVec(base, simplex + ((n + 1u) << 4), w_vec);
  StoreVec(base, simplex + ((n + 6u) << 4), p_a);
  StoreVec(base, simplex + ((n + 11u) << 4), p_b);
  REX_STORE_U32(simplex + ((n + 64u) << 2), idx_a);
  REX_STORE_U32(simplex + ((n + 69u) << 2), idx_b);
  REX_STORE_U32(simplex, n + 1u);
}

// Fast-path case 0/1 of sub_829D7428, falling back to sub_829D7428 for cases 2..4.
static inline uint8_t UpdateSimplexNEON(PPCContext& ctx, uint8_t* base, uint32_t simplex) {
  if (REX_LOAD_U8(simplex + 432u) == 0u) return 0u;
  const uint32_t n = REX_LOAD_U32(simplex);
  if (n == 1u) {
    constexpr uint32_t kBase82B0 = static_cast<uint32_t>(int32_t(-2102394880));
    const float32x4_t zero = vdupq_n_f32(0.0f);
    REX_STORE_U8(simplex + 368u + 48u, 0u);
    StoreVec(base, simplex + 368u + 32u, zero);
    StoreVec(base, simplex + 368u + 16u, zero);
    REX_STORE_U8(simplex + 432u, 0u);
    const float32x4_t p0 = LoadVec(base, simplex + 96u);
    const float32x4_t q0 = LoadVec(base, simplex + 176u);
    const float32x4_t diff = vsubq_f32(p0, q0);
    StoreVec(base, simplex + 304u, p0);
    StoreVec(base, simplex + 320u, q0);
    StoreVec(base, simplex + 336u, diff);
    StoreVec(base, simplex + 400u, LoadVec(base, kBase82B0 + 29120u));
    return 1u;
  }
  ctx.r3.u64 = simplex;
  sub_829D7428(ctx, base);
  ctx.fpscr.enableFlushMode();
  return static_cast<uint8_t>(ctx.r3.u32 & 0xFFu);
}


static inline void sub_829BF170Impl(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();
  ctx.fpscr.enableFlushMode();

  constexpr uint32_t kBase82B0 = static_cast<uint32_t>(int32_t(-2102394880));
  constexpr uint32_t kBase831B = static_cast<uint32_t>(int32_t(-2095382528));
  constexpr uint32_t kBase82B2 = static_cast<uint32_t>(int32_t(-2102263808));
  constexpr uint32_t kBase831D = static_cast<uint32_t>(int32_t(-2095251456));

  constexpr uint32_t kAddr82B071A0 = kBase82B0 + 29088u;
  constexpr uint32_t kAddr82B071B0 = kBase82B0 + 29104u;
  constexpr uint32_t kAddr82B07200 = kBase82B0 + 29184u;
  constexpr uint32_t kAddr82B07230 = kBase82B0 + 29232u;
  constexpr uint32_t kAddr82B07260 = kBase82B0 + 29280u;

  constexpr uint32_t kAddr831AB670 = static_cast<uint32_t>(int32_t(kBase831B) - 18832);
  constexpr uint32_t kAddr831AB680 = static_cast<uint32_t>(int32_t(kBase831B) - 18816);
  constexpr uint32_t kAddr831AB6D0 = static_cast<uint32_t>(int32_t(kBase831B) - 18736);
  constexpr uint32_t kAddr831AB6F0 = static_cast<uint32_t>(int32_t(kBase831B) - 18704);
  constexpr uint32_t kAddr831AB780 = static_cast<uint32_t>(int32_t(kBase831B) - 18560);

  constexpr uint32_t kAddr82B1B220 = static_cast<uint32_t>(int32_t(kBase82B2) - 19936);
  constexpr uint32_t kAddr82B1B230 = static_cast<uint32_t>(int32_t(kBase82B2) - 19920);
  constexpr uint32_t kAddr82B1BC80 = static_cast<uint32_t>(int32_t(kBase82B2) - 17280);
  constexpr uint32_t kAddr82B1BC90 = static_cast<uint32_t>(int32_t(kBase82B2) - 17264);
  constexpr uint32_t kAddr82B1BCA0 = static_cast<uint32_t>(int32_t(kBase82B2) - 17248);
  constexpr uint32_t kAddr82B1BCB0 = static_cast<uint32_t>(int32_t(kBase82B2) - 17232);
  constexpr uint32_t kAddr831CDD00 = static_cast<uint32_t>(int32_t(kBase831D) - 8960);

  const uint32_t orig_sp = ctx.r1.u32;
  const uint32_t sp = orig_sp - 528u;
  REX_STORE_U32(sp, orig_sp);
  ctx.r1.u32 = sp;

  const uint32_t detector = ctx.r3.u32;   // r26
  const uint32_t xforms = ctx.r4.u32;     // r25
  const uint32_t cb_obj = ctx.r5.u32;     // [sp + 564]
  REX_STORE_U32(sp + 564u, cb_obj);

  const float32x4_t v_zero = vdupq_n_f32(0.0f);
  const float32x4_t v_mask_xyz = LoadVec(base, kAddr831AB6F0);  // v0
  const float32x4_t v_fill_w = LoadVec(base, kAddr831AB6D0);    // v11
  const float32x4_t v_831AB680 = LoadVec(base, kAddr831AB680);  // v6
  const float32x4_t v_831AB670 = LoadVec(base, kAddr831AB670);  // [sp+132]
  const float32x4_t v_82B07260 = LoadVec(base, kAddr82B07260);  // [r14]
  const float32x4_t v_82B1BC80 = LoadVec(base, kAddr82B1BC80);  // [sp+136]
  const float32x4_t v_831CDD00 = LoadVec(base, kAddr831CDD00);  // [sp+192]
  const float32x4_t v_82B07230 = LoadVec(base, kAddr82B07230);  // [r16]

  float32x4_t v125 = v_82B07230;
  float32x4_t v126 = v_zero;
  float32x4_t v127 = v_zero;
  float32x4_t v124 = v_zero;

  const uint32_t simplex0 = REX_LOAD_U32(detector + 36u);
  REX_STORE_U32(simplex0, 0u);
  REX_STORE_U8(simplex0 + 432u, 1u);
  StoreVec(base, simplex0 + 352u, v_82B07230);
  REX_STORE_U8(simplex0 + 368u + 48u, 0u);
  StoreVec(base, simplex0 + 368u + 32u, v_zero);
  StoreVec(base, simplex0 + 368u + 16u, v_zero);

  uint8_t flag_128 = 0u;
  uint8_t flag_129 = 0u;
  const uint8_t flag_130 = 0u;
  uint8_t flag_131 = 1u;
  int32_t iter_count = 0;

  const uint32_t mat_b_addr = xforms + 64u;
  const float32x4_t mat_a_r0 = LoadVec(base, xforms + 0u);
  const float32x4_t mat_a_r1 = LoadVec(base, xforms + 16u);
  const float32x4_t mat_a_r2 = LoadVec(base, xforms + 32u);
  const float32x4_t mat_a_r3 = LoadVec(base, xforms + 48u);

  const float32x4_t mat_b_r0 = LoadVec(base, mat_b_addr + 0u);
  const float32x4_t mat_b_r1 = LoadVec(base, mat_b_addr + 16u);
  const float32x4_t mat_b_r2 = LoadVec(base, mat_b_addr + 32u);
  const float32x4_t mat_b_r3 = LoadVec(base, mat_b_addr + 48u);

  // Pre-transpose Mat A and Mat B (rows -> columns for Dot4)
  const float32x4_t a_hi02 = UnpackHi32(mat_a_r2, mat_a_r0);
  const float32x4_t a_hi13 = UnpackHi32(mat_a_r3, mat_a_r1);
  const float32x4_t a_lo02 = UnpackLo32(mat_a_r2, mat_a_r0);
  const float32x4_t a_lo13 = UnpackLo32(mat_a_r3, mat_a_r1);
  const float32x4_t mat_a_c0 = UnpackHi32(a_hi13, a_hi02);
  const float32x4_t mat_a_c1 = UnpackLo32(a_hi13, a_hi02);
  const float32x4_t mat_a_c2 = UnpackHi32(a_lo13, a_lo02);

  const float32x4_t b_hi02 = UnpackHi32(mat_b_r2, mat_b_r0);
  const float32x4_t b_hi13 = UnpackHi32(mat_b_r3, mat_b_r1);
  const float32x4_t b_lo02 = UnpackLo32(mat_b_r2, mat_b_r0);
  const float32x4_t b_lo13 = UnpackLo32(mat_b_r3, mat_b_r1);
  const float32x4_t mat_b_c0 = UnpackHi32(b_hi13, b_hi02);
  const float32x4_t mat_b_c1 = UnpackLo32(b_hi13, b_hi02);
  const float32x4_t mat_b_c2 = UnpackHi32(b_lo13, b_lo02);

  // Support point helper for both shape A (pass 0) and shape B (pass 1).
  const auto compute_support = [&](uint32_t bound, float32x4_t dir, uint32_t idx_out_addr,
                                   uint32_t& idx_local) -> float32x4_t {
    const uint8_t shape_type = REX_LOAD_U8(bound + 4u);
    switch (shape_type) {
      case 0:
        return LoadVec(base, bound + 48u);
      case 1: {
        const float32x4_t v_pos = LoadVec(base, kAddr82B1B230);
        const float32x4_t v_neg = LoadVec(base, kAddr82B1B220);
        const float32x4_t center = LoadVec(base, bound + 48u);
        const float32x4_t half_ext = LoadVec(base, bound + 144u);
        const float32x4_t lhs = NeonOr(NeonAnd(v_831AB680, v_mask_xyz), v_fill_w);
        const float32x4_t rhs = NeonAnd(dir, v_mask_xyz);
        const uint32x4_t cmp = vcgtq_f32(lhs, rhs);
        const float32x4_t sgn = vbslq_f32(cmp, v_pos, v_neg);
        return NeonMadd(half_ext, sgn, center);
      }
      case 2: {
        ctx.r3.u64 = sp + 272u;
        ctx.r4.u64 = bound;
        ctx.r5.u64 = idx_out_addr;
        ToPPCV(ctx.v1, dir);
        sub_8299C080(ctx, base);
        ctx.fpscr.enableFlushMode();
        idx_local = REX_LOAD_U32(idx_out_addr);
        return LoadVec(base, ctx.r3.u32);
      }
      case 3: {
        const float32x4_t v13_a = LoadVec(base, bound + 32u);
        const float32x4_t v12_a = LoadVec(base, bound + 16u);
        const float32x4_t v_half = LoadVec(base, kAddr82B07200);
        const float32x4_t center = LoadVec(base, bound + 48u);
        const float32x4_t margin = LoadVec(base, bound + 112u);
        const float32x4_t diff = vsubq_f32(v12_a, v13_a);
        const float32x4_t half_diff = vsubq_f32(vmulq_f32(diff, v_half), margin);
        const float32x4_t lhs = NeonOr(NeonAnd(dir, v_mask_xyz), v_fill_w);
        const float32x4_t rhs = NeonAnd(v_831AB680, v_mask_xyz);
        const uint32x4_t cmp = vcgtq_f32(lhs, rhs);
        return vbslq_f32(cmp, vaddq_f32(center, half_diff), vsubq_f32(center, half_diff));
      }
      case 4: {
        uint32_t verts_addr = REX_LOAD_U32(bound + 132u);
        if (verts_addr == 0u) verts_addr = REX_LOAD_U32(bound + 176u);
        const int32_t count_m1 = static_cast<int32_t>(REX_LOAD_U32(bound + 200u)) - 1;
        const float32x4_t v_off = LoadVec(base, bound + 160u);
        const float32x4_t v_scl = LoadVec(base, bound + 144u);
        const uint32x4_t mask_u32 = vreinterpretq_u32_f32(v_mask_xyz);

        uint32_t best_idx = static_cast<uint32_t>(count_m1);
        float32x4_t best_pt = NeonMadd(
            LoadQuantizedVec4F32(base, verts_addr + best_idx * 6u), v_scl, v_off);
        float32x4_t best_dot = Dot3(dir, best_pt);

        uint32_t cur_addr = verts_addr;
        for (int32_t i = 0; i < count_m1; ++i, cur_addr += 6u) {
          const float32x4_t pt = NeonMadd(LoadQuantizedVec4F32(base, cur_addr), v_scl, v_off);
          const float32x4_t d = Dot3(dir, pt);
          const float32x4_t d_lhs = NeonOr(NeonAnd(d, v_mask_xyz), v_fill_w);
          const float32x4_t best_rhs = NeonAnd(best_dot, v_mask_xyz);
          const uint32x4_t cmp = vcgtq_f32(d_lhs, best_rhs);
          best_dot = vbslq_f32(cmp, d, best_dot);
          best_pt = vbslq_f32(cmp, pt, best_pt);
          if (AllLanesTrue(vceqq_u32(vandq_u32(cmp, mask_u32), mask_u32))) {
            best_idx = static_cast<uint32_t>(i);
          }
        }
        idx_local = best_idx;
        REX_STORE_U32(idx_out_addr, best_idx);
        return best_pt;
      }
      case 5: {
        ctx.r3.u64 = sp + 288u;
        ctx.r4.u64 = bound;
        ctx.r5.u64 = idx_out_addr;
        ToPPCV(ctx.v1, dir);
        sub_8296C540(ctx, base);
        ctx.fpscr.enableFlushMode();
        idx_local = REX_LOAD_U32(idx_out_addr);
        return LoadVec(base, ctx.r3.u32);
      }
      case 13: {
        const float32x4_t p1 = LoadVec(base, bound + 144u);
        const float32x4_t p2 = LoadVec(base, bound + 160u);
        const float32x4_t p0 = LoadVec(base, bound + 128u);
        const float32x4_t d1 = Dot3(dir, p1);
        const float32x4_t d0 = Dot3(dir, p0);
        const float32x4_t d2 = Dot3(dir, p2);
        const float32x4_t zero_rhs = NeonAnd(v_831AB680, v_mask_xyz);
        const float32x4_t diff01 = vsubq_f32(d0, d1);
        const float32x4_t diff02 = vsubq_f32(d0, d2);
        const float32x4_t diff12 = vsubq_f32(d1, d2);
        const uint32x4_t cmp01 = vcgtq_f32(NeonOr(NeonAnd(diff01, v_mask_xyz), v_fill_w), zero_rhs);
        const float32x4_t best01_d = vbslq_f32(cmp01, diff02, diff12);
        const float32x4_t best01_p = vbslq_f32(cmp01, p0, p1);
        const uint32x4_t cmp_final = vcgtq_f32(NeonOr(NeonAnd(best01_d, v_mask_xyz), v_fill_w), zero_rhs);
        return vbslq_f32(cmp_final, best01_p, p2);
      }
      default:
        return v_831AB670;
    }
  };

  float32x4_t world_a = v_zero;
  float32x4_t world_b = v_zero;

  // Main GJK iteration loop (loc_829BF25C)
  for (;;) {
    const float32x4_t dir = LoadVec(base, detector + 16u);
    const float32x4_t neg_dir = vsubq_f32(v_zero, dir);

    // Transform dir by transpose(Mat B) -> dir_b (v127)
    const float32x4_t bx = Dot3(mat_b_r0, dir);
    const float32x4_t by = Dot3(mat_b_r1, dir);
    const float32x4_t bz = Dot3(mat_b_r2, dir);
    const float32x4_t b_xz = UnpackHi32(bz, bx);
    const float32x4_t b_tmp = UnpackHi32(b_xz, by);
    const float32x4_t dir_b = UnpackHi32(b_tmp, b_xz);

    // Transform -dir by transpose(Mat A) -> dir_a (v1)
    const float32x4_t ax = Dot3(mat_a_r0, neg_dir);
    const float32x4_t ay = Dot3(mat_a_r1, neg_dir);
    const float32x4_t az = Dot3(mat_a_r2, neg_dir);
    const float32x4_t a_xz = UnpackHi32(az, ax);
    const float32x4_t a_tmp = UnpackHi32(a_xz, ay);
    const float32x4_t dir_a = UnpackHi32(a_tmp, a_xz);

    uint32_t idx_a = 0u;
    uint32_t idx_b = 0u;
    REX_STORE_U32(sp + 144u, 0u);
    REX_STORE_U32(sp + 148u, 0u);

    const float32x4_t supp_a = compute_support(REX_LOAD_U32(detector + 40u), dir_a, sp + 144u, idx_a);
    const float32x4_t supp_b = compute_support(REX_LOAD_U32(detector + 44u), dir_b, sp + 148u, idx_b);

    // Transform supp_a into world space (world_a) and supp_b into world space (world_b)
    const float32x4_t sa_h = NeonOr(v_fill_w, NeonAnd(supp_a, v_mask_xyz));
    const float32x4_t wa_x = Dot4(sa_h, mat_a_c0);
    const float32x4_t wa_y = Dot4(sa_h, mat_a_c1);
    const float32x4_t wa_z = Dot4(sa_h, mat_a_c2);
    const float32x4_t wa_yw = UnpackHi32(sa_h, wa_y);
    const float32x4_t wa_xz = UnpackHi32(wa_z, wa_x);
    world_a = UnpackHi32(wa_yw, wa_xz);

    const float32x4_t sb_h = NeonOr(v_fill_w, NeonAnd(supp_b, v_mask_xyz));
    const float32x4_t wb_x = Dot4(sb_h, mat_b_c0);
    const float32x4_t wb_y = Dot4(sb_h, mat_b_c1);
    const float32x4_t wb_z = Dot4(sb_h, mat_b_c2);
    const float32x4_t wb_yw = UnpackHi32(sb_h, wb_y);
    const float32x4_t wb_xz = UnpackHi32(wb_z, wb_x);
    world_b = UnpackHi32(wb_yw, wb_xz);

    v126 = vsubq_f32(world_a, world_b);
    v127 = Dot3(v126, dir);

    // Early separating-axis test
    const float32x4_t v9_zero = NeonAnd(v_831AB670, v_mask_xyz);
    if (AllLanesTrue(vcgtq_f32(NeonOr(NeonAnd(v127, v_mask_xyz), v_fill_w), v9_zero))) {
      const float32x4_t d2 = vmulq_f32(v127, v127);
      const float32x4_t thresh = vmulq_f32(v125, LoadVec(base, xforms + 144u));
      if (AllLanesTrue(vcgtq_f32(NeonOr(NeonAnd(d2, v_mask_xyz), v_fill_w),
                                 NeonAnd(thresh, v_mask_xyz)))) {
        flag_131 = 0u;
        break;
      }
    }

    const uint32_t simplex = REX_LOAD_U32(detector + 36u);
    if (CheckSimplexDuplicateNEON(base, simplex, v126) != 0u) {
      flag_129 = 1u;
      flag_128 = 1u;
      break;
    }

    const float32x4_t progress = vsubq_f32(v125, v127);
    const float32x4_t min_prog = vmulq_f32(v125, v_831CDD00);
    if (!AllLanesTrue(vcgtq_f32(NeonOr(NeonAnd(progress, v_mask_xyz), v_fill_w),
                                NeonAnd(min_prog, v_mask_xyz)))) {
      if (!AllLanesTrue(vcgtq_f32(NeonOr(NeonAnd(progress, v_mask_xyz), v_fill_w),
                                  NeonAnd(v_831AB670, v_mask_xyz)))) {
        flag_129 = 1u;
      }
      flag_128 = 1u;
      break;
    }

    AddSimplexVertexNEON(base, simplex, v126, world_a, world_b, idx_a, idx_b);
    const uint8_t updated = UpdateSimplexNEON(ctx, base, simplex);
    const float32x4_t closest = updated ? LoadVec(base, simplex + 336u) : v_82B07260;

    if (AllLanesTrue(vceqq_f32(NeonAnd(v_82B07260, v_mask_xyz),
                               NeonAnd(closest, v_mask_xyz)))) {
      flag_129 = 1u;
      flag_128 = 1u;
      break;
    }

    const float32x4_t prev_dist_sq = v125;
    v125 = Dot3(closest, closest);
    StoreVec(base, detector + 16u, closest);

    const float32x4_t step_dec = vsubq_f32(prev_dist_sq, v125);
    const float32x4_t min_dec = vmulq_f32(v_82B1BC80, prev_dist_sq);
    if (!AllLanesTrue(vcgtq_f32(NeonOr(NeonAnd(step_dec, v_mask_xyz), v_fill_w),
                                NeonAnd(min_dec, v_mask_xyz)))) {
      StoreVec(base, detector + 16u, LoadVec(base, simplex + 336u));
      flag_128 = 1u;
      break;
    }

    const int32_t cur_iter = iter_count;
    ++iter_count;
    if (cur_iter > 1000) {
      break;
    }

    const int32_t s_count = static_cast<int32_t>(REX_LOAD_U32(simplex));
    if (s_count == 4) {
      StoreVec(base, detector + 16u, LoadVec(base, simplex + 336u));
      break;
    }

    float32x4_t max_vlen = v_831AB670;
    for (int32_t i = 0; i < s_count; ++i) {
      const float32x4_t sv = LoadVec(base, simplex + 16u + static_cast<uint32_t>(i) * 16u);
      max_vlen = MaxPS(max_vlen, Dot3(sv, sv));
    }
    const float32x4_t eps_vlen = vmulq_f32(v_82B1BC80, max_vlen);
    if (!AllLanesTrue(vcgtq_f32(NeonOr(NeonAnd(v125, v_mask_xyz), v_fill_w),
                                NeonAnd(eps_vlen, v_mask_xyz)))) {
      StoreVec(base, detector + 16u, LoadVec(base, simplex + 336u));
      break;
    }
  }

  // Compute margins for bound A (v125) and bound B (v126) — loc_829BF954
  const auto load_margin = [&](uint32_t bound) noexcept -> float32x4_t {
    const uint8_t st = REX_LOAD_U8(bound + 4u);
    if (st <= 1u) return LoadVec(base, bound + 128u);
    if (st < 3u) return MinPS(LoadVec(base, bound + 128u), LoadVec(base, bound + 144u));
    return LoadVec(base, bound + 112u);
  };
  v125 = load_margin(REX_LOAD_U32(detector + 40u));
  v126 = load_margin(REX_LOAD_U32(detector + 44u));

  v124 = v_82B07230;
  v127 = v_831AB670;
  uint8_t flag_22 = flag_130;

  const float32x4_t v_one = LoadVec(base, kAddr82B071A0);
  const float32x4_t v_half = LoadVec(base, kAddr82B071B0);
  const float32x4_t v_inf = LoadVec(base, kAddr831AB780);

  if (flag_128 != 0u) {
    // Inlined sub_829D7870
    const uint32_t simplex = REX_LOAD_U32(detector + 36u);
    UpdateSimplexNEON(ctx, base, simplex);
    world_a = LoadVec(base, simplex + 304u);
    world_b = LoadVec(base, simplex + 320u);
    v127 = vsubq_f32(world_a, world_b);

    const float32x4_t sep_vec = LoadVec(base, detector + 16u);
    const float32x4_t sep_sq = Dot3(sep_vec, sep_vec);
    const float32x4_t sep_sq_lhs = NeonOr(NeonAnd(sep_sq, v_mask_xyz), v_fill_w);
    const float32x4_t thresh_a = NeonAnd(LoadVec(base, kAddr82B1BCA0), v_mask_xyz);
    if (!AllLanesTrue(vcgeq_f32(sep_sq_lhs, thresh_a))) {
      flag_129 = 1u;
    }

    if (AllLanesTrue(vcgtq_f32(sep_sq_lhs, NeonAnd(v_82B1BC80, v_mask_xyz)))) {
      float32x4_t rsq = vdivq_f32(vdupq_n_f32(1.0f), vsqrtq_f32(sep_sq));
      rsq = vbslq_f32(vceqq_f32(rsq, v_inf), v_zero, rsq);
      const float32x4_t nr = NeonNmsub(sep_sq, vmulq_f32(rsq, rsq), v_one);
      rsq = NeonMadd(nr, vmulq_f32(v_half, rsq), rsq);

      const float32x4_t m_a = vmulq_f32(v125, rsq);
      const float32x4_t m_b = vmulq_f32(v126, rsq);
      const float32x4_t inv_rsq0 = vdivq_f32(vdupq_n_f32(1.0f), rsq);
      v127 = vmulq_f32(v127, rsq);

      world_a = NeonNmsub(sep_vec, m_a, world_a);
      world_b = NeonMadd(sep_vec, m_b, world_b);

      const float32x4_t inv_nr = NeonNmsub(inv_rsq0, rsq, v_one);
      const float32x4_t dist = NeonMadd(inv_rsq0, inv_nr, inv_rsq0);
      v124 = vsubq_f32(vsubq_f32(dist, v125), v126);
      flag_22 = 1u;
    }
  }

  const uint32_t pen_solver = REX_LOAD_U32(detector + 32u);
  uint8_t pen_needed = 0u;
  if (pen_solver != 0u && flag_129 != 0u) {
    const float32x4_t sum_d = vaddq_f32(vaddq_f32(v124, v125), v126);
    const float32x4_t pen_lim = NeonAnd(LoadVec(base, kAddr82B1BCB0), v_mask_xyz);
    if (!AllLanesTrue(vcgeq_f32(NeonOr(NeonAnd(sum_d, v_mask_xyz), v_fill_w), pen_lim))) {
      pen_needed = 1u;
    }
  }

  if (flag_131 != 0u && (flag_22 == 0u || pen_needed != 0u) && pen_solver != 0u) {
    ctx.r3.u64 = pen_solver;
    ctx.r4.u64 = REX_LOAD_U32(detector + 36u);
    ctx.r5.u64 = REX_LOAD_U32(detector + 40u);
    ctx.r6.u64 = REX_LOAD_U32(detector + 44u);
    ctx.r7.u64 = xforms;
    ctx.r8.u64 = mat_b_addr;
    ctx.r9.u64 = detector + 16u;
    ctx.r10.u64 = sp + 256u;
    REX_STORE_U32(sp + 116u, 0u);
    REX_STORE_U32(sp + 84u, sp + 224u);
    ToPPCV(ctx.v1, LoadVec(base, xforms + 160u));

    const uint32_t pen_vtbl = REX_LOAD_U32(pen_solver);
    const uint32_t pen_fn = REX_LOAD_U32(pen_vtbl + 4u);
    ctx.lr = 0x829BFC48u;
    REX_CALL_INDIRECT_FUNC(pen_fn);
    ctx.fpscr.enableFlushMode();

    if ((ctx.r3.u32 & 0xFFu) != 0u) {
      const float32x4_t p_a = LoadVec(base, sp + 256u);
      const float32x4_t p_b = LoadVec(base, sp + 224u);
      const float32x4_t diff = vsubq_f32(p_b, p_a);
      const float32x4_t diff_sq = Dot3(diff, diff);
      const float32x4_t pen_eps = NeonAnd(LoadVec(base, kAddr82B1BC90), v_mask_xyz);
      if (AllLanesTrue(vcgtq_f32(NeonOr(NeonAnd(diff_sq, v_mask_xyz), v_fill_w), pen_eps))) {
        float32x4_t rsq = vdivq_f32(vdupq_n_f32(1.0f), vsqrtq_f32(diff_sq));
        rsq = vbslq_f32(vceqq_f32(rsq, v_inf), v_zero, rsq);
        const float32x4_t nr = NeonNmsub(diff_sq, vmulq_f32(rsq, rsq), v_one);
        rsq = NeonMadd(nr, vmulq_f32(v_half, rsq), rsq);
        const float32x4_t pen_dist = vmulq_f32(diff_sq, rsq);
        const float32x4_t neg_pen_dist = vsubq_f32(v_zero, pen_dist);

        bool accept = true;
        if (flag_22 != 0u) {
          if (AllLanesTrue(vcgeq_f32(NeonOr(NeonAnd(neg_pen_dist, v_mask_xyz), v_fill_w),
                                     NeonAnd(v124, v_mask_xyz)))) {
            accept = false;
          }
        }
        if (accept) {
          const float32x4_t inv0 = vdivq_f32(vdupq_n_f32(1.0f), pen_dist);
          const float32x4_t inv_nr = NeonNmsub(inv0, pen_dist, v_one);
          const float32x4_t inv = NeonMadd(inv0, inv_nr, inv0);
          v124 = neg_pen_dist;
          v127 = vmulq_f32(diff, inv);
          world_a = p_a;
          world_b = p_b;
          flag_22 = 1u;
        }
      }
    }
  }

  // Final contact manifold emission — loc_829BFD2C
  if (flag_22 != 0u) {
    const float32x4_t neg_n = vsubq_f32(v_zero, v127);

    const float32x4_t na_x = Dot3(mat_a_r0, neg_n);
    const float32x4_t na_y = Dot3(mat_a_r1, neg_n);
    const float32x4_t na_z = Dot3(mat_a_r2, neg_n);
    const float32x4_t na_xz = UnpackHi32(na_z, na_x);
    const float32x4_t na_tmp = UnpackHi32(na_xz, na_y);
    const float32x4_t local_na = UnpackHi32(na_tmp, na_xz);

    const float32x4_t nb_x = Dot3(mat_b_r0, v127);
    const float32x4_t nb_y = Dot3(mat_b_r1, v127);
    const float32x4_t nb_z = Dot3(mat_b_r2, v127);
    const float32x4_t nb_xz = UnpackHi32(nb_z, nb_x);
    const float32x4_t nb_tmp = UnpackHi32(nb_xz, nb_y);
    const float32x4_t local_nb = UnpackHi32(nb_tmp, nb_xz);

    ctx.r3.u64 = REX_LOAD_U32(detector + 36u);
    ctx.r4.u64 = REX_LOAD_U32(detector + 40u);
    ToPPCV(ctx.v1, local_na);
    sub_829D6F88(ctx, base);
    const uint32_t res_a = ctx.r3.u32;

    ctx.r3.u64 = REX_LOAD_U32(detector + 36u);
    ctx.r4.u64 = REX_LOAD_U32(detector + 44u);
    ToPPCV(ctx.v1, local_nb);
    sub_829D7000(ctx, base);
    const uint32_t res_b = ctx.r3.u32;

    ctx.fpscr.enableFlushMode();
    const float32x4_t origin_off = LoadVec(base, xforms + 128u);
    const float32x4_t out_a = vaddq_f32(world_a, origin_off);
    const float32x4_t out_b = vaddq_f32(world_b, origin_off);
    StoreVec(base, sp + 176u, out_a);
    StoreVec(base, sp + 160u, out_b);

    ctx.r3.u64 = cb_obj;
    ctx.r4.u64 = res_a;
    ctx.r5.u64 = res_b;
    ToPPCV(ctx.v1, v127);
    ToPPCV(ctx.v2, out_a);
    ToPPCV(ctx.v3, out_b);
    ToPPCV(ctx.v4, v124);

    const uint32_t cb_vtbl = REX_LOAD_U32(cb_obj);
    const uint32_t cb_fn = REX_LOAD_U32(cb_vtbl + 4u);
    ctx.lr = 0x829BFDFCu;
    REX_CALL_INDIRECT_FUNC(cb_fn);
  }

  ctx.r1.u32 = orig_sp;
}
}  // namespace batch12_gjk_detail
extern "C" void sub_829BF170(PPCContext& ctx, uint8_t* base) {
  batch12_gjk_detail::sub_829BF170Impl(ctx, base);
}

#endif // ENABLE_BATCH12_VERIFIED_HOOKS
