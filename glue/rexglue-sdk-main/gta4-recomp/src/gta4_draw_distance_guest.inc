#pragma once

// Derived by tools/derive_draw_distance_hooks.py --write.
// Complete retail PPC logic; only the documented operand below is replaced.
// Hand-optimized for ARM64 NEON, inlined sub_821D0478/sub_823B32E0, and
// non-volatile L1-cached guest loads with bit-exact floating-point parity.

namespace scan_fast {

inline uint8_t FastLoadU8(uint8_t* base, uint32_t address) {
  return *GuestPointer(base, address);
}

inline uint16_t FastLoadU16(uint8_t* base, uint32_t address) {
  uint16_t raw;
  std::memcpy(&raw, GuestPointer(base, address), sizeof(raw));
  return __builtin_bswap16(raw);
}

inline uint32_t FastLoadU32(uint8_t* base, uint32_t address) {
  uint32_t raw;
  std::memcpy(&raw, GuestPointer(base, address), sizeof(raw));
  return __builtin_bswap32(raw);
}

inline float FastLoadF32(uint8_t* base, uint32_t address) {
  return std::bit_cast<float>(FastLoadU32(base, address));
}

inline void FastStoreU8(uint8_t* base, uint32_t address, uint8_t value) {
  *GuestPointer(base, address) = value;
}

inline void FastStoreU16(uint8_t* base, uint32_t address, uint16_t value) {
  const uint16_t raw = __builtin_bswap16(value);
  std::memcpy(GuestPointer(base, address), &raw, sizeof(raw));
}

inline void FastStoreU32(uint8_t* base, uint32_t address, uint32_t value) {
  const uint32_t raw = __builtin_bswap32(value);
  std::memcpy(GuestPointer(base, address), &raw, sizeof(raw));
}

inline void FastStoreF32(uint8_t* base, uint32_t address, float value) {
  FastStoreU32(base, address, std::bit_cast<uint32_t>(value));
}

struct RdataConsts {
  float f26;  // 0x820BBDC4
  float f24;  // 0x82000DC8
  float f16;  // 0x82000D74
  float f27;  // 0x82000D48
  float f20;  // 0x82019670
  float f21;  // 0x8200251C
  float f22;  // 0x82002518
  float f17;  // 0x82000A34
  float f18;  // 0x820BBCD8
  float f28;  // 0x82000E0C
  float f19;  // 0x82002510
};

inline RdataConsts LoadRdataConsts(uint8_t* base) {
  return RdataConsts{
      .f26 = FastLoadF32(base, 0x820BBDC4u),
      .f24 = FastLoadF32(base, 0x82000DC8u),
      .f16 = FastLoadF32(base, 0x82000D74u),
      .f27 = FastLoadF32(base, 0x82000D48u),
      .f20 = FastLoadF32(base, 0x82019670u),
      .f21 = FastLoadF32(base, 0x8200251Cu),
      .f22 = FastLoadF32(base, 0x82002518u),
      .f17 = FastLoadF32(base, 0x82000A34u),
      .f18 = FastLoadF32(base, 0x820BBCD8u),
      .f28 = FastLoadF32(base, 0x82000E0Cu),
      .f19 = FastLoadF32(base, 0x82002510u),
  };
}

// Inlined sub_823B32E0: clears bit 0x4 in sel+92 and returns true if already faded out (>= 239).
__attribute__((always_inline)) inline bool IsParentEntityFadedOut(
    uint8_t* base, uint32_t parent_ent, bool lod_fade_enabled) {
  if (!lod_fade_enabled) {
    return false;
  }
  auto is_valid_parent = [&](uint32_t ptr) -> bool {
    return ptr != 0 &&
           (FastLoadU8(base, ptr + 36) & 1u) != 0 &&
           (FastLoadU32(base, ptr + 40) & 0x40u) == 0;
  };
  uint32_t sel = 0;
  if (is_valid_parent(parent_ent)) {
    sel = parent_ent;
  } else if (parent_ent != 0) {
    const uint32_t grand_parent = FastLoadU32(base, parent_ent + 76);
    if (is_valid_parent(grand_parent)) {
      sel = grand_parent;
    }
  }
  if (sel == 0) {
    return false;
  }
  const int16_t model_idx = static_cast<int16_t>(FastLoadU16(base, sel + 46));
  const uint32_t model_info = FastLoadU32(base, 0x82B3A1A0u + uint32_t(int32_t(model_idx) * 4));
  if ((FastLoadU32(base, model_info + 64) & 0x8u) == 0) {
    return false;
  }
  const uint16_t f92 = FastLoadU16(base, sel + 92) & 65531u;
  FastStoreU16(base, sel + 92, f92);
  return FastLoadU8(base, sel + 99) >= 239;
}

#if defined(__aarch64__) || defined(_M_ARM64)
__attribute__((always_inline)) inline float CalcDist3dNEON(
    uint8_t* base, uint32_t pos_addr, float32x4_t cam_xyz) {
  const uint32_t aligned = pos_addr & ~0xFu;
  const float32x4_t pos_xyz =
      vreinterpretq_f32_u8(vrev32q_u8(vld1q_u8(GuestPointer(base, aligned))));
  const float32x4_t diff = vsubq_f32(pos_xyz, cam_xyz);
  const float dx = vgetq_lane_f32(diff, 0);
  const float dy = vgetq_lane_f32(diff, 1);
  const float dz = vgetq_lane_f32(diff, 2);
  return std::sqrt(std::fma(dx, dx, std::fma(dy, dy, dz * dz)));
}
#endif

}  // namespace scan_fast


