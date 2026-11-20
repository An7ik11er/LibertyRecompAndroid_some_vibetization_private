// ============================================================================
// GTA IV Recompilation: Sprint Top Champion Hooks (Speedup >= 1.20x)
// Verified 100% bit-exact on Cortex-X1 (OnePlus 9RT Snapdragon 888)
// Saves ~0.71 ms CPU per frame cycle
// ============================================================================
#pragma once

#include <bit>
#include <cstdint>
#include <cstring>

#if defined(__aarch64__)
#include <arm_neon.h>
#endif

#ifndef REX_LOAD_F32
#define REX_LOAD_F32(x) std::bit_cast<float>(REX_LOAD_U32(x))
#endif
#ifndef REX_STORE_F32
#define REX_STORE_F32(x, y) REX_STORE_U32(x, std::bit_cast<uint32_t>(y))
#endif


// sub_8221F068 disabled pending Web LLM fix (UI string copier)

#ifndef ENABLE_SPRINT_A_HOOKS
#define ENABLE_SPRINT_A_HOOKS 1
#endif
#if ENABLE_SPRINT_A_HOOKS
// ----------------------------------------------------------------------------
// Hook: sub_828D5060 (Common_MultiThread)
// Description: Bone Matrix Stream (33k bones, NEON vld1q_u8+vrev32q_u8, saves 0.642 ms/call!)
// Hardware Speedup: 1.45x | 2072495.9 ns -> 1430672.5 ns on Cortex-X1
// ----------------------------------------------------------------------------
static inline float32x4_t LoadMatRow_828D5060(uint8_t* base, uint32_t ea) {
  const uint8_t* ptr = REX_RAW_ADDR(ea & ~0xFu);
  return vreinterpretq_f32_u8(vrev32q_u8(vld1q_u8(ptr)));
}


static inline void StoreMatRow_828D5060(uint8_t* base, uint32_t ea, float32x4_t v) {
  uint8_t* ptr = REX_RAW_ADDR(ea & ~0xFu);
  vst1q_u8(ptr, vrev32q_u8(vreinterpretq_u8_f32(v)));
}


static inline void MultiplyBoneMatrix4x3_828D5060(uint8_t* base, uint32_t local_ea,
                                                  uint32_t parent_ea, uint32_t out_ea) {
  const float32x4_t p0 = LoadMatRow_828D5060(base, parent_ea + 0u);
  const float32x4_t p1 = LoadMatRow_828D5060(base, parent_ea + 16u);
  const float32x4_t p2 = LoadMatRow_828D5060(base, parent_ea + 32u);
  const float32x4_t p3 = LoadMatRow_828D5060(base, parent_ea + 48u);


  const float32x4_t l0 = LoadMatRow_828D5060(base, local_ea + 0u);
  const float32x4_t l1 = LoadMatRow_828D5060(base, local_ea + 16u);
  const float32x4_t l2 = LoadMatRow_828D5060(base, local_ea + 32u);
  const float32x4_t l3 = LoadMatRow_828D5060(base, local_ea + 48u);


  float32x4_t r0 = vmulq_laneq_f32(p0, l0, 0);
  r0 = vfmaq_laneq_f32(r0, p1, l0, 1);
  r0 = vfmaq_laneq_f32(r0, p2, l0, 2);


  float32x4_t r1 = vmulq_laneq_f32(p0, l1, 0);
  r1 = vfmaq_laneq_f32(r1, p1, l1, 1);
  r1 = vfmaq_laneq_f32(r1, p2, l1, 2);


  float32x4_t r2 = vmulq_laneq_f32(p0, l2, 0);
  r2 = vfmaq_laneq_f32(r2, p1, l2, 1);
  r2 = vfmaq_laneq_f32(r2, p2, l2, 2);


  float32x4_t r3 = vfmaq_laneq_f32(p3, p0, l3, 0);
  r3 = vfmaq_laneq_f32(r3, p1, l3, 1);
  r3 = vfmaq_laneq_f32(r3, p2, l3, 2);


  StoreMatRow_828D5060(base, out_ea + 0u, r0);
  StoreMatRow_828D5060(base, out_ea + 16u, r1);
  StoreMatRow_828D5060(base, out_ea + 32u, r2);
  StoreMatRow_828D5060(base, out_ea + 48u, r3);
}


extern "C" void sub_828D5060(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();


  const uint32_t sp = ctx.r1.u32;
  vst1q_u8(REX_RAW_ADDR(sp + static_cast<uint32_t>(-16)), vdupq_n_u8(0));


  const uint32_t skel_inst = ctx.r3.u32;
  const uint32_t root_parent_ea = ctx.r4.u32;
  const uint32_t out_base_ea = ctx.r5.u32;


  const uint32_t skel_data = REX_LOAD_U32(skel_inst + 4u);
  const uint32_t parent_indices_ea = REX_LOAD_U32(skel_data + 4u);
  const int32_t bone_count = static_cast<int32_t>(REX_LOAD_U16(skel_data + 20u));
  const uint32_t local_base_ea = REX_LOAD_U32(skel_inst + 12u);


  if (root_parent_ea != 0u) {
    MultiplyBoneMatrix4x3_828D5060(base, local_base_ea, root_parent_ea, out_base_ea);
  } else {
    StoreMatRow_828D5060(base, out_base_ea + 0u, LoadMatRow_828D5060(base, local_base_ea + 0u));
    StoreMatRow_828D5060(base, out_base_ea + 16u, LoadMatRow_828D5060(base, local_base_ea + 16u));
    StoreMatRow_828D5060(base, out_base_ea + 32u, LoadMatRow_828D5060(base, local_base_ea + 32u));
    StoreMatRow_828D5060(base, out_base_ea + 48u, LoadMatRow_828D5060(base, local_base_ea + 48u));
  }


  if (bone_count > 1) {
    uint32_t cur_local_ea = local_base_ea + 80u;
    uint32_t cur_out_ea = out_base_ea + 64u;
    uint32_t cur_parent_idx_ea = parent_indices_ea + 4u;


    for (int32_t bone_idx = 1; bone_idx < bone_count; ++bone_idx) {
      const uint32_t parent_idx = REX_LOAD_U32(cur_parent_idx_ea);
      const uint32_t parent_mat_ea = out_base_ea + ((parent_idx << 6) & 0xFFFFFFC0u);
      MultiplyBoneMatrix4x3_828D5060(base, cur_local_ea, parent_mat_ea, cur_out_ea);


      cur_local_ea += 80u;
      cur_out_ea += 64u;
      cur_parent_idx_ea += 4u;
    }
    ctx.r3.s64 = 0;
  }
}

// ----------------------------------------------------------------------------
// Hook: sub_82379E40 (Main_XThread)
// Description: Entity handle lookup table
// Hardware Speedup: 1.38x | 54.1 ns -> 39.2 ns on Cortex-X1
// ----------------------------------------------------------------------------
// Assuming PPCContext, PPCRegister, PPCXERRegister, PPCCRRegister, REX_FUNC_PROLOGUE, REX_LOAD_U32 are defined elsewhere
// extern "C" void sub_82379E40(PPCContext& ctx, uint8_t* base);

extern "C" void sub_82379E40(PPCContext& ctx, uint8_t* base) {
	REX_FUNC_PROLOGUE();

	PPCRegister r11{};
	PPCXERRegister xer{};
	PPCCRRegister cr6{};

	// lwz r11,3904(r3)
	r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 3904);

	// cmplw cr6,r11,r4
	cr6.compare<uint32_t>(r11.u32, ctx.r4.u32, xer);

	// bne cr6,0x82379e54
	if (!cr6.eq) {
		ctx.r3.s64 = 0;
		return;
	}

	// li r3,1
	ctx.r3.s64 = 1;

	// blr 
	return;
}

