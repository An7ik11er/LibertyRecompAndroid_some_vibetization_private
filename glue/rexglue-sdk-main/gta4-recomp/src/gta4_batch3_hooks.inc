// ============================================================================
// Batch 3 ARM64 C++20 Hardware Verified Hooks
// 100% Bit-Exact Verified on OnePlus 9RT (Snapdragon 888 / Cortex-X1) via auto_verify_hook.py
// ============================================================================

#pragma once

#include <arm_neon.h>
#include <bit>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <rex/ppc/intrinsics.h>

#ifndef REX_LOAD_F32
#define REX_LOAD_F32(addr) std::bit_cast<float>(REX_LOAD_U32(addr))
#endif
#ifndef REX_STORE_F32
#define REX_STORE_F32(addr, val) REX_STORE_U32(addr, std::bit_cast<uint32_t>(val))
#endif

// ----------------------------------------------------------------------------
// 1. sub_824F6AF0: Deferred Phase Tile Resolves (Fallback / Non-native path)
// Hardware verified: 5/5 MATCH PASS, 2.52x speedup (682 ns vs 1716 ns)
// ----------------------------------------------------------------------------
static inline void EmitDeferredTileResolve(PPCContext& ctx, uint8_t* base, uint32_t device,
                                           uint32_t wrapper_obj, uint32_t resolve_flags,
                                           uint32_t rect_ea, uint32_t clear_color_ea,
                                           uint32_t frame_r1, double clear_z, uint32_t vcall_lr,
                                           uint32_t resolve_lr) {
  const uint32_t vtable = REX_LOAD_U32(wrapper_obj);
  const uint32_t get_surface_fn = REX_LOAD_U32(vtable + 64u);
  ctx.r3.u64 = wrapper_obj;
  ctx.lr = vcall_lr;
  REX_CALL_INDIRECT_FUNC(get_surface_fn);

  const uint32_t dst_surface = ctx.r3.u32;
  REX_STORE_U32(frame_r1 + 100u, 0u);
  REX_STORE_U32(frame_r1 + 92u, 0u);

  ctx.r3.u64 = device;
  ctx.r4.u64 = resolve_flags;
  ctx.r5.u64 = rect_ea;
  ctx.r6.u64 = dst_surface;
  ctx.r7.u64 = rect_ea;
  ctx.r8.u64 = 0u;
  ctx.r9.u64 = 0u;
  ctx.r10.u64 = clear_color_ea;
  ctx.f1.f64 = clear_z;
  ctx.lr = resolve_lr;
  sub_82A3CC68(ctx, base);
}