extern "C" {
void sub_821D0478(PPCContext& ctx, uint8_t* base);
void sub_821E5278(PPCContext& ctx, uint8_t* base);
}

namespace scan_entities_v3_detail {

[[gnu::always_inline]] inline float LoadF32BE(uint8_t* base, uint32_t ea) noexcept {
    return std::bit_cast<float>(REX_LOAD_U32(ea));
}

[[gnu::always_inline]] inline void StoreF32BE(uint8_t* base, uint32_t ea, float val) noexcept {
    REX_STORE_U32(ea, std::bit_cast<uint32_t>(val));
}

[[gnu::always_inline]] inline int32_t PpcFctiwz(float v) noexcept {
    if (std::isnan(v)) [[unlikely]] {
        return static_cast<int32_t>(0x80000000u);
    }
    if (v >= 2147483648.0f) [[unlikely]] {
        return INT32_MAX;
    }
    if (v <= -2147483648.0f) [[unlikely]] {
        return INT32_MIN;
    }
    return static_cast<int32_t>(v);
}

#if defined(__aarch64__) || defined(_M_ARM64)
[[gnu::always_inline]] inline float32x4_t LoadGuestVec4BE(uint8_t* base, uint32_t ea) noexcept {
    const uint8x16_t raw = vld1q_u8(REX_RAW_ADDR(ea & ~0xFu));
    return vreinterpretq_f32_u8(vrev32q_u8(raw));
}

// Single-precision FMADD.S + FSQRT.S (zero FCVT S<->D overhead, exact PPC order)
[[gnu::always_inline]] inline float Distance3D_ZYX(float32x4_t pos, float32x4_t cam) noexcept {
    const float32x4_t d = vsubq_f32(pos, cam);
    const float dx = vgetq_lane_f32(d, 0);
    const float dy = vgetq_lane_f32(d, 1);
    const float dz = vgetq_lane_f32(d, 2);
    return std::sqrt(std::fmaf(dx, dx, std::fmaf(dy, dy, dz * dz)));
}

[[gnu::always_inline]] inline float Distance3D_YZX(float32x4_t pos, float32x4_t cam) noexcept {
    const float32x4_t d = vsubq_f32(pos, cam);
    const float dx = vgetq_lane_f32(d, 0);
    const float dy = vgetq_lane_f32(d, 1);
    const float dz = vgetq_lane_f32(d, 2);
    return std::sqrt(std::fmaf(dx, dx, std::fmaf(dz, dz, dy * dy)));
}
#else
struct Vec4F32 { float x, y, z, w; };

[[gnu::always_inline]] inline Vec4F32 LoadGuestVec4BE(uint8_t* base, uint32_t ea) noexcept {
    const uint32_t a = ea & ~0xFu;
    return { LoadF32BE(base, a), LoadF32BE(base, a + 4u),
             LoadF32BE(base, a + 8u), LoadF32BE(base, a + 12u) };
}

[[gnu::always_inline]] inline float Distance3D_ZYX(Vec4F32 pos, Vec4F32 cam) noexcept {
    const float dx = pos.x - cam.x;
    const float dy = pos.y - cam.y;
    const float dz = pos.z - cam.z;
    return std::sqrt(std::fmaf(dx, dx, std::fmaf(dy, dy, dz * dz)));
}

[[gnu::always_inline]] inline float Distance3D_YZX(Vec4F32 pos, Vec4F32 cam) noexcept {
    const float dx = pos.x - cam.x;
    const float dy = pos.y - cam.y;
    const float dz = pos.z - cam.z;
    return std::sqrt(std::fmaf(dx, dx, std::fmaf(dz, dz, dy * dy)));
}
#endif

[[gnu::always_inline]] inline uint32_t GetEntityPosAddr(uint8_t* base, uint32_t entity) noexcept {
    const uint32_t mat = REX_LOAD_U32(entity + 32u);
    return (mat != 0u) ? (mat + 48u) : (entity + 16u);
}

[[gnu::always_inline]] inline void ResetLodParentFade(uint8_t* base, uint32_t lod_parent) noexcept {
    const uint32_t lf40 = REX_LOAD_U32(lod_parent + 40u);
    if ((lf40 & 0x1000u) != 0u) {
        REX_STORE_U8(lod_parent + 99u, 255u);
    }
    REX_STORE_U32(lod_parent + 40u, lf40 & ~0x1000u);
}

// Fully inlined sub_823B32E0: checks parent / grandparent LOD streaming & clears target->flags92 bit 2
[[gnu::always_inline]] inline uint32_t CheckLodStreamingFast(
    uint8_t* base, uint32_t lod_parent, bool streaming_fade_enabled) noexcept
{
    if (!streaming_fade_enabled || lod_parent == 0u) [[likely]] {
        return 0u;
    }

    constexpr uint32_t kAddrModelInfoArray = 0x82B3A1A0u; // 0x82B40000 - 24160

    uint32_t target = 0u;
    if ((REX_LOAD_U8(lod_parent + 36u) & 1u) != 0u &&
        (REX_LOAD_U32(lod_parent + 40u) & 0x40u) == 0u) {
        target = lod_parent;
    } else {
        const uint32_t lod_grand = REX_LOAD_U32(lod_parent + 76u);
        if (lod_grand != 0u &&
            (REX_LOAD_U8(lod_grand + 36u) & 1u) != 0u &&
            (REX_LOAD_U32(lod_grand + 40u) & 0x40u) == 0u) {
            target = lod_grand;
        } else {
            return 0u;
        }
    }

    const int16_t  model_idx  = static_cast<int16_t>(REX_LOAD_U16(target + 46u));
    const uint32_t model_info = REX_LOAD_U32(
        kAddrModelInfoArray + (static_cast<uint32_t>(static_cast<int32_t>(model_idx)) << 2));
    if ((REX_LOAD_U32(model_info + 64u) & 0x8u) == 0u) {
        return 0u;
    }

    const uint16_t flags92 = REX_LOAD_U16(target + 92u);
    const uint8_t  alpha99 = REX_LOAD_U8(target + 99u);
    REX_STORE_U16(target + 92u, static_cast<uint16_t>(flags92 & 0xFFFBu));

    return (alpha99 >= 239u) ? 1u : 0u;
}

// Devirtualized model_info->vtable[3] (+12): GetModelType()
[[gnu::always_inline]] inline uint32_t GetModelTypeFast(
    uint8_t* base, uint32_t model_info, PPCContext& ctx) noexcept
{
    const uint32_t vtbl = REX_LOAD_U32(model_info);
    const uint32_t fn   = REX_LOAD_U32(vtbl + 12u);

    if (fn == 0x826041E8u) [[likely]] { ctx.r3.u64 = 1u; return 1u; } // CBaseModelInfo (>90%)
    if (fn == 0x821C04B8u)            { ctx.r3.u64 = 6u; return 6u; } // CVehicleModelInfo
    if (fn == 0x82174080u)            { ctx.r3.u64 = 5u; return 5u; } // CPedModelInfo
    if (fn == 0x82175518u)            { ctx.r3.u64 = 3u; return 3u; } // CTimeModelInfo
    if (fn == 0x82175650u)            { ctx.r3.u64 = 2u; return 2u; } // CMloModelInfo

    ctx.r3.u64 = model_info;
    ctx.lr     = 0x821D63E8u;
    REX_CALL_INDIRECT_FUNC(fn);
    return ctx.r3.u32 & 0xFFu;
}

// entity->vtable[21] (+84): GetBoundRadius() returning double in ctx.f1.f64
[[gnu::always_inline]] inline double GetEntityBoundRadius(
    uint8_t* base, uint32_t entity, PPCContext& ctx) noexcept
{
    const uint32_t fn = REX_LOAD_U32(REX_LOAD_U32(entity) + 84u);
    ctx.r3.u64 = entity;
    ctx.lr     = 0x821D6548u;
    REX_CALL_INDIRECT_FUNC(fn);
    return ctx.f1.f64;
}

} // namespace scan_entities_v3_detail