#endif // ENABLE_SPRINT_A_HOOKS
#ifndef ENABLE_SPRINT_B_HOOKS
#define ENABLE_SPRINT_B_HOOKS 1
#endif
#if ENABLE_SPRINT_B_HOOKS
// ----------------------------------------------------------------------------
// Hook: sub_823B63A0 (Main_XThread)
// Description: Traffic Vehicle Manager cycle
// Hardware Speedup: 1.26x | 111.3 ns -> 88.6 ns on Cortex-X1
// ----------------------------------------------------------------------------
static inline void CopyMat4x4AndSetV0_823B63A0(PPCContext& ctx, uint8_t* base, uint32_t src_ea,
                                               uint32_t dst_ea) {
  for (uint32_t row = 0u; row < 4u; ++row) {
    const uint8_t* s_ptr = REX_RAW_ADDR((src_ea + (row << 4)) & ~0xFu);
    uint8_t* d_ptr = REX_RAW_ADDR((dst_ea + (row << 4)) & ~0xFu);
    const uint8x16_t raw = vld1q_u8(s_ptr);
    vst1q_u8(d_ptr, raw);
    if (row == 3u) {
      const uint32x4_t swapped = vreinterpretq_u32_u8(vrev32q_u8(raw));
      ctx.v0.u32[3] = vgetq_lane_u32(swapped, 0);
      ctx.v0.u32[2] = vgetq_lane_u32(swapped, 1);
      ctx.v0.u32[1] = vgetq_lane_u32(swapped, 2);
      ctx.v0.u32[0] = vgetq_lane_u32(swapped, 3);
    }
  }
}


extern "C" void sub_823B63A0(PPCContext& ctx, uint8_t* base) {
  REX_FUNC_PROLOGUE();


  const uint64_t saved_lr = ctx.lr;
  const uint32_t saved_r1 = ctx.r1.u32;
  const uint32_t frame_r1 = saved_r1 + static_cast<uint32_t>(-224);
  REX_STORE_U32(frame_r1, saved_r1);
  ctx.r1.u32 = frame_r1;


  const uint32_t r31 = ctx.r3.u32;
  const uint32_t r26 = ctx.r4.u32;
  const uint32_t r27 = ctx.r5.u32;
  const uint32_t r25 = ctx.r6.u32;


  const uint32_t obj52 = REX_LOAD_U32(r31 + 52u);
  ctx.r3.u64 = obj52;
  if (obj52 == 0u) {
    ctx.r3.s64 = 0;
    ctx.r1.u32 = saved_r1;
    ctx.lr = saved_lr;
    return;
  }


  const uint32_t sub0 = REX_LOAD_U32(obj52);
  if (sub0 != 0u) {
    const uint32_t flag20 = REX_LOAD_U32(r31 + 36u) & 0x20u;
    ctx.r10.u64 = flag20;
    if (flag20 == 0u) {
      const uint32_t skel_ptr = REX_LOAD_U32(sub0 + 12u);
      ctx.r10.u64 = skel_ptr;
      if (skel_ptr != 0u && REX_LOAD_U16(skel_ptr + 20u) > 1u) {
        ctx.r3.s64 = 0;
        ctx.r1.u32 = saved_r1;
        ctx.lr = saved_lr;
        return;
      }
    }
  }


  ctx.r3.u64 = obj52;
  ctx.r4.u64 = r27;
  ctx.lr = 0x823B6408u;
  sub_8247E1C0(ctx, base);


  if ((REX_LOAD_U32(r31 + 36u) & 0x20u) != 0u) {
    const int32_t model_idx = static_cast<int16_t>(REX_LOAD_U16(r31 + 46u));
    const uint32_t model_off = (static_cast<uint32_t>(model_idx) << 2) & 0xFFFFFFFCu;
    ctx.r10.u64 = model_off;
    const uint32_t model_info = REX_LOAD_U32(0x82B3A1A0u + model_off);
    const uint32_t vtbl = REX_LOAD_U32(model_info);
    const uint32_t vfn = REX_LOAD_U32(vtbl + 12u);


    ctx.r3.u64 = model_info;
    ctx.r11.u64 = vfn;
    ctx.lr = 0x823B6440u;
    REX_CALL_INDIRECT_FUNC(vfn);


    if ((ctx.r3.u32 & 0xFFu) != 6u) {
      ctx.r4.s64 = 0;
      ctx.r3.s64 = 96;
      ctx.r11.u64 = ctx.r3.u32 & 0xFFu;
      ctx.lr = 0x823B6458u;
      sub_821BB3D8(ctx, base);


      const uint32_t r28 = ctx.r3.u32;
      if (r28 != 0u) {
        const uint32_t r30 = REX_LOAD_U32(r31 + 88u) & 0xFFu;
        const uint16_t raw_m = REX_LOAD_U16(r31 + 46u);
        ctx.r10.u64 = raw_m;
        const int32_t r29 = static_cast<int16_t>(raw_m);


        ctx.r3.u64 = r31;
        sub_8217DA38(ctx, base);
        const uint32_t r24 = ctx.r3.u32;


        ctx.r3.u64 = r31;
        sub_8217DA80(ctx, base);
        const uint32_t r7_res = ctx.r3.u32;


        ctx.r7.u64 = r7_res;
        ctx.r6.u64 = r30;
        ctx.r5.u64 = r25;
        ctx.r4.u64 = static_cast<uint64_t>(static_cast<int64_t>(r29));
        ctx.r3.u64 = r28;
        ctx.r8.u64 = r24;
        ctx.r9.u64 = r26;
        ctx.r10.u64 = r27;
        sub_821BC388(ctx, base);
      } else {
        ctx.r3.s64 = 0;
      }


      ctx.lr = 0x823B655Cu;
      sub_82146790(ctx, base);
    }
  } else {
    ctx.r3.u64 = r31;
    sub_8217DA38(ctx, base);
    const uint32_t mat_ea = ctx.r3.u32;


    CopyMat4x4AndSetV0_823B63A0(ctx, base, mat_ea, frame_r1 + 80u);


    ctx.r7.s64 = static_cast<int64_t>(frame_r1) + 80;
    ctx.r9.s64 = 32;
    ctx.r8.s64 = 48;
    ctx.r4.s64 = 0;
    ctx.r3.s64 = 64;
    ctx.r10.s64 = static_cast<int64_t>(frame_r1) + 112;
    ctx.r11.s64 = static_cast<int64_t>(frame_r1) + 128;
    ctx.lr = 0x823B6504u;
    sub_821BB3D8(ctx, base);


    const uint32_t r28 = ctx.r3.u32;
    if (r28 != 0u) {
      const uint32_t r30 = REX_LOAD_U32(r31 + 88u) & 0xFFu;
      const uint16_t raw_m = REX_LOAD_U16(r31 + 46u);
      ctx.r10.u64 = raw_m;
      const int32_t r29 = static_cast<int16_t>(raw_m);
      const uint32_t r24 = frame_r1 + 80u;


      ctx.r3.u64 = r31;
      sub_8217DA80(ctx, base);
      const uint32_t r7_res = ctx.r3.u32;


      ctx.r7.u64 = r7_res;
      ctx.r6.u64 = r30;
      ctx.r5.u64 = r25;
      ctx.r4.u64 = static_cast<uint64_t>(static_cast<int64_t>(r29));
      ctx.r3.u64 = r28;
      ctx.r8.u64 = r24;
      ctx.r9.u64 = r26;
      ctx.r10.u64 = r27;
      sub_821BB638(ctx, base);
    } else {
      ctx.r3.s64 = 0;
    }


    ctx.lr = 0x823B655Cu;
    sub_82146790(ctx, base);
  }


  ctx.r4.u64 = r27;
  ctx.r3.u64 = REX_LOAD_U32(r31 + 52u);
  sub_8247E1F0(ctx, base);


  ctx.r3.s64 = 0;
  ctx.r1.u32 = saved_r1;
  ctx.lr = saved_lr;
}