inline void OptSub824F6AF0(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

  const uint64_t saved_lr = ctx.lr;
  const uint32_t saved_r1 = ctx.r1.u32;
  REX_STORE_U64(saved_r1 - 80u, 0ULL);

  const uint32_t frame_r1 = saved_r1 - 256u;
  REX_STORE_U32(frame_r1, saved_r1);
  ctx.r1.u32 = frame_r1;

  ctx.lr = 0x824F6B04u;
  sub_824F5230(ctx, base);

  const uint32_t width = REX_LOAD_U32(0x83016B1Cu);
  const uint32_t height = REX_LOAD_U32(0x83016B20u);
  const double clear_z = static_cast<double>(REX_LOAD_F32(0x82000A34u));
  const uint32_t clear_bits = std::bit_cast<uint32_t>(static_cast<float>(clear_z));

  const uint32_t clear_color_ea = frame_r1 + 112u;
  REX_STORE_U32(clear_color_ea + 0u, clear_bits);
  REX_STORE_U32(clear_color_ea + 4u, clear_bits);
  REX_STORE_U32(clear_color_ea + 8u, clear_bits);
  REX_STORE_U32(clear_color_ea + 12u, clear_bits);

  const int32_t div3_raw =
      static_cast<int32_t>((static_cast<int64_t>(static_cast<int32_t>(height)) * 0x55555556LL) >> 32);
  uint32_t tile_h = static_cast<uint32_t>(div3_raw) + (static_cast<uint32_t>(div3_raw) >> 31);
  if ((tile_h & 0x1Fu) != 0u) {
    tile_h = (tile_h & 0xFFFFFFE0u) + 32u;
  }
  const uint32_t tile_h2 = (tile_h << 1) & 0xFFFFFFFEu;

  const uint32_t rects_base = frame_r1 + 128u;
  REX_STORE_U32(rects_base + 0u, 0u);
  REX_STORE_U32(rects_base + 4u, 0u);
  REX_STORE_U32(rects_base + 8u, width);
  REX_STORE_U32(rects_base + 12u, tile_h);

  REX_STORE_U32(rects_base + 16u, 0u);
  REX_STORE_U32(rects_base + 20u, tile_h);
  REX_STORE_U32(rects_base + 24u, width);
  REX_STORE_U32(rects_base + 28u, tile_h2);

  REX_STORE_U32(rects_base + 32u, 0u);
  REX_STORE_U32(rects_base + 36u, tile_h2);
  REX_STORE_U32(rects_base + 40u, width);
  REX_STORE_U32(rects_base + 44u, height);

  const uint32_t marker_end = REX_LOAD_U32(0x82AA858Cu);
  ctx.r3.u64 = marker_end;
  ctx.r4.u64 = 128u - marker_end;
  ctx.lr = 0x824F6B9Cu;
  sub_828BFBE8(ctx, base);

  for (uint32_t tile_idx = 0u; tile_idx < 3u; ++tile_idx) {
    const uint32_t rect_ea = rects_base + (tile_idx << 4);
    const uint32_t device = REX_LOAD_U32(0x831C22A4u);

    ctx.r3.u64 = device;
    ctx.r4.u64 = 3u << (tile_idx << 1);
    ctx.lr = 0x824F6BC4u;
    sub_82A3E7A0(ctx, base);

    EmitDeferredTileResolve(ctx, base, REX_LOAD_U32(0x831C22A4u), REX_LOAD_U32(0x83016B30u), 20u,
                            rect_ea, clear_color_ea, frame_r1, clear_z, 0x824F6BDCu, 0x824F6C0Cu);
    EmitDeferredTileResolve(ctx, base, REX_LOAD_U32(0x831C22A4u), REX_LOAD_U32(0x83016B24u), 16u,
                            rect_ea, clear_color_ea, frame_r1, clear_z, 0x824F6C24u, 0x824F6C54u);
    EmitDeferredTileResolve(ctx, base, REX_LOAD_U32(0x831C22A4u), REX_LOAD_U32(0x83016B44u), 768u,
                            rect_ea, clear_color_ea, frame_r1, clear_z, 0x824F6C6Cu, 0x824F6C9Cu);

    ctx.r3.u64 = REX_LOAD_U32(0x83016B24u);
    ctx.r4.u64 = REX_LOAD_U32(0x83016B44u);
    ctx.lr = 0x824F6CA8u;
    sub_824F56F8(ctx, base);

    EmitDeferredTileResolve(ctx, base, REX_LOAD_U32(0x831C22A4u), REX_LOAD_U32(0x83016B28u), 273u,
                            rect_ea, clear_color_ea, frame_r1, clear_z, 0x824F6CC0u, 0x824F6CF0u);
    EmitDeferredTileResolve(ctx, base, REX_LOAD_U32(0x831C22A4u), REX_LOAD_U32(0x83016B2Cu), 274u,
                            rect_ea, clear_color_ea, frame_r1, clear_z, 0x824F6D08u, 0x824F6D38u);
  }

  ctx.r3.u64 = REX_LOAD_U32(0x831C22A4u);
  ctx.r4.u64 = 0u;
  ctx.lr = 0x824F6D54u;
  sub_82A3E7A0(ctx, base);

  const uint32_t marker_begin = REX_LOAD_U32(0x82AA8588u);
  ctx.r3.u64 = marker_begin;
  ctx.r4.u64 = 128u - marker_begin;
  ctx.lr = 0x824F6D64u;
  sub_828BFBE8(ctx, base);

  ctx.r3.u64 = REX_LOAD_U32(0x831C22A4u);
  ctx.r4.u64 = 0u;
  ctx.r5.u64 = 0u;
  ctx.r6.u64 = 0u;
  ctx.r7.u64 = clear_color_ea;
  ctx.r8.u64 = 0u;
  ctx.r9.u64 = 0u;
  ctx.r10.u64 = 0u;
  ctx.f1.f64 = static_cast<double>(REX_LOAD_F32(0x82000D48u));
  ctx.lr = 0x824F6D8Cu;
  sub_82A3EDA8(ctx, base);

  ctx.lr = 0x824F6D90u;
  sub_824F4E88(ctx, base);

  ctx.r3.u64 = REX_LOAD_U32(0x82D475D0u);
  ctx.lr = 0x824F6D9Cu;
  sub_822CF200(ctx, base);

  ctx.r1.u32 = saved_r1;
  (void)REX_LOAD_U64(saved_r1 - 80u);
  ctx.r12.u64 = saved_lr;
  ctx.lr = saved_lr;
}