extern "C" void sub_821D6260(PPCContext& ctx, uint8_t* base) {
    if (!IsNativeMode()) {
        __imp__sub_821D6260(ctx, base);
        return;
    }
    using namespace scan_entities_v3_detail;

    constexpr uint32_t kAddrScanCounter       = 0x82B85390u;
    constexpr uint32_t kAddrModelInfoPtrs     = 0x82B3A1A0u;
    constexpr uint32_t kAddrStreamingFadeEn   = 0x830328E0u; // 0x83030000 (-2096955392) + 10464
    constexpr uint32_t kAddrGlobalFlag8303    = 0x83032953u; // 0x83030000 + 10579
    constexpr uint32_t kAddrGlobalInt82AA     = 0x82AA89ECu;
    constexpr uint32_t kAddrGlobalByte82B9    = 0x82B98462u;
    constexpr uint32_t kAddrGlobalFloat82A9A4 = 0x82A931A4u;
    constexpr uint32_t kAddrGlobalFloat82A9B8 = 0x82A931B8u;

    const uint32_t begin_addr    = ctx.r3.u32;
    const uint32_t end_addr      = ctx.r4.u32;
    const uint32_t viewport_addr = ctx.r5.u32;

    // 1. Pre-scan hook
    ctx.lr = 0x821D6284u;
    sub_821D0478(ctx, base);

    // Exact PPC order: load viewport+2336 BEFORE begin_addr >= end_addr check
    const float vp_lod_f23 = LoadF32BE(base, viewport_addr + 2336u);
    REX_STORE_U32(kAddrScanCounter, REX_LOAD_U32(kAddrScanCounter) + 1u);

    const bool pre_scan_zero = ((ctx.r3.u32 & 0xFFu) == 0u);
    if (begin_addr >= end_addr) {
        return;
    }

    // 2. Exact PPC pre-loop volatile GPR setup (preserves Live-Out ctx.r3 if loop body skips all)
    ctx.r3.s64  = -2113798144LL;
    ctx.r4.s64  = -2113929216LL;
    ctx.r5.s64  = -2113929216LL;
    ctx.r6.s64  = -2113929216LL;
    ctx.r7.s64  = -2113142784LL;
    ctx.r8.s64  = -2113929216LL;
    ctx.r9.s64  = -2113929216LL;
    ctx.r10.s64 = -2113929216LL;
    ctx.r11.s64 = -2102132736LL;

    // Hoist all 12 .rdata constants + 6 loop-invariant renderer globals + vp_scale
    const float c_f26 = LoadF32BE(base, 0x820BEDD4u);
    const float c_f24 = LoadF32BE(base, 0x82000DC8u);
    const float c_f16 = LoadF32BE(base, 0x82000D74u);
    const float c_f27 = LoadF32BE(base, 0x82000D48u);
    const float c_f20 = LoadF32BE(base, 0x82019670u);
    const float c_f21 = LoadF32BE(base, 0x8200251Cu);
    const float c_f22 = LoadF32BE(base, 0x82002518u);
    const float c_f17 = LoadF32BE(base, 0x82000A34u);
    const float c_f18 = LoadF32BE(base, 0x820BECD8u);
    const float c_f28 = LoadF32BE(base, 0x82000E0Cu);
    const float c_f25 = IsNativeMode()
        ? gta4::draw_distance::ResolveRemapLimit(scan_fast::FastLoadF32(base, kDistanceScaleInputGlobal))
        : LoadF32BE(base, 0x82002514u);
    const float c_f19 = LoadF32BE(base, 0x82002510u);

    const bool  streaming_fade_enabled = (REX_LOAD_U32(kAddrStreamingFadeEn) != 0u);
    const bool  lod_parent_filter_on   = (REX_LOAD_U8(kAddrGlobalFlag8303) != 0u) &&
                                         (static_cast<int32_t>(REX_LOAD_U32(kAddrGlobalInt82AA)) > 0);
    const bool  allow_fade_write       = (REX_LOAD_U8(kAddrGlobalByte82B9) == 0u);
    const float extra_cull_dist        = LoadF32BE(base, kAddrGlobalFloat82A9A4);
    const float radius_mult_f0         = LoadF32BE(base, kAddrGlobalFloat82A9B8);
    const float vp_scale               = LoadF32BE(base, viewport_addr + 2340u);

    // Lazily hoist camera position on first distance calculation (zero spurious OOB)
    bool cam_pos_loaded = false;
#if defined(__aarch64__) || defined(_M_ARM64)
    float32x4_t cam_pos{};
#else
    Vec4F32 cam_pos{};
#endif
    auto get_cam_pos = [&]() noexcept {
        if (!cam_pos_loaded) [[unlikely]] {
            cam_pos = LoadGuestVec4BE(base, viewport_addr + 2304u);
            cam_pos_loaded = true;
        }
        return cam_pos;
    };

    // 3. Main entity visibility, distance sort & LOD alpha fade loop
    for (uint32_t entry_ptr = begin_addr; entry_ptr < end_addr; entry_ptr += 8u) {
        if (entry_ptr + 32u < end_addr) {
            __builtin_prefetch(REX_RAW_ADDR(entry_ptr + 32u), 0, 1);
        }

        const uint32_t entity = REX_LOAD_U32(entry_ptr);
        if (entity == 0u) {
            continue;
        }

        const uint32_t lod_parent = REX_LOAD_U32(entity + 76u);

        // Inlined sub_823B32E0: LOD parent/grandparent streaming fade check (0 function calls)
        const uint32_t stream_culled = CheckLodStreamingFast(base, lod_parent, streaming_fade_enabled);
        ctx.r3.u64 = stream_culled;
        if (stream_culled != 0u) [[unlikely]] {
            REX_STORE_U32(entry_ptr, 0u);
            const uint16_t f92 = REX_LOAD_U16(entity + 92u);
            REX_STORE_U8(entity + 99u, 0u);
            REX_STORE_U16(entity + 92u, static_cast<uint16_t>(f92 & 0xFFFBu));
            continue;
        }

        // Hoisted global LOD parent filter
        if (lod_parent_filter_on && lod_parent != 0u && (REX_LOAD_U8(lod_parent + 36u) & 1u) == 0u) {
            REX_STORE_U32(entry_ptr, 0u);
            continue;
        }

        // Cache flags36 & flags40 once for the entire iteration
        const uint32_t flags36 = REX_LOAD_U32(entity + 36u);
        if ((flags36 & 0x04000000u) == 0u) {
            REX_STORE_U32(entry_ptr, 0u);
            continue;
        }
        const uint32_t flags40 = REX_LOAD_U32(entity + 40u);
        if ((flags40 & 0x00000100u) != 0u) {
            REX_STORE_U32(entry_ptr, 0u);
            continue;
        }

        // Deferred CBaseModelInfo lookup (only for entities that survive visibility & streaming culls)
        const int16_t  model_idx  = static_cast<int16_t>(REX_LOAD_U16(entity + 46u));
        const uint32_t model_info = REX_LOAD_U32(
            kAddrModelInfoPtrs + (static_cast<uint32_t>(static_cast<int32_t>(model_idx)) << 2));

        // Devirtualized model_info->GetModelType() (0 indirect jumps for 5 standard RAGE model types)
        const uint32_t model_type = GetModelTypeFast(base, model_info, ctx);

        // --- Branch A: model_type == 5 (Ped) ---
        if (model_type == 5u) {
            if ((flags36 & 0x00080000u) == 0u) {
                continue;
            }
            const auto pos = LoadGuestVec4BE(base, GetEntityPosAddr(base, entity));
            StoreF32BE(base, entry_ptr + 4u, Distance3D_ZYX(pos, get_cam_pos()));
            continue;
        }

        // --- Branch B: model_type == 6 (Vehicle) ---
        if (model_type == 6u) {
            if ((flags36 & 0x00000040u) != 0u) {
                const uint32_t p120 = REX_LOAD_U32(entity + 120u);
                if (p120 == 0u || (REX_LOAD_U8(p120) & 0x40u) == 0u) {
                    REX_STORE_U32(entry_ptr, 0u);
                    continue;
                }
            }
            if ((flags36 & 0x00080000u) == 0u) {
                continue;
            }

            const auto pos = LoadGuestVec4BE(base, GetEntityPosAddr(base, entity));
            float dist = Distance3D_YZX(pos, get_cam_pos());
            StoreF32BE(base, entry_ptr + 4u, dist);

            if ((flags40 & 0x03C00000u) == 0x00C00000u &&
                (REX_LOAD_U32(entity + 572u) & 0x20000000u) != 0u) {
                const uint32_t p2688 = REX_LOAD_U32(entity + 2688u);
                if (p2688 != 0u) {
                    const uint32_t fn_rad = REX_LOAD_U32(REX_LOAD_U32(p2688) + 84u);
                    ctx.r3.u64 = p2688;
                    ctx.lr     = 0x821D688Cu;
                    REX_CALL_INDIRECT_FUNC(fn_rad);
                    dist = static_cast<float>(ctx.f1.f64 + static_cast<double>(dist));
                    StoreF32BE(base, entry_ptr + 4u, dist);
                }
            }

            const uint32_t fn_v36 = REX_LOAD_U32(REX_LOAD_U32(entity) + 36u);
            ctx.r3.u64 = entity;
            ctx.lr     = 0x821D68ACu;
            REX_CALL_INDIRECT_FUNC(fn_v36);
            if ((ctx.r3.u32 & 0xFFu) == 0u) {
                continue;
            }

            ctx.r3.u64 = entity;
            ctx.lr     = 0x821D68C0u;
            sub_821E5278(ctx, base);
            const uint32_t aux_obj = ctx.r3.u32;
            if (aux_obj == 0u) {
                continue;
            }

            const uint32_t fn_aux = REX_LOAD_U32(REX_LOAD_U32(aux_obj) + 84u);
            ctx.r3.u64 = aux_obj;
            ctx.lr     = 0x821D68D8u;
            REX_CALL_INDIRECT_FUNC(fn_aux);
            dist = static_cast<float>(ctx.f1.f64 + static_cast<double>(dist));
            StoreF32BE(base, entry_ptr + 4u, dist);
            continue;
        }

        // --- Branch C: Standard Entities (Buildings, Props, LODs — >90% of city scene) ---
        const uint32_t pos_owner = (lod_parent != 0u) ? lod_parent : entity;
        const auto     owner_pos = LoadGuestVec4BE(base, GetEntityPosAddr(base, pos_owner));
        const auto     cam       = get_cam_pos();

        const float entity_lod_dist = LoadF32BE(base, entity + 80u);
        float f29 = entity_lod_dist;
        float f30 = Distance3D_ZYX(owner_pos, cam);

        const uint32_t is_lo_lod = (flags36 >> 24) & 1u;
        if (is_lo_lod != 0u && vp_lod_f23 > c_f19) {
            f29 = vp_lod_f23;
        }

        const float f31 = vp_scale * f29;

        if ((flags36 & 0x02000000u) == 0u) {
            if (f30 > c_f25 && f31 > c_f25 && f30 < (f31 + c_f28)) {
                f30 = (f31 - c_f25) + f30;
            }
        }

        const float sort_bias = (is_lo_lod != 0u) ? c_f18 : c_f17;
        float sort_dist;
        if (lod_parent != 0u) {
            const auto self_pos = LoadGuestVec4BE(base, GetEntityPosAddr(base, entity));
            sort_dist = Distance3D_ZYX(self_pos, cam) + sort_bias;
        } else {
            sort_dist = sort_bias + f30;
        }
        StoreF32BE(base, entry_ptr + 4u, sort_dist);

        // entity->vtable[21] (+84): GetBoundRadius()
        const double bound_rad = GetEntityBoundRadius(base, entity, ctx);

        float f0  = static_cast<float>(bound_rad + static_cast<double>(vp_lod_f23));
        float f13 = c_f28;
        f0 = ((f31 - f0) >= 0.0f) ? f0 : f31;

        if (lod_parent == 0u) {
            const float f12 = ((f29 - f0) >= 0.0f) ? f0 : f29;
            if (f12 > c_f22) {
                f13 = std::fmaf(f12, c_f21, c_f20);
            }
            if ((flags36 & 0x02000000u) != 0u) {
                f0 = radius_mult_f0 * f0;
            }
        }

        const uint32_t can_fade = (REX_LOAD_U32(model_info + 64u) >> 3) & 1u;
        if (can_fade != 0u) {
            const float f12 = (f13 + f30) - c_f28;
            if (f12 < f31) {
                if (lod_parent != 0u) {
                    ResetLodParentFade(base, lod_parent);
                }
                continue;
            }

            if ((f30 - c_f28) < f31) {
                float t = ((f0 + c_f28) - f30) / f13;
                t = ((t - c_f27) >= 0.0f) ? c_f27 : t;

                if (lod_parent != 0u) {
                    const uint32_t lf40 = REX_LOAD_U32(lod_parent + 40u);
                    REX_STORE_U32(lod_parent + 40u, lf40 | 0x1000u);
                    if (t < c_f16) {
                        t = t * c_f24;
                    } else {
                        if (allow_fade_write) {
                            const float lod_alpha = ((c_f27 - t) * c_f24) * c_f26;
                            REX_STORE_U8(lod_parent + 99u, static_cast<uint8_t>(PpcFctiwz(lod_alpha)));
                        }
                        t = c_f27;
                    }
                }

                const float alpha_f = t * c_f26;
                if ((flags40 & 0x1000u) == 0u && allow_fade_write) {
                    const float cur_a = static_cast<float>(REX_LOAD_U8(entity + 99u));
                    const float min_a = ((alpha_f - cur_a) >= 0.0f) ? cur_a : alpha_f;
                    REX_STORE_U8(entity + 99u, static_cast<uint8_t>(PpcFctiwz(min_a)));
                }
                continue;
            }
        }

        // loc_821D66F8: Far distance cull / LOD streaming flag
        if ((f30 - (extra_cull_dist + c_f28)) < f0) {
            if (lod_parent != 0u) {
                ResetLodParentFade(base, lod_parent);
            }
            if (can_fade != 0u || !pre_scan_zero) {
                REX_STORE_U32(entry_ptr, 0u);
                continue;
            }
            const float max_d = entity_lod_dist * vp_scale;
            if (!((f30 - c_f28) < max_d)) {
                REX_STORE_U32(entity + 8u,  REX_LOAD_U32(entity + 8u)  | 0x40000000u);
                REX_STORE_U32(entity + 12u, REX_LOAD_U32(entity + 12u) | 0x40000000u);
            }
            continue;
        }

        // loc_821D678C: Entity culled
        if (lod_parent != 0u) {
            ResetLodParentFade(base, lod_parent);
        }
        REX_STORE_U32(entry_ptr, 0u);
    }
}