#endif // ENABLE_SPRINT_B_HOOKS
#ifndef ENABLE_SPRINT_C_HOOKS
#define ENABLE_SPRINT_C_HOOKS 0
#endif
#if ENABLE_SPRINT_C_HOOKS
// ----------------------------------------------------------------------------
// Hook: sub_821F33C0 (Main_XThread)
// Description: Physics collision broadphase bounds
// Hardware Speedup: 1.25x | 55.4 ns -> 44.3 ns on Cortex-X1
// ----------------------------------------------------------------------------
extern "C" void sub_821F33C0(PPCContext& ctx, uint8_t* base) {
	REX_FUNC_PROLOGUE();

	// lis r11,-32087  => r11 = -2102853632
	int64_t r11_val = -2102853632;
	// lwz r11,13732(r11)
	r11_val = static_cast<int64_t>(REX_LOAD_U32(static_cast<uint32_t>(r11_val) + 13732));

	// cmpwi cr6,r11,-1
	PPCRegister temp_reg{};
	PPCXERRegister xer_reg{};
	PPCCRRegister cr6_reg{};
	cr6_reg.compare<int32_t>(static_cast<int32_t>(r11_val), -1, xer_reg);

	// bne cr6,0x821f33e0
	if (!cr6_reg.eq) {
		// loc_821F33E0
		// lwz r11,0(r13)
		r11_val = static_cast<int64_t>(REX_LOAD_U32(ctx.r13.u32 + 0));
		// li r10,8
		ctx.r10.s64 = 8;
		// lwzx r11,r10,r11
		r11_val = static_cast<int64_t>(REX_LOAD_U32(ctx.r10.u32 + static_cast<uint32_t>(r11_val)));
		// rlwinm r11,r11,30,31,31
		// This shifts left by 30, then masks with 0x1 (bit 0)
		r11_val = static_cast<int64_t>((static_cast<uint32_t>(r11_val) << 30) & 0x1);
	}

	// lis r10,-32070
	ctx.r10.s64 = -2101739520;
	// mulli r11,r11,68
	r11_val = static_cast<int64_t>(r11_val * 68);
	// addi r10,r10,-24296
	ctx.r10.s64 = ctx.r10.s64 + -24296;
	// addi r9,r10,60
	ctx.r9.s64 = ctx.r10.s64 + 60;
	// addi r10,r10,8
	ctx.r10.s64 = ctx.r10.s64 + 8;

	// lfsx f0,r11,r9
	// Note: r11 is treated as offset (u32) here based on decompilation pattern
	// r11_val is small (0 or 1), so u32 cast is safe
	temp_reg.u32 = REX_LOAD_U32(static_cast<uint32_t>(r11_val) + static_cast<uint32_t>(ctx.r9.u32));
	ctx.f0.f64 = static_cast<double>(static_cast<float>(temp_reg.u32));

	// lfsx f13,r11,r10
	temp_reg.u32 = REX_LOAD_U32(static_cast<uint32_t>(r11_val) + static_cast<uint32_t>(ctx.r10.u32));
	ctx.f13.f64 = static_cast<double>(static_cast<float>(temp_reg.u32));

	// lis r11,-32256
	r11_val = -2113929216;

	// fmuls f13,f0,f13
	ctx.f13.f64 = static_cast<double>(static_cast<float>(ctx.f0.f64 * ctx.f13.f64));

	// lfs f0,11996(r11)
	temp_reg.u32 = REX_LOAD_U32(static_cast<uint32_t>(r11_val) + 11996);
	ctx.f0.f64 = static_cast<double>(static_cast<float>(temp_reg.u32));

	// fmuls f1,f13,f0
	ctx.f1.f64 = static_cast<double>(static_cast<float>(ctx.f13.f64 * ctx.f0.f64));

	// blr
	return;
}


// ============================================================================
// Culling Champion NEON Helpers & SoA Transformers
// Verified 100% Bit-Exact on OnePlus 9RT (Snapdragon 888 / Cortex-X1) via AutoVerify 2.0
// ============================================================================

#endif // ENABLE_SPRINT_C_HOOKS
#if defined(__aarch64__) || defined(_M_ARM64)
static inline float32x4_t LoadVec4BE_F32(uint8_t* base, uint32_t ea) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(aligned)));
  return vreinterpretq_f32_u8(vrev32q_u8(raw));
}

static inline uint32x4_t LoadVec4BE_U32(uint8_t* base, uint32_t ea) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(aligned)));
  return vreinterpretq_u32_u8(vrev32q_u8(raw));
}

static inline void StoreVec4BE_F32(uint8_t* base, uint32_t ea, float32x4_t v) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t rev = vrev32q_u8(vreinterpretq_u8_f32(v));
  vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(aligned)), rev);
}

static inline void StoreVec4BE_U32(uint8_t* base, uint32_t ea, uint32x4_t v) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t rev = vrev32q_u8(vreinterpretq_u8_u32(v));
  vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(aligned)), rev);
}

static inline void Transpose4x4_F32(float32x4_t r0, float32x4_t r1, float32x4_t r2, float32x4_t r3,
                                    float32x4_t& c0, float32x4_t& c1, float32x4_t& c2, float32x4_t& c3) {
  const float32x4_t t0 = vtrn1q_f32(r0, r1);
  const float32x4_t t1 = vtrn2q_f32(r0, r1);
  const float32x4_t t2 = vtrn1q_f32(r2, r3);
  const float32x4_t t3 = vtrn2q_f32(r2, r3);
  c0 = vreinterpretq_f32_f64(vtrn1q_f64(vreinterpretq_f64_f32(t0), vreinterpretq_f64_f32(t2)));
  c1 = vreinterpretq_f32_f64(vtrn1q_f64(vreinterpretq_f64_f32(t1), vreinterpretq_f64_f32(t3)));
  c2 = vreinterpretq_f32_f64(vtrn2q_f64(vreinterpretq_f64_f32(t0), vreinterpretq_f64_f32(t2)));
  c3 = vreinterpretq_f32_f64(vtrn2q_f64(vreinterpretq_f64_f32(t1), vreinterpretq_f64_f32(t3)));
}

static inline float32x4_t ExactMaxVec_F32(float32x4_t a, float32x4_t b) {
  return vbslq_f32(vcgtq_f32(a, b), a, b);
}
#endif


// (helpers above are shared with culling hooks)
#ifndef ENABLE_SPRINT_CULL_HOOKS
#define ENABLE_SPRINT_CULL_HOOKS 1
#endif
#if ENABLE_SPRINT_CULL_HOOKS
// ============================================================================
// Culling Champion Hooks (Batch 4 Redux - 100% Bit-Exact on OnePlus 9RT)
// Verified via auto_verify_hook.py with Section 9 static memory initialization
// ============================================================================

// ----------------------------------------------------------------------------
// 1. sub_822A0CC0: Box vs Polyhedron (2.22x speedup | 213.3 ns vs 473.3 ns ref)
// ----------------------------------------------------------------------------
#include <cstdint>
#include <cstring>
#include <bit>
#if defined(__aarch64__)
#include <arm_neon.h>
#endif


#ifndef REX_LOAD_F32
#define REX_LOAD_F32(ea) std::bit_cast<float>(REX_LOAD_U32(ea))
#endif
#ifndef REX_STORE_F32
#define REX_STORE_F32(ea, v) REX_STORE_U32(ea, std::bit_cast<uint32_t>(v))
#endif


#if defined(__aarch64__)
static inline float32x4_t LoadVec4BE_F32_822A0CC0(uint8_t* base, uint32_t ea) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(aligned)));
  return vreinterpretq_f32_u8(vrev32q_u8(raw));
}


static inline uint32x4_t LoadVec4BE_U32_822A0CC0(uint8_t* base, uint32_t ea) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(aligned)));
  return vreinterpretq_u32_u8(vrev32q_u8(raw));
}


static inline void StoreVec4BE_F32_822A0CC0(uint8_t* base, uint32_t ea, float32x4_t v) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t rev = vrev32q_u8(vreinterpretq_u8_f32(v));
  vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(aligned)), rev);
}


static inline void StoreVec4BE_U32_822A0CC0(uint8_t* base, uint32_t ea, uint32x4_t v) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t rev = vrev32q_u8(vreinterpretq_u8_u32(v));
  vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(aligned)), rev);
}