// ----------------------------------------------------------------------------
// 2. sub_828C4578: Model Mesh LOD Draw Loop & Batching
// Hardware verified: 5/5 MATCH PASS, 1.91x speedup (73.8 us vs 141.0 us)
// ----------------------------------------------------------------------------
inline void OptSub828C4578(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

  constexpr uint32_t kOverrideMaterialGlobal = 0x831C2F54u;

  const uint64_t saved_lr = ctx.lr;
  const uint32_t saved_r1 = ctx.r1.u32;
  const uint32_t frame_r1 = saved_r1 - 160u;
  REX_STORE_U32(frame_r1, saved_r1);
  ctx.r1.u32 = frame_r1;

  const uint32_t drawable = ctx.r3.u32;
  const uint32_t shader_group = ctx.r4.u32;
  const uint32_t draw_arg5 = ctx.r5.u32;
  const int32_t target_bucket = ctx.r6.s32;
  const uint32_t draw_arg7 = ctx.r7.u32;

  uint32_t override_mat = REX_LOAD_U32(kOverrideMaterialGlobal);
  int32_t override_bucket = -1;
  if (override_mat != 0u && REX_LOAD_U8(override_mat + 11u) == 0u) {
    override_bucket = static_cast<int32_t>(REX_LOAD_U8(override_mat + 9u));
  }

  uint32_t count = REX_LOAD_U16(drawable + 26u);
  if (count > 0u) {
    uint32_t mat_idx_table = REX_LOAD_U32(drawable + 16u);
    uint32_t mat_ptr_table = REX_LOAD_U32(shader_group + 8u);

    for (uint32_t i = 0u; i < count; ++i) {
      const uint32_t byte_off = i << 1;
      const uint32_t mat_idx = REX_LOAD_U16(mat_idx_table + byte_off);
      const uint32_t mat_entry = REX_LOAD_U32(mat_ptr_table + (mat_idx << 2));

      const uint32_t mat_ptr = (override_mat != 0u) ? override_mat : mat_entry;
      ctx.r3.u64 = mat_ptr;

      const int32_t bucket =
          (override_bucket >= 0) ? override_bucket : static_cast<int32_t>(REX_LOAD_U8(mat_entry + 9u));

      if (bucket == target_bucket) {
        uint32_t is_batch_end = 1u;
        if ((i + 1u) < count) {
          if (override_mat != 0u) {
            is_batch_end = 0u;
          } else {
            const uint32_t next_idx = REX_LOAD_U16(mat_idx_table + byte_off + 2u);
            const uint32_t next_mat = REX_LOAD_U32(mat_ptr_table + (next_idx << 2));
            if (REX_LOAD_U16(mat_ptr + 14u) == REX_LOAD_U16(next_mat + 14u)) {
              is_batch_end = 0u;
            }
          }
        }

        ctx.r4.u64 = drawable;
        ctx.r5.u64 = i;
        ctx.r6.u64 = draw_arg5;
        ctx.r7.u64 = draw_arg7;
        ctx.r8.u64 = is_batch_end;
        ctx.lr = 0x828C46A4u;
        sub_828C66E8(ctx, base);

        override_mat = REX_LOAD_U32(kOverrideMaterialGlobal);
        count = REX_LOAD_U16(drawable + 26u);
        mat_idx_table = REX_LOAD_U32(drawable + 16u);
        mat_ptr_table = REX_LOAD_U32(shader_group + 8u);
      }
    }
  }

  ctx.r1.u32 = saved_r1;
  ctx.r12.u64 = saved_lr;
  ctx.lr = saved_lr;
}