extern "C" void sub_821D8CD8(PPCContext& ctx, uint8_t* base) {
  if (!IsNativeMode()) {
    __imp__sub_821D8CD8(ctx, base);
    return;
  }
  const float native_remap_limit = gta4::draw_distance::ResolveRemapLimit(
      LoadF32(base, kDistanceScaleInputGlobal));
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x829ff7c4
	ctx.lr = 0x821D8CE0;
	__savegprlr_27(ctx, base);
	// stwu r1,-144(r1)
	ea = -144 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r29,r6
	ctx.r29.u64 = ctx.r6.u64;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// mr r28,r4
	ctx.r28.u64 = ctx.r4.u64;
	// mr r27,r5
	ctx.r27.u64 = ctx.r5.u64;
	// lwz r11,60(r29)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r29.u32 + 60);
	// cmpwi cr6,r11,2
	ctx.cr6.compare<int32_t>(ctx.r11.s32, 2, ctx.xer);
	// lwz r11,36(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 36);
	// beq cr6,0x821d8d1c
	if (ctx.cr6.eq) goto loc_821D8D1C;
	// rlwinm r10,r11,0,16,16
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 0) & 0x8000;
	// cmplwi cr6,r10,32768
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 32768, ctx.xer);
	// bne cr6,0x821d8d34
	if (!ctx.cr6.eq) goto loc_821D8D34;