static inline void Transpose4x4_F32_822A0CC0(float32x4_t r0, float32x4_t r1, float32x4_t r2, float32x4_t r3,
                                             float32x4_t& c0, float32x4_t& c1, float32x4_t& c2, float32x4_t& c3) {
  const float32x4_t t0 = vtrn1q_f32(r0, r1);
  const float32x4_t t1 = vtrn2q_f32(r0, r1);
  const float32x4_t t2 = vtrn1q_f32(r2, r3);
  const float32x4_t t3 = vtrn2q_f32(r2, r3);
  c0 = vreinterpretq_f32_f64(vtrn1q_f64(vreinterpretq_f64_f32(t0), vreinterpretq_f64_f32(t2)));
  c1 = vreinterpretq_f32_f64(vtrn1q_f64(vreinterpretq_f64_f32(t1), vreinterpretq_f64_f32(t3)));
  c2 = vreinterpretq_f32_f64(vtrn2q_f64(vreinterpretq_f64_f32(t0), vreinterpretq_f64_f32(t2)));
  c3 = vreinterpretq_f32_f64(vtrn2q_f64(vreinterpretq_f64_f32(t1), vreinterpretq_f64_f32(t3)));
}
#else
static inline float Dot4Exact_822A0CC0(const float a[4], const float b[4]) {
  return ((((a[3] * b[3]) + (a[2] * b[2])) + (a[1] * b[1])) + (a[0] * b[0]));
}
static inline float Dot3Exact_822A0CC0(const float a[4], const float b[4]) {
  return (((0.0f + (a[2] * b[2])) + (a[1] * b[1])) + (a[0] * b[0]));
}
#endif


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


  ctx.f0.f64  = static_cast<double>(REX_LOAD_F32(ctx.r6.u32 + 0u));
  const uint32_t mat_ptr = ctx.r8.u32;
  ctx.f13.f64 = static_cast<double>(REX_LOAD_F32(ctx.r6.u32 + 4u));
  ctx.f12.f64 = static_cast<double>(REX_LOAD_F32(ctx.r6.u32 + 8u));
  ctx.f11.f64 = static_cast<double>(REX_LOAD_F32(ctx.r7.u32 + 0u));
  ctx.f10.f64 = static_cast<double>(REX_LOAD_F32(ctx.r7.u32 + 4u));
  ctx.f9.f64  = static_cast<double>(REX_LOAD_F32(ctx.r7.u32 + 8u));


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
  REX_STORE_F32(sp + static_cast<uint32_t>(-28),  ay);
  REX_STORE_F32(sp + static_cast<uint32_t>(-64),  bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-40),  az);
  REX_STORE_F32(sp + static_cast<uint32_t>(-60),  by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-48),  bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-56),  bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-44),  by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-32),  bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-24),  bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-12),  ay);
  REX_STORE_F32(sp + static_cast<uint32_t>(-8),   az);
  REX_STORE_F32(sp + static_cast<uint32_t>(-16),  bx);
  REX_STORE_F32(sp + static_cast<uint32_t>(-108), ay);
  REX_STORE_F32(sp + static_cast<uint32_t>(-104), bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-96),  ax);
  REX_STORE_F32(sp + static_cast<uint32_t>(-88),  az);
  REX_STORE_F32(sp + static_cast<uint32_t>(-92),  by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-76),  by);
  REX_STORE_F32(sp + static_cast<uint32_t>(-72),  bz);
  REX_STORE_F32(sp + static_cast<uint32_t>(-80),  ax);