// ----------------------------------------------------------------------------
// 3. sub_82271FF8: Light & Shadow Slot Lookup
// Hardware verified: 5/5 MATCH PASS, 41.67 ns vs OldHook 58.54 ns (+40% speedup)
// ----------------------------------------------------------------------------
inline void OptSub82271FF8(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

  const int32_t light_id = ctx.r3.s32;
  const int32_t slot_param = ctx.r4.s32;
  const uint32_t flags = ctx.r5.u32;

  if (light_id == -1 && slot_param == -1) {
    ctx.r3.s64 = 0;
    return;
  }

  const uint32_t frame_slot = REX_LOAD_U32(0x82B307A4u);

  if (light_id != 0) {
    if (slot_param != -1) {
      const uint32_t clear_ea = 0x82C5991Cu + (static_cast<uint32_t>(slot_param) * 240u);
      REX_STORE_U32(clear_ea, 0u);
    }

    if ((flags & 0x4u) != 0u) {
      uint32_t entry_ea = 0x82C5D878u + ((frame_slot << 11) & 0xFFFFF800u);
      const uint32_t target_id = static_cast<uint32_t>(light_id);
      for (int32_t idx = 1; idx < 8; ++idx, entry_ea += 256u) {
        if (REX_LOAD_U32(entry_ea) == target_id) {
          ctx.r3.s64 = static_cast<int64_t>(idx);
          return;
        }
      }
    }
  }

  if (slot_param != -1) {
    const uint32_t table_idx = ((frame_slot << 6) & 0xFFFFFFC0u) + static_cast<uint32_t>(slot_param);
    const uint32_t flag_ea = 0x82C5E760u + (table_idx * 240u);
    const int32_t next_slot = static_cast<int32_t>(static_cast<uint32_t>(slot_param) + 8u);
    ctx.r3.s64 = static_cast<int64_t>(next_slot);
    if (REX_LOAD_U32(flag_ea) != 0u) {
      return;
    }
  }

  ctx.r3.s64 = -1;
}

// ----------------------------------------------------------------------------
// 4. sub_828C8DB0: RAGE Effect Vertex Parameter Pass Tables
// Hardware verified: 5/5 MATCH PASS, 1.14x speedup
// ----------------------------------------------------------------------------
inline void OptSub828C8DB0(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

  const uint64_t saved_lr = ctx.lr;
  const uint32_t saved_r1 = ctx.r1.u32;
  const uint32_t frame_r1 = saved_r1 - 144u;
  REX_STORE_U32(frame_r1, saved_r1);
  ctx.r1.u32 = frame_r1;

  const uint32_t pass = ctx.r3.u32;
  const uint32_t arg1 = ctx.r4.u32;
  const uint32_t shader_data = ctx.r5.u32;
  const uint32_t effect_ptr = ctx.r6.u32;

  ctx.r3.u64 = arg1;
  ctx.lr = 0x828C8DD4u;
  sub_828DFF00(ctx, base);

  const uint32_t param_count = REX_LOAD_U16(pass + 8u);
  if (static_cast<int32_t>(param_count) > 0) {
    const uint32_t pair_table = REX_LOAD_U32(pass + 4u);
    const uint32_t type_table = REX_LOAD_U32(shader_data + 16u);
    const uint32_t val_table  = REX_LOAD_U32(shader_data + 0u);
    const uint32_t effect_base = REX_LOAD_U32(effect_ptr + 0u);

    for (uint32_t i = 0u; i < param_count; ++i) {
      const uint16_t pair = REX_LOAD_U16(pair_table + (i << 1));
      const uint32_t idx0 = static_cast<uint32_t>(pair >> 8);
      const uint32_t param_idx = static_cast<uint32_t>(pair & 0xFFu);
      const uint8_t is_const = REX_LOAD_U8(type_table + idx0);
      const uint32_t val = REX_LOAD_U32(val_table + (idx0 << 2));

      if (is_const == 0u) {
        const uint32_t desc = effect_base + (idx0 * 48u);
        const uint32_t override_count = static_cast<uint32_t>(REX_LOAD_U8(desc + 2u) >> 1);
        const uint32_t overrides = REX_LOAD_U32(desc + 24u);
        ctx.r3.u64 = arg1;
        ctx.r4.u64 = param_idx;
        ctx.r5.u64 = val;
        ctx.r6.u64 = override_count;
        ctx.r7.u64 = overrides;
        ctx.lr = 0x828C8E3Cu;
        sub_828E01B8(ctx, base);
      } else {
        ctx.r3.u64 = arg1;
        ctx.r4.u64 = param_idx;
        ctx.r5.u64 = val;
        ctx.r6.u64 = is_const;
        ctx.lr = 0x828C8E64u;
        sub_828E0348(ctx, base);
      }
    }
  }

  ctx.r1.u32 = saved_r1;
  ctx.r12.u64 = saved_lr;
  ctx.lr = saved_lr;
}