loc_821D8D10:
	// li r3,2
	ctx.r3.s64 = 2;
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x829ff814
	__restgprlr_27(ctx, base);
	return;
loc_821D8D1C:
	// rlwinm r10,r11,0,17,17
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 0) & 0x4000;
	// cmplwi cr6,r10,16384
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 16384, ctx.xer);
	// bne cr6,0x821d8d34
	if (!ctx.cr6.eq) goto loc_821D8D34;
	// li r3,2
	ctx.r3.s64 = 2;
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x829ff814
	__restgprlr_27(ctx, base);
	return;
loc_821D8D34:
	// rlwinm r11,r11,0,26,26
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 0) & 0x20;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x821d8d70
	if (ctx.cr6.eq) goto loc_821D8D70;
	// lwz r11,40(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 40);
	// lis r10,256
	ctx.r10.s64 = 16777216;
	// rlwinm r11,r11,0,6,9
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 0) & 0x3C00000;
	// cmplw cr6,r11,r10
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
	// bne cr6,0x821d8d70
	if (!ctx.cr6.eq) goto loc_821D8D70;
	// lis r11,-32071
	ctx.r11.s64 = -2101805056;
	// lbz r11,24256(r11)
	ctx.r11.u64 = REX_LOAD_U8(ctx.r11.u32 + 24256);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x821d8d70
	if (ctx.cr6.eq) goto loc_821D8D70;