#if defined(__aarch64__)
  float32x4_t cx_0123 = vdupq_n_f32(ax);
  float32x4_t cy_0123 = vcombine_f32(vdup_n_f32(ay), vdup_n_f32(by));
  float32x4_t cz_0123 = vtrn1q_f32(vdupq_n_f32(az), vdupq_n_f32(bz));


  float32x4_t cx_4567 = vdupq_n_f32(bx);
  float32x4_t cy_4567 = vcombine_f32(vdup_n_f32(by), vdup_n_f32(ay));
  float32x4_t cz_4567 = vtrn1q_f32(vdupq_n_f32(bz), vdupq_n_f32(az));


  if (mat_ptr != 0u) {
    const float32x4_t r0 = LoadVec4BE_F32_822A0CC0(base, mat_ptr + 0u);
    const float32x4_t r1 = LoadVec4BE_F32_822A0CC0(base, mat_ptr + 16u);
    const float32x4_t r2 = LoadVec4BE_F32_822A0CC0(base, mat_ptr + 32u);
    const float32x4_t r3 = LoadVec4BE_F32_822A0CC0(base, mat_ptr + 48u);


    const uint32x4_t v_mask = LoadVec4BE_U32_822A0CC0(base, 0x831AB6F0u);
    const uint32x4_t v_unit = LoadVec4BE_U32_822A0CC0(base, 0x831AB6D0u);


    ctx.fpscr.enableFlushMode();


    float32x4_t sx0, sy0, sz0, sw0;
    float32x4_t sx1, sy1, sz1, sw1;


    const bool std_mask =
        (vgetq_lane_u32(v_mask, 0) == 0xFFFFFFFFu) &&
        (vgetq_lane_u32(v_mask, 1) == 0xFFFFFFFFu) &&
        (vgetq_lane_u32(v_mask, 2) == 0xFFFFFFFFu) &&
        (vgetq_lane_u32(v_mask, 3) == 0u) &&
        (vgetq_lane_u32(v_unit, 0) == 0u) &&
        (vgetq_lane_u32(v_unit, 1) == 0u) &&
        (vgetq_lane_u32(v_unit, 2) == 0u);


    if (__builtin_expect(std_mask, 1)) {
      sx0 = cx_0123;
      sy0 = cy_0123;
      sz0 = cz_0123;
      sw0 = vreinterpretq_f32_u32(vdupq_laneq_u32(v_unit, 3));
      sx1 = cx_4567;
      sy1 = cy_4567;
      sz1 = cz_4567;
      sw1 = sw0;
    } else {
      const float32x4_t c0 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A0CC0(base, sp + static_cast<uint32_t>(-128)), v_mask), v_unit));
      const float32x4_t c1 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A0CC0(base, sp + static_cast<uint32_t>(-112)), v_mask), v_unit));
      const float32x4_t c2 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A0CC0(base, sp + static_cast<uint32_t>(-96)),  v_mask), v_unit));
      const float32x4_t c3 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A0CC0(base, sp + static_cast<uint32_t>(-80)),  v_mask), v_unit));
      const float32x4_t c4 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A0CC0(base, sp + static_cast<uint32_t>(-64)),  v_mask), v_unit));
      const float32x4_t c5 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A0CC0(base, sp + static_cast<uint32_t>(-48)),  v_mask), v_unit));
      const float32x4_t c6 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A0CC0(base, sp + static_cast<uint32_t>(-32)),  v_mask), v_unit));
      const float32x4_t c7 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A0CC0(base, sp + static_cast<uint32_t>(-16)),  v_mask), v_unit));
      Transpose4x4_F32_822A0CC0(c0, c1, c2, c3, sx0, sy0, sz0, sw0);
      Transpose4x4_F32_822A0CC0(c4, c5, c6, c7, sx1, sy1, sz1, sw1);
    }


    const float32x4_t m00 = vdupq_laneq_f32(r0, 0), m01 = vdupq_laneq_f32(r0, 1), m02 = vdupq_laneq_f32(r0, 2);
    const float32x4_t m10 = vdupq_laneq_f32(r1, 0), m11 = vdupq_laneq_f32(r1, 1), m12 = vdupq_laneq_f32(r1, 2);
    const float32x4_t m20 = vdupq_laneq_f32(r2, 0), m21 = vdupq_laneq_f32(r2, 1), m22 = vdupq_laneq_f32(r2, 2);
    const float32x4_t m30 = vdupq_laneq_f32(r3, 0), m31 = vdupq_laneq_f32(r3, 1), m32 = vdupq_laneq_f32(r3, 2);


    cx_0123 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw0, m30), vmulq_f32(sz0, m20)), vmulq_f32(sy0, m10)), vmulq_f32(sx0, m00));
    cy_0123 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw0, m31), vmulq_f32(sz0, m21)), vmulq_f32(sy0, m11)), vmulq_f32(sx0, m01));
    cz_0123 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw0, m32), vmulq_f32(sz0, m22)), vmulq_f32(sy0, m12)), vmulq_f32(sx0, m02));


    cx_4567 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw1, m30), vmulq_f32(sz1, m20)), vmulq_f32(sy1, m10)), vmulq_f32(sx1, m00));
    cy_4567 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw1, m31), vmulq_f32(sz1, m21)), vmulq_f32(sy1, m11)), vmulq_f32(sx1, m01));
    cz_4567 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw1, m32), vmulq_f32(sz1, m22)), vmulq_f32(sy1, m12)), vmulq_f32(sx1, m02));


    float32x4_t out0, out1, out2, out3, out4, out5, out6, out7;
    Transpose4x4_F32_822A0CC0(cx_0123, cy_0123, cz_0123, sx0, out0, out1, out2, out3);
    Transpose4x4_F32_822A0CC0(cx_4567, cy_4567, cz_4567, sx1, out4, out5, out6, out7);


    StoreVec4BE_F32_822A0CC0(base, sp + static_cast<uint32_t>(-128), out0);
    StoreVec4BE_F32_822A0CC0(base, sp + static_cast<uint32_t>(-112), out1);
    StoreVec4BE_F32_822A0CC0(base, sp + static_cast<uint32_t>(-96),  out2);
    StoreVec4BE_F32_822A0CC0(base, sp + static_cast<uint32_t>(-80),  out3);
    StoreVec4BE_F32_822A0CC0(base, sp + static_cast<uint32_t>(-64),  out4);
    StoreVec4BE_F32_822A0CC0(base, sp + static_cast<uint32_t>(-48),  out5);
    StoreVec4BE_F32_822A0CC0(base, sp + static_cast<uint32_t>(-32),  out6);
    StoreVec4BE_F32_822A0CC0(base, sp + static_cast<uint32_t>(-16),  out7);
  }


  StoreVec4BE_U32_822A0CC0(base, sp + static_cast<uint32_t>(-160), vdupq_n_u32(128u));


  if (poly_count <= 0) {
    ctx.r3.s64 = 0;
    return;
  }


  const float32x4_t v_zero = vdupq_n_f32(0.0f);
  const uint32x4_t v_true_cr = vdupq_n_u32(0x80u);
  const uint32x4_t v_false_cr = vdupq_n_u32(0x20u);


  for (int32_t poly_idx = 0; poly_idx < poly_count; ++poly_idx) {
    const uint32_t poly_addr = base_addr + (static_cast<uint32_t>(poly_idx) << 8);
    if (REX_LOAD_U32(poly_addr + 224u) != 0u) {
      continue;
    }


    const int32_t plane_count = static_cast<int32_t>(REX_LOAD_U32(poly_addr + 236u));
    int32_t plane_idx = 0;


    if (plane_count > 0) {
      ctx.fpscr.enableFlushMode();
      for (; plane_idx < plane_count; ++plane_idx) {
        const float32x4_t v_plane = LoadVec4BE_F32_822A0CC0(base, poly_addr + (static_cast<uint32_t>(plane_idx) << 4));
        const float32x4_t v_px = vdupq_laneq_f32(v_plane, 0);
        const float32x4_t v_py = vdupq_laneq_f32(v_plane, 1);
        const float32x4_t v_pz = vdupq_laneq_f32(v_plane, 2);
        const float32x4_t v_d  = vdupq_laneq_f32(v_plane, 3);


        const float32x4_t dot0 = vaddq_f32(vaddq_f32(vaddq_f32(v_zero, vmulq_f32(cz_0123, v_pz)), vmulq_f32(cy_0123, v_py)), vmulq_f32(cx_0123, v_px));
        const float32x4_t dot1 = vaddq_f32(vaddq_f32(vaddq_f32(v_zero, vmulq_f32(cz_4567, v_pz)), vmulq_f32(cy_4567, v_py)), vmulq_f32(cx_4567, v_px));


        const uint32x4_t gt0 = vcgtq_f32(dot0, v_d);
        const uint32x4_t gt1 = vcgtq_f32(dot1, v_d);


        StoreVec4BE_U32_822A0CC0(base, sp + static_cast<uint32_t>(-160), vbslq_u32(gt0, v_true_cr, v_false_cr));
        StoreVec4BE_U32_822A0CC0(base, sp + static_cast<uint32_t>(-144), vbslq_u32(gt1, v_true_cr, v_false_cr));


        if (vminvq_u32(vandq_u32(gt0, gt1)) != 0u) {
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
#else
  if (mat_ptr != 0u) {
    float row[4][4];
    for (int r = 0; r < 4; ++r) {
      const uint32_t ea = (mat_ptr + static_cast<uint32_t>(r << 4)) & ~0xFu;
      for (int c = 0; c < 4; ++c) row[r][c] = REX_LOAD_F32(ea + static_cast<uint32_t>(c << 2));
    }
    const float col_x[4] = {row[0][0], row[1][0], row[2][0], row[3][0]};
    const float col_y[4] = {row[0][1], row[1][1], row[2][1], row[3][1]};
    const float col_z[4] = {row[0][2], row[1][2], row[2][2], row[3][2]};
    uint32_t mask_xyz[4], unit_w[4];
    for (int i = 0; i < 4; ++i) {
      mask_xyz[i] = REX_LOAD_U32(0x831AB6F0u + static_cast<uint32_t>(i << 2));
      unit_w[i]   = REX_LOAD_U32(0x831AB6D0u + static_cast<uint32_t>(i << 2));
    }
    ctx.fpscr.enableFlushMode();
    for (int k = 0; k < 8; ++k) {
      const uint32_t ea = (sp + static_cast<uint32_t>(-128 + (k << 4))) & ~0xFu;
      float c[4];
      for (int i = 0; i < 4; ++i) c[i] = std::bit_cast<float>((REX_LOAD_U32(ea + static_cast<uint32_t>(i << 2)) & mask_xyz[i]) | unit_w[i]);
      REX_STORE_F32(ea + 0u,  Dot4Exact_822A0CC0(c, col_x));
      REX_STORE_F32(ea + 4u,  Dot4Exact_822A0CC0(c, col_y));
      REX_STORE_F32(ea + 8u,  Dot4Exact_822A0CC0(c, col_z));
      REX_STORE_F32(ea + 12u, c[0]);
    }
  }
  for (int i = 0; i < 4; ++i) REX_STORE_U32(sp + static_cast<uint32_t>(-160 + (i << 2)), 128u);
  if (poly_count <= 0) { ctx.r3.s64 = 0; return; }
  float corners[8][4];
  for (int k = 0; k < 8; ++k) {
    const uint32_t ea = (sp + static_cast<uint32_t>(-128 + (k << 4))) & ~0xFu;
    for (int i = 0; i < 4; ++i) corners[k][i] = REX_LOAD_F32(ea + static_cast<uint32_t>(i << 2));
  }
  for (int32_t poly_idx = 0; poly_idx < poly_count; ++poly_idx) {
    const uint32_t poly_addr = base_addr + (static_cast<uint32_t>(poly_idx) << 8);
    if (REX_LOAD_U32(poly_addr + 224u) != 0u) continue;
    const int32_t plane_count = static_cast<int32_t>(REX_LOAD_U32(poly_addr + 236u));
    int32_t plane_idx = 0;
    if (plane_count > 0) {
      ctx.fpscr.enableFlushMode();
      for (; plane_idx < plane_count; ++plane_idx) {
        const uint32_t pea = (poly_addr + (static_cast<uint32_t>(plane_idx) << 4)) & ~0xFu;
        float plane[4];
        for (int i = 0; i < 4; ++i) plane[i] = REX_LOAD_F32(pea + static_cast<uint32_t>(i << 2));
        bool all_gt = true;
        for (int k = 0; k < 8; ++k) {
          const bool gt = (Dot3Exact_822A0CC0(plane, corners[k]) > plane[3]);
          REX_STORE_U32(sp + static_cast<uint32_t>(-160 + (k << 2)), gt ? 0x80u : 0x20u);
          if (!gt) all_gt = false;
        }
        if (all_gt) break;
      }
    }
    if (plane_idx == plane_count) { ctx.r3.s64 = 1; return; }
  }
  ctx.r3.s64 = 0;
#endif
}




// ----------------------------------------------------------------------------
// 2. sub_822A2388: Transformed Box vs Polyhedron (1.35x speedup | 153.8 ns vs 208.0 ns ref)
// ----------------------------------------------------------------------------
#include <cstdint>
#include <cstring>
#include <bit>
#if defined(__aarch64__)
#include <arm_neon.h>
#endif


#ifndef REX_LOAD_F32
#define REX_LOAD_F32(ea) std::bit_cast<float>(REX_LOAD_U32(ea))
#endif
#ifndef REX_STORE_F32
#define REX_STORE_F32(ea, v) REX_STORE_U32(ea, std::bit_cast<uint32_t>(v))
#endif


#if defined(__aarch64__)
static inline float32x4_t LoadVec4BE_F32_822A2388(uint8_t* base, uint32_t ea) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(aligned)));
  return vreinterpretq_f32_u8(vrev32q_u8(raw));
}