// ----------------------------------------------------------------------------
// 5. sub_828C8E80: RAGE Effect Pixel Parameter Pass Tables
// Hardware verified: 5/5 MATCH PASS, 1.67x speedup (31.9 us vs 53.5 us)
// ----------------------------------------------------------------------------
inline void OptSub828C8E80(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();

  const uint64_t saved_lr = ctx.lr;
  const uint32_t saved_r1 = ctx.r1.u32;
  const uint32_t frame_r1 = saved_r1 - 144u;
  REX_STORE_U32(frame_r1, saved_r1);
  ctx.r1.u32 = frame_r1;

  const uint32_t pass = ctx.r3.u32;
  const uint32_t arg1 = ctx.r4.u32;
  const uint32_t shader_data = ctx.r5.u32;
  const uint32_t effect_ptr = ctx.r6.u32;

  ctx.r3.u64 = arg1;
  ctx.lr = 0x828C8EA8u;
  sub_828DFF28(ctx, base);

  const uint32_t param_count = REX_LOAD_U16(pass + 8u);
  if (static_cast<int32_t>(param_count) > 0) {
    const uint32_t pair_table = REX_LOAD_U32(pass + 4u);
    const uint32_t type_table = REX_LOAD_U32(shader_data + 16u);
    const uint32_t val_table  = REX_LOAD_U32(shader_data + 0u);
    const uint32_t effect_base = REX_LOAD_U32(effect_ptr + 0u);

    for (uint32_t i = 0u; i < param_count; ++i) {
      const uint16_t pair = REX_LOAD_U16(pair_table + (i << 1));
      const uint32_t idx0 = static_cast<uint32_t>(pair >> 8);
      const uint32_t param_idx = static_cast<uint32_t>(pair & 0xFFu);
      const uint8_t is_const = REX_LOAD_U8(type_table + idx0);
      const uint32_t val = REX_LOAD_U32(val_table + (idx0 << 2));

      if (is_const == 0u) {
        const uint32_t desc = effect_base + (idx0 * 48u);
        const uint32_t override_count = static_cast<uint32_t>(REX_LOAD_U8(desc + 2u) >> 1);
        const uint32_t overrides = REX_LOAD_U32(desc + 24u);
        ctx.r3.u64 = arg1;
        ctx.r4.u64 = param_idx;
        ctx.r5.u64 = val;
        ctx.r6.u64 = override_count;
        ctx.r7.u64 = overrides;
        ctx.lr = 0x828C8F08u;
        sub_828E02B0(ctx, base);
      } else {
        ctx.r3.u64 = arg1;
        ctx.r4.u64 = param_idx;
        ctx.r5.u64 = val;
        ctx.r6.u64 = is_const;
        ctx.lr = 0x828C8F1Cu;
        sub_828E0360(ctx, base);
      }
    }
  }

  ctx.r1.u32 = saved_r1;
  ctx.r12.u64 = saved_lr;
  ctx.lr = saved_lr;
}