loc_821D8D64:
	// li r3,1
	ctx.r3.s64 = 1;
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x829ff814
	__restgprlr_27(ctx, base);
	return;
loc_821D8D70:
	// lhz r10,46(r31)
	ctx.r10.u64 = REX_LOAD_U16(ctx.r31.u32 + 46);
	// lis r11,-32076
	ctx.r11.s64 = -2102132736;
	// extsh r10,r10
	ctx.r10.s64 = ctx.r10.s16;
	// addi r11,r11,-24160
	ctx.r11.s64 = ctx.r11.s64 + -24160;
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// lwzx r30,r10,r11
	ctx.r30.u64 = REX_LOAD_U32(ctx.r10.u32 + ctx.r11.u32);
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// lwz r11,0(r30)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r30.u32 + 0);
	// lwz r11,12(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 12);
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x821D8D9C;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
	// clrlwi r11,r3,24
	ctx.r11.u64 = ctx.r3.u32 & 0xFF;
	// cmpwi cr6,r11,3
	ctx.cr6.compare<int32_t>(ctx.r11.s32, 3, ctx.xer);
	// beq cr6,0x821d8e70
	if (ctx.cr6.eq) goto loc_821D8E70;
	// addi r11,r11,-5
	ctx.r11.s64 = ctx.r11.s64 + -5;
	// cmplwi cr6,r11,1
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 1, ctx.xer);
	// bgt cr6,0x821d8e90
	if (ctx.cr6.gt) goto loc_821D8E90;
	// lwz r11,36(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 36);
	// rlwinm r11,r11,0,5,5
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 0) & 0x4000000;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x821d8e64
	if (ctx.cr6.eq) goto loc_821D8E64;
	// li r5,0
	ctx.r5.s64 = 0;
	// mr r4,r29
	ctx.r4.u64 = ctx.r29.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x821d6dd0
	ctx.lr = 0x821D8DD4;
	sub_821D6DD0(ctx, base);
	// clrlwi r11,r3,24
	ctx.r11.u64 = ctx.r3.u32 & 0xFF;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x821d8d10
	if (!ctx.cr6.eq) goto loc_821D8D10;
	// mr r4,r29
	ctx.r4.u64 = ctx.r29.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x822a2aa8
	ctx.lr = 0x821D8DEC;
	sub_822A2AA8(ctx, base);
	// clrlwi r11,r3,24
	ctx.r11.u64 = ctx.r3.u32 & 0xFF;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x821d8d10
	if (!ctx.cr6.eq) goto loc_821D8D10;
	// lwz r11,36(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 36);
	// rlwinm r11,r11,0,12,12
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 0) & 0x80000;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x821d8d64
	if (ctx.cr6.eq) goto loc_821D8D64;
	// li r10,1
	ctx.r10.s64 = 1;
	// lwz r11,32(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 32);
	// li r9,2304
	ctx.r9.s64 = 2304;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// addi r11,r11,48
	ctx.r11.s64 = ctx.r11.s64 + 48;
	// lvx128 v0,r29,r9
	ea = (ctx.r29.u32 + ctx.r9.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)ctx.v0.u8, simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)REX_RAW_ADDR(ea)), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// slw r5,r10,r27
	ctx.r5.u64 = ctx.r27.u8 & 0x20 ? 0 : (ctx.r10.u32 << (ctx.r27.u8 & 0x3F));
	// bne cr6,0x821d8e2c
	if (!ctx.cr6.eq) goto loc_821D8E2C;
	// addi r11,r31,16
	ctx.r11.s64 = ctx.r31.s64 + 16;