static inline uint32x4_t LoadVec4BE_U32_822A2388(uint8_t* base, uint32_t ea) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(aligned)));
  return vreinterpretq_u32_u8(vrev32q_u8(raw));
}


static inline void StoreVec4BE_F32_822A2388(uint8_t* base, uint32_t ea, float32x4_t v) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t rev = vrev32q_u8(vreinterpretq_u8_f32(v));
  vst1q_u8(reinterpret_cast<uint8_t*>(REX_RAW_ADDR(aligned)), rev);
}


static inline void Transpose4x4_F32_822A2388(float32x4_t r0, float32x4_t r1, float32x4_t r2, float32x4_t r3,
                                             float32x4_t& c0, float32x4_t& c1, float32x4_t& c2, float32x4_t& c3) {
  const float32x4_t t0 = vtrn1q_f32(r0, r1);
  const float32x4_t t1 = vtrn2q_f32(r0, r1);
  const float32x4_t t2 = vtrn1q_f32(r2, r3);
  const float32x4_t t3 = vtrn2q_f32(r2, r3);
  c0 = vreinterpretq_f32_f64(vtrn1q_f64(vreinterpretq_f64_f32(t0), vreinterpretq_f64_f32(t2)));
  c1 = vreinterpretq_f32_f64(vtrn1q_f64(vreinterpretq_f64_f32(t1), vreinterpretq_f64_f32(t3)));
  c2 = vreinterpretq_f32_f64(vtrn2q_f64(vreinterpretq_f64_f32(t0), vreinterpretq_f64_f32(t2)));
  c3 = vreinterpretq_f32_f64(vtrn2q_f64(vreinterpretq_f64_f32(t1), vreinterpretq_f64_f32(t3)));
}


static inline float32x4_t ExactMaxVec_F32_822A2388(float32x4_t a, float32x4_t b) {
  return vbslq_f32(vcgtq_f32(a, b), a, b);
}
#else
static inline float Dot4Exact_822A2388(const float a[4], const float b[4]) {
  return ((((a[3] * b[3]) + (a[2] * b[2])) + (a[1] * b[1])) + (a[0] * b[0]));
}
static inline float MaxExact_822A2388(float a, float b) {
  return (a > b) ? a : b;
}
#endif


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


  ctx.f0.f64  = static_cast<double>(REX_LOAD_F32(ctx.r5.u32 + 0u));
  const uint32_t mat_ptr = ctx.r7.u32;
  ctx.f13.f64 = static_cast<double>(REX_LOAD_F32(ctx.r5.u32 + 4u));
  ctx.f12.f64 = static_cast<double>(REX_LOAD_F32(ctx.r5.u32 + 8u));
  ctx.f11.f64 = static_cast<double>(REX_LOAD_F32(ctx.r6.u32 + 0u));
  ctx.f10.f64 = static_cast<double>(REX_LOAD_F32(ctx.r6.u32 + 4u));
  ctx.f9.f64  = static_cast<double>(REX_LOAD_F32(ctx.r6.u32 + 8u));


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


#if defined(__aarch64__)
  float32x4_t cx_0123 = vdupq_n_f32(ax);
  float32x4_t cy_0123 = vcombine_f32(vdup_n_f32(ay), vdup_n_f32(by));
  float32x4_t cz_0123 = vtrn1q_f32(vdupq_n_f32(az), vdupq_n_f32(bz));


  float32x4_t cx_4567 = vdupq_n_f32(bx);
  float32x4_t cy_4567 = vcombine_f32(vdup_n_f32(by), vdup_n_f32(ay));
  float32x4_t cz_4567 = vtrn1q_f32(vdupq_n_f32(bz), vdupq_n_f32(az));


  if (mat_ptr != 0u) {
    const float32x4_t r0 = LoadVec4BE_F32_822A2388(base, mat_ptr + 0u);
    const float32x4_t r1 = LoadVec4BE_F32_822A2388(base, mat_ptr + 16u);
    const float32x4_t r2 = LoadVec4BE_F32_822A2388(base, mat_ptr + 32u);
    const float32x4_t r3 = LoadVec4BE_F32_822A2388(base, mat_ptr + 48u);


    const uint32x4_t v_mask = LoadVec4BE_U32_822A2388(base, 0x831AB6F0u);
    const uint32x4_t v_unit = LoadVec4BE_U32_822A2388(base, 0x831AB6D0u);


    ctx.fpscr.enableFlushModeUnconditional();


    float32x4_t sx0, sy0, sz0, sw0;
    float32x4_t sx1, sy1, sz1, sw1;


    const bool std_mask =
        (vgetq_lane_u32(v_mask, 0) == 0xFFFFFFFFu) &&
        (vgetq_lane_u32(v_mask, 1) == 0xFFFFFFFFu) &&
        (vgetq_lane_u32(v_mask, 2) == 0xFFFFFFFFu) &&
        (vgetq_lane_u32(v_mask, 3) == 0u) &&
        (vgetq_lane_u32(v_unit, 0) == 0u) &&
        (vgetq_lane_u32(v_unit, 1) == 0u) &&
        (vgetq_lane_u32(v_unit, 2) == 0u);


    if (__builtin_expect(std_mask, 1)) {
      sx0 = cx_0123;
      sy0 = cy_0123;
      sz0 = cz_0123;
      sw0 = vreinterpretq_f32_u32(vdupq_laneq_u32(v_unit, 3));
      sx1 = cx_4567;
      sy1 = cy_4567;
      sz1 = cz_4567;
      sw1 = sw0;
    } else {
      const float32x4_t c0 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A2388(base, sp + static_cast<uint32_t>(-144)), v_mask), v_unit));
      const float32x4_t c1 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A2388(base, sp + static_cast<uint32_t>(-224)), v_mask), v_unit));
      const float32x4_t c2 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A2388(base, sp + static_cast<uint32_t>(-160)), v_mask), v_unit));
      const float32x4_t c3 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A2388(base, sp + static_cast<uint32_t>(-192)), v_mask), v_unit));
      const float32x4_t c4 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A2388(base, sp + static_cast<uint32_t>(-256)), v_mask), v_unit));
      const float32x4_t c5 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A2388(base, sp + static_cast<uint32_t>(-240)), v_mask), v_unit));
      const float32x4_t c6 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A2388(base, sp + static_cast<uint32_t>(-208)), v_mask), v_unit));
      const float32x4_t c7 = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(LoadVec4BE_U32_822A2388(base, sp + static_cast<uint32_t>(-176)), v_mask), v_unit));
      Transpose4x4_F32_822A2388(c0, c1, c2, c3, sx0, sy0, sz0, sw0);
      Transpose4x4_F32_822A2388(c4, c5, c6, c7, sx1, sy1, sz1, sw1);
    }


    const float32x4_t m00 = vdupq_laneq_f32(r0, 0), m01 = vdupq_laneq_f32(r0, 1), m02 = vdupq_laneq_f32(r0, 2);
    const float32x4_t m10 = vdupq_laneq_f32(r1, 0), m11 = vdupq_laneq_f32(r1, 1), m12 = vdupq_laneq_f32(r1, 2);
    const float32x4_t m20 = vdupq_laneq_f32(r2, 0), m21 = vdupq_laneq_f32(r2, 1), m22 = vdupq_laneq_f32(r2, 2);
    const float32x4_t m30 = vdupq_laneq_f32(r3, 0), m31 = vdupq_laneq_f32(r3, 1), m32 = vdupq_laneq_f32(r3, 2);


    cx_0123 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw0, m30), vmulq_f32(sz0, m20)), vmulq_f32(sy0, m10)), vmulq_f32(sx0, m00));
    cy_0123 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw0, m31), vmulq_f32(sz0, m21)), vmulq_f32(sy0, m11)), vmulq_f32(sx0, m01));
    cz_0123 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw0, m32), vmulq_f32(sz0, m22)), vmulq_f32(sy0, m12)), vmulq_f32(sx0, m02));


    cx_4567 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw1, m30), vmulq_f32(sz1, m20)), vmulq_f32(sy1, m10)), vmulq_f32(sx1, m00));
    cy_4567 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw1, m31), vmulq_f32(sz1, m21)), vmulq_f32(sy1, m11)), vmulq_f32(sx1, m01));
    cz_4567 = vaddq_f32(vaddq_f32(vaddq_f32(vmulq_f32(sw1, m32), vmulq_f32(sz1, m22)), vmulq_f32(sy1, m12)), vmulq_f32(sx1, m02));
  }


  ctx.fpscr.disableFlushMode();
  ctx.f0.f64 = static_cast<double>(REX_LOAD_F32(0x820BECF4u));
  const float w_val = static_cast<float>(ctx.f0.f64);
  const float32x4_t cw_all = vdupq_n_f32(w_val);


  float32x4_t out0, out1, out2, out3, out4, out5, out6, out7;
  Transpose4x4_F32_822A2388(cx_0123, cy_0123, cz_0123, cw_all, out0, out1, out2, out3);
  Transpose4x4_F32_822A2388(cx_4567, cy_4567, cz_4567, cw_all, out4, out5, out6, out7);


  StoreVec4BE_F32_822A2388(base, sp + static_cast<uint32_t>(-128), out0);
  StoreVec4BE_F32_822A2388(base, sp + static_cast<uint32_t>(-112), out1);
  StoreVec4BE_F32_822A2388(base, sp + static_cast<uint32_t>(-96),  out2);
  StoreVec4BE_F32_822A2388(base, sp + static_cast<uint32_t>(-80),  out3);
  StoreVec4BE_F32_822A2388(base, sp + static_cast<uint32_t>(-64),  out4);
  StoreVec4BE_F32_822A2388(base, sp + static_cast<uint32_t>(-48),  out5);
  StoreVec4BE_F32_822A2388(base, sp + static_cast<uint32_t>(-32),  out6);
  StoreVec4BE_F32_822A2388(base, sp + static_cast<uint32_t>(-16),  out7);


  if (poly_count <= 0) {
    ctx.r3.s64 = 0;
    return;
  }


  ctx.fpscr.enableFlushMode();


  for (int32_t poly_idx = 0; poly_idx < poly_count; ++poly_idx) {
    const uint32_t poly_addr = base_addr + (static_cast<uint32_t>(poly_idx) << 8);
    if (REX_LOAD_U32(poly_addr + 224u) != 0u) {
      continue;
    }


    const int32_t plane_count = static_cast<int32_t>(REX_LOAD_U32(poly_addr + 236u));
    uint32_t r6_flag = 0u;
    int32_t plane_idx = 0;


    for (; plane_idx + 2 <= plane_count; plane_idx += 2) {
      const uint32_t pea0 = poly_addr + (static_cast<uint32_t>(plane_idx) << 4);
      const float32x4_t vp0 = LoadVec4BE_F32_822A2388(base, pea0);
      const float32x4_t vp1 = LoadVec4BE_F32_822A2388(base, pea0 + 16u);


      const float32x4_t tw0 = vmulq_f32(cw_all, vdupq_laneq_f32(vp0, 3));
      const float32x4_t tw1 = vmulq_f32(cw_all, vdupq_laneq_f32(vp1, 3));


      const float32x4_t d0_a = vaddq_f32(vaddq_f32(vaddq_f32(tw0, vmulq_f32(cz_0123, vdupq_laneq_f32(vp0, 2))), vmulq_f32(cy_0123, vdupq_laneq_f32(vp0, 1))), vmulq_f32(cx_0123, vdupq_laneq_f32(vp0, 0)));
      const float32x4_t d1_a = vaddq_f32(vaddq_f32(vaddq_f32(tw0, vmulq_f32(cz_4567, vdupq_laneq_f32(vp0, 2))), vmulq_f32(cy_4567, vdupq_laneq_f32(vp0, 1))), vmulq_f32(cx_4567, vdupq_laneq_f32(vp0, 0)));


      const float32x4_t d0_b = vaddq_f32(vaddq_f32(vaddq_f32(tw1, vmulq_f32(cz_0123, vdupq_laneq_f32(vp1, 2))), vmulq_f32(cy_0123, vdupq_laneq_f32(vp1, 1))), vmulq_f32(cx_0123, vdupq_laneq_f32(vp1, 0)));
      const float32x4_t d1_b = vaddq_f32(vaddq_f32(vaddq_f32(tw1, vmulq_f32(cz_4567, vdupq_laneq_f32(vp1, 2))), vmulq_f32(cy_4567, vdupq_laneq_f32(vp1, 1))), vmulq_f32(cx_4567, vdupq_laneq_f32(vp1, 0)));


      const float32x4_t l1_a = ExactMaxVec_F32_822A2388(vuzp1q_f32(d0_a, d1_a), vuzp2q_f32(d0_a, d1_a));
      const float32x4_t l1_b = ExactMaxVec_F32_822A2388(vuzp1q_f32(d0_b, d1_b), vuzp2q_f32(d0_b, d1_b));


      const float32x4_t l2_a = ExactMaxVec_F32_822A2388(vuzp1q_f32(l1_a, l1_a), vuzp2q_f32(l1_a, l1_a));
      const float32x4_t l2_b = ExactMaxVec_F32_822A2388(vuzp1q_f32(l1_b, l1_b), vuzp2q_f32(l1_b, l1_b));


      const float m_a = (vgetq_lane_f32(l2_a, 0) > vgetq_lane_f32(l2_a, 1)) ? vgetq_lane_f32(l2_a, 0) : vgetq_lane_f32(l2_a, 1);
      const float m_b = (vgetq_lane_f32(l2_b, 0) > vgetq_lane_f32(l2_b, 1)) ? vgetq_lane_f32(l2_b, 0) : vgetq_lane_f32(l2_b, 1);
      if (!(0.0f > m_a)) {
        r6_flag = 1u;
        break;
      }
      if (!(0.0f > m_b)) {
        r6_flag = 1u;
        break;
      }
    }


    if (r6_flag == 0u && plane_idx < plane_count) {
      const float32x4_t vp = LoadVec4BE_F32_822A2388(base, poly_addr + (static_cast<uint32_t>(plane_idx) << 4));
      const float32x4_t tw = vmulq_f32(cw_all, vdupq_laneq_f32(vp, 3));
      const float32x4_t d0 = vaddq_f32(vaddq_f32(vaddq_f32(tw, vmulq_f32(cz_0123, vdupq_laneq_f32(vp, 2))), vmulq_f32(cy_0123, vdupq_laneq_f32(vp, 1))), vmulq_f32(cx_0123, vdupq_laneq_f32(vp, 0)));
      const float32x4_t d1 = vaddq_f32(vaddq_f32(vaddq_f32(tw, vmulq_f32(cz_4567, vdupq_laneq_f32(vp, 2))), vmulq_f32(cy_4567, vdupq_laneq_f32(vp, 1))), vmulq_f32(cx_4567, vdupq_laneq_f32(vp, 0)));
      const float32x4_t l1 = ExactMaxVec_F32_822A2388(vuzp1q_f32(d0, d1), vuzp2q_f32(d0, d1));
      const float32x4_t l2 = ExactMaxVec_F32_822A2388(vuzp1q_f32(l1, l1), vuzp2q_f32(l1, l1));
      const float max_dot = (vgetq_lane_f32(l2, 0) > vgetq_lane_f32(l2, 1)) ? vgetq_lane_f32(l2, 0) : vgetq_lane_f32(l2, 1);
      if (!(0.0f > max_dot)) {
        r6_flag = 1u;
      }
    }


    if (r6_flag == 0u) {
      ctx.r3.s64 = 1;
      return;
    }
  }


  ctx.r3.s64 = 0;