loc_821D8E2C:
	// lvx128 v13,r0,r11
	ea = (ctx.r11.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)ctx.v13.u8, simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)REX_RAW_ADDR(ea)), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// addi r11,r1,80
	ctx.r11.s64 = ctx.r1.s64 + 80;
	// vsubfp v0,v13,v0
	ctx.fpscr.enableFlushMode();
	simde_mm_store_ps(ctx.v0.f32, simde_mm_sub_ps(simde_mm_load_ps(ctx.v13.f32), simde_mm_load_ps(ctx.v0.f32)));
	// li r6,0
	ctx.r6.s64 = 0;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// stvx128 v0,r0,r11
	ea = (ctx.r11.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)REX_RAW_ADDR(ea), simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)ctx.v0.u8), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// lfs f13,84(r1)
	ctx.fpscr.disableFlushModeUnconditional();
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + 84);
	ctx.f13.f64 = double(temp.f32);
	// fmuls f12,f13,f13
	ctx.f12.f64 = double(float(ctx.f13.f64 * ctx.f13.f64));
	// lfs f0,88(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + 88);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,80(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + 80);
	ctx.f13.f64 = double(temp.f32);
	// fmadds f0,f0,f0,f12
	ctx.f0.f64 = double(float(std::fma(ctx.f0.f64, ctx.f0.f64, ctx.f12.f64)));
	// fmadds f0,f13,f13,f0
	ctx.f0.f64 = double(float(std::fma(ctx.f13.f64, ctx.f13.f64, ctx.f0.f64)));
	// fsqrts f1,f0
	ctx.f1.f64 = double(float(sqrt(ctx.f0.f64)));
	// bl 0x821d7900
	ctx.lr = 0x821D8E64;
	sub_821D7900(ctx, base);
loc_821D8E64:
	// li r3,0
	ctx.r3.s64 = 0;
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x829ff814
	__restgprlr_27(ctx, base);
	return;
loc_821D8E70:
	// li r6,0
	ctx.r6.s64 = 0;
	// li r5,1
	ctx.r5.s64 = 1;
	// li r4,0
	ctx.r4.s64 = 0;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x821d8040
	ctx.lr = 0x821D8E84;
	sub_821D8040(ctx, base);
	// clrlwi r11,r3,24
	ctx.r11.u64 = ctx.r3.u32 & 0xFF;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x821d8e64
	if (!ctx.cr6.eq) goto loc_821D8E64;