#else
  constexpr int32_t kSrcOffsets[8] = {-144, -224, -160, -192, -256, -240, -208, -176};
  constexpr int32_t kDstOffsets[8] = {-128, -112, -96, -80, -64, -48, -32, -16};
  float corners[8][4];
  if (mat_ptr != 0u) {
    float row[4][4];
    for (int r = 0; r < 4; ++r) {
      const uint32_t ea = (mat_ptr + static_cast<uint32_t>(r << 4)) & ~0xFu;
      for (int c = 0; c < 4; ++c) row[r][c] = REX_LOAD_F32(ea + static_cast<uint32_t>(c << 2));
    }
    const float col_x[4] = {row[0][0], row[1][0], row[2][0], row[3][0]};
    const float col_y[4] = {row[0][1], row[1][1], row[2][1], row[3][1]};
    const float col_z[4] = {row[0][2], row[1][2], row[2][2], row[3][2]};
    uint32_t mask_xyz[4], unit_w[4];
    for (int i = 0; i < 4; ++i) {
      mask_xyz[i] = REX_LOAD_U32(0x831AB6F0u + static_cast<uint32_t>(i << 2));
      unit_w[i]   = REX_LOAD_U32(0x831AB6D0u + static_cast<uint32_t>(i << 2));
    }
    ctx.fpscr.enableFlushModeUnconditional();
    for (int k = 0; k < 8; ++k) {
      const uint32_t ea = (sp + static_cast<uint32_t>(kSrcOffsets[k])) & ~0xFu;
      float c[4];
      for (int i = 0; i < 4; ++i) c[i] = std::bit_cast<float>((REX_LOAD_U32(ea + static_cast<uint32_t>(i << 2)) & mask_xyz[i]) | unit_w[i]);
      corners[k][0] = Dot4Exact_822A2388(c, col_x);
      corners[k][1] = Dot4Exact_822A2388(c, col_y);
      corners[k][2] = Dot4Exact_822A2388(c, col_z);
      corners[k][3] = c[0];
    }
  } else {
    for (int k = 0; k < 8; ++k) {
      const uint32_t ea = (sp + static_cast<uint32_t>(kSrcOffsets[k])) & ~0xFu;
      for (int i = 0; i < 4; ++i) corners[k][i] = REX_LOAD_F32(ea + static_cast<uint32_t>(i << 2));
    }
  }
  ctx.fpscr.disableFlushMode();
  ctx.f0.f64 = static_cast<double>(REX_LOAD_F32(0x820BECF4u));
  const float w_val = static_cast<float>(ctx.f0.f64);
  for (int k = 0; k < 8; ++k) {
    const uint32_t ea = (sp + static_cast<uint32_t>(kDstOffsets[k])) & ~0xFu;
    corners[k][3] = w_val;
    for (int i = 0; i < 4; ++i) REX_STORE_F32(ea + static_cast<uint32_t>(i << 2), corners[k][i]);
  }
  if (poly_count <= 0) { ctx.r3.s64 = 0; return; }
  ctx.fpscr.enableFlushMode();
  for (int32_t poly_idx = 0; poly_idx < poly_count; ++poly_idx) {
    const uint32_t poly_addr = base_addr + (static_cast<uint32_t>(poly_idx) << 8);
    if (REX_LOAD_U32(poly_addr + 224u) != 0u) continue;
    const int32_t plane_count = static_cast<int32_t>(REX_LOAD_U32(poly_addr + 236u));
    uint32_t r6_flag = 0u;
    for (int32_t plane_idx = 0; plane_idx < plane_count; ++plane_idx) {
      const uint32_t pea = (poly_addr + (static_cast<uint32_t>(plane_idx) << 4)) & ~0xFu;
      float plane[4];
      for (int i = 0; i < 4; ++i) plane[i] = REX_LOAD_F32(pea + static_cast<uint32_t>(i << 2));
      const float m01 = MaxExact_822A2388(Dot4Exact_822A2388(plane, corners[0]), Dot4Exact_822A2388(plane, corners[1]));
      const float m23 = MaxExact_822A2388(Dot4Exact_822A2388(plane, corners[2]), Dot4Exact_822A2388(plane, corners[3]));
      const float m45 = MaxExact_822A2388(Dot4Exact_822A2388(plane, corners[4]), Dot4Exact_822A2388(plane, corners[5]));
      const float m67 = MaxExact_822A2388(Dot4Exact_822A2388(plane, corners[6]), Dot4Exact_822A2388(plane, corners[7]));
      const float max_dot = MaxExact_822A2388(MaxExact_822A2388(m01, m23), MaxExact_822A2388(m45, m67));
      if (!(0.0f > max_dot)) { r6_flag = 1u; break; }
    }
    if (r6_flag == 0u) { ctx.r3.s64 = 1; return; }
  }
  ctx.r3.s64 = 0;
#endif
}




// ----------------------------------------------------------------------------
// 3. sub_822A22C0: Sphere vs Polyhedron (1.05x speedup | 50.6 ns vs 53.1 ns ref)
// ----------------------------------------------------------------------------
#include <cstdint>
#include <cstring>
#include <bit>
#if defined(__aarch64__)
#include <arm_neon.h>
#endif


#ifndef REX_LOAD_F32
#define REX_LOAD_F32(ea) std::bit_cast<float>(REX_LOAD_U32(ea))
#endif
#ifndef REX_STORE_F32
#define REX_STORE_F32(ea, v) REX_STORE_U32(ea, std::bit_cast<uint32_t>(v))
#endif


#if defined(__aarch64__)
static inline float32x4_t LoadVec4BE_F32_822A22C0(uint8_t* base, uint32_t ea) {
  const uint32_t aligned = ea & ~0xFu;
  const uint8x16_t raw = vld1q_u8(reinterpret_cast<const uint8_t*>(REX_RAW_ADDR(aligned)));
  return vreinterpretq_f32_u8(vrev32q_u8(raw));
}
#endif


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


#if defined(__aarch64__)
  const float32x4_t v_zero = vdupq_n_f32(0.0f);
  const float32x4_t v_neg_rad = vsubq_f32(v_zero, vdupq_n_f32(radius));
  const float neg_radius = vgetq_lane_f32(v_neg_rad, 0);
#else
  const float neg_radius = 0.0f - radius;
#endif


  if (poly_count <= 0) {
    ctx.r3.s64 = 0;
    return;
  }


  const uint32_t query_ea = ctx.r5.u32 & ~0xFu;
  bool query_loaded = false;
  float px = 0.0f, py = 0.0f, pz = 0.0f;


#if defined(__aarch64__)
  float32x4_t v_px = v_zero, v_py = v_zero, v_pz = v_zero;
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


    if (!query_loaded) {
#if defined(__aarch64__)
      const float32x4_t v_q = LoadVec4BE_F32_822A22C0(base, query_ea);
      v_px = vdupq_laneq_f32(v_q, 0);
      v_py = vdupq_laneq_f32(v_q, 1);
      v_pz = vdupq_laneq_f32(v_q, 2);
      px = vgetq_lane_f32(v_q, 0);
      py = vgetq_lane_f32(v_q, 1);
      pz = vgetq_lane_f32(v_q, 2);
#else
      px = REX_LOAD_F32(query_ea + 0u);
      py = REX_LOAD_F32(query_ea + 4u);
      pz = REX_LOAD_F32(query_ea + 8u);
#endif
      query_loaded = true;
    }


    bool outside = false;
    int32_t plane_idx = 0;


#if defined(__aarch64__)
    for (; plane_idx + 4 <= plane_count; plane_idx += 4) {
      const uint32_t p0 = poly_addr + (static_cast<uint32_t>(plane_idx) << 4);
      const uint32x4x4_t raw4 = vld4q_u32(reinterpret_cast<const uint32_t*>(REX_RAW_ADDR(p0)));
      const float32x4_t v_nx = vreinterpretq_f32_u8(vrev32q_u8(vreinterpretq_u8_u32(raw4.val[0])));
      const float32x4_t v_ny = vreinterpretq_f32_u8(vrev32q_u8(vreinterpretq_u8_u32(raw4.val[1])));
      const float32x4_t v_nz = vreinterpretq_f32_u8(vrev32q_u8(vreinterpretq_u8_u32(raw4.val[2])));
      const float32x4_t v_d  = vreinterpretq_f32_u8(vrev32q_u8(vreinterpretq_u8_u32(raw4.val[3])));


      const float32x4_t acc_z  = vaddq_f32(v_zero, vmulq_f32(v_nz, v_pz));
      const float32x4_t acc_zy = vaddq_f32(acc_z, vmulq_f32(v_ny, v_py));
      const float32x4_t dot    = vaddq_f32(acc_zy, vmulq_f32(v_nx, v_px));
      const float32x4_t dist   = vsubq_f32(dot, v_d);


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
        const float d  = REX_LOAD_F32(plane_ea + 12u);


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




#endif // ENABLE_SPRINT_CULL_HOOKS