loc_821D8E90:
	// lwz r11,76(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 76);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x821d8eb8
	if (ctx.cr6.eq) goto loc_821D8EB8;
	// lwz r10,32(r11)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r11.u32 + 32);
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x821d8eb0
	if (ctx.cr6.eq) goto loc_821D8EB0;
	// addi r11,r10,48
	ctx.r11.s64 = ctx.r10.s64 + 48;
	// b 0x821d8ecc
	goto loc_821D8ECC;
loc_821D8EB0:
	// addi r11,r11,16
	ctx.r11.s64 = ctx.r11.s64 + 16;
	// b 0x821d8ecc
	goto loc_821D8ECC;
loc_821D8EB8:
	// lwz r11,32(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 32);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// addi r11,r11,48
	ctx.r11.s64 = ctx.r11.s64 + 48;
	// bne cr6,0x821d8ecc
	if (!ctx.cr6.eq) goto loc_821D8ECC;
	// addi r11,r31,16
	ctx.r11.s64 = ctx.r31.s64 + 16;
loc_821D8ECC:
	// lvx128 v0,r0,r11
	ea = (ctx.r11.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)ctx.v0.u8, simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)REX_RAW_ADDR(ea)), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// addi r11,r1,80
	ctx.r11.s64 = ctx.r1.s64 + 80;
	// stvx128 v0,r0,r11
	ea = (ctx.r11.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)REX_RAW_ADDR(ea), simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)ctx.v0.u8), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// li r11,2304
	ctx.r11.s64 = 2304;
	// lvx128 v13,r29,r11
	ea = (ctx.r29.u32 + ctx.r11.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)ctx.v13.u8, simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)REX_RAW_ADDR(ea)), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// addi r11,r1,80
	ctx.r11.s64 = ctx.r1.s64 + 80;
	// vsubfp v0,v0,v13
	ctx.fpscr.enableFlushMode();
	simde_mm_store_ps(ctx.v0.f32, simde_mm_sub_ps(simde_mm_load_ps(ctx.v0.f32), simde_mm_load_ps(ctx.v13.f32)));
	// stvx128 v0,r0,r11
	ea = (ctx.r11.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)REX_RAW_ADDR(ea), simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)ctx.v0.u8), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,88(r1)
	ctx.fpscr.disableFlushModeUnconditional();
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + 88);
	ctx.f13.f64 = double(temp.f32);
	// fmuls f12,f13,f13
	ctx.f12.f64 = double(float(ctx.f13.f64 * ctx.f13.f64));
	// lfs f0,84(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + 84);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,80(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + 80);
	ctx.f13.f64 = double(temp.f32);
	// lfs f11,9492(r11)
	temp.u32 = std::bit_cast<uint32_t>(native_remap_limit);
	ctx.f11.f64 = double(temp.f32);
	// fmadds f0,f0,f0,f12
	ctx.f0.f64 = double(float(std::fma(ctx.f0.f64, ctx.f0.f64, ctx.f12.f64)));
	// fmadds f0,f13,f13,f0
	ctx.f0.f64 = double(float(std::fma(ctx.f13.f64, ctx.f13.f64, ctx.f0.f64)));
	// fsqrts f0,f0
	ctx.f0.f64 = double(float(sqrt(ctx.f0.f64)));
	// stfs f0,0(r28)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r28.u32 + 0, temp.u32);
	// fcmpu cr6,f0,f11
	ctx.cr6.compare(ctx.f0.f64, ctx.f11.f64);
	// ble cr6,0x821d8f50
	if (!ctx.cr6.gt) goto loc_821D8F50;
	// lfs f13,2340(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 2340);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,80(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 80);
	ctx.f12.f64 = double(temp.f32);
	// fmuls f10,f12,f13
	ctx.f10.f64 = double(float(ctx.f12.f64 * ctx.f13.f64));
	// fcmpu cr6,f10,f11
	ctx.cr6.compare(ctx.f10.f64, ctx.f11.f64);
	// ble cr6,0x821d8f50
	if (!ctx.cr6.gt) goto loc_821D8F50;
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f10,3596(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 3596);
	ctx.f10.f64 = double(temp.f32);
	// fmadds f10,f12,f13,f10
	ctx.f10.f64 = double(float(std::fma(ctx.f12.f64, ctx.f13.f64, ctx.f10.f64)));
	// fcmpu cr6,f0,f10
	ctx.cr6.compare(ctx.f0.f64, ctx.f10.f64);
	// bge cr6,0x821d8f50
	if (!ctx.cr6.lt) goto loc_821D8F50;
	// fmsubs f13,f12,f13,f11
	ctx.f13.f64 = double(float(std::fma(ctx.f12.f64, ctx.f13.f64, -ctx.f11.f64)));
	// fadds f0,f13,f0
	ctx.f0.f64 = double(float(ctx.f13.f64 + ctx.f0.f64));
	// stfs f0,0(r28)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r28.u32 + 0, temp.u32);
loc_821D8F50:
	// mr r8,r29
	ctx.r8.u64 = ctx.r29.u64;
	// lfs f1,0(r28)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r28.u32 + 0);
	ctx.f1.f64 = double(temp.f32);
	// mr r7,r27
	ctx.r7.u64 = ctx.r27.u64;
	// li r6,1
	ctx.r6.s64 = 1;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x821d8848
	ctx.lr = 0x821D8F6C;
	sub_821D8848(ctx, base);
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x829ff814
	__restgprlr_27(ctx, base);
	return;
}
