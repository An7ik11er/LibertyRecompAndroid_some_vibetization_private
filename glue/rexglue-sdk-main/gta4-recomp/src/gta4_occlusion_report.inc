// gta4_occlusion_report.inc — runtime diagnostics & A/B toggle for GTA IV's CPU occluder culling.
//
// sub_822A2AA8(entity r3, viewport r4) returns non-zero when the entity is hidden by the
// occluder volumes active for the viewport. Callers (exact ctx.lr):
//   - 0x821D6A1C (sub_821D6948 world scan)
//   - 0x821D8DEC (sub_821D8CD8 entity scan)
//   - 0x821D8A20 / 0x821D8B5C (sub_821D8848 streaming)
// Leaf volume checks in Batch 9:
//   - sub_822A22C0 (sphere vs convex polyhedra: lr=0x822A2BB8 entity, lr=0x822B2F4C light/corona)
//   - sub_822A2388 (box vs convex polyhedra max-dot: lr=0x822A2CE8)
//   - sub_822A0CC0 (box vs convex polyhedra secondary set2288: lr=0x822A2D24)
//
// Zero overhead when both CVars are false (default).

#define GTA4_OCCLUSION_REPORT_DEFINED 1

REXCVAR_DEFINE_BOOL(gta4_occlusion_report, false, "GTA IV/Diagnostics",
                    "Count live COcclusion calls, occluded hit-rates, corona culling, and Batch 9 "
                    "live parity vs __imp__ (sub_822A2AA8/22C0/2388/0CC0), logging every 5 s");

REXCVAR_DEFINE_BOOL(gta4_disable_batch9_occlusion_hooks, false, "GTA IV/Diagnostics",
                    "Bypass Batch 9 NEON occlusion hooks (sub_822A0CC0/2388/22C0) and run "
                    "original decompiled __imp__ implementations for A/B comparison");

namespace gta4::occlusion_report {

constexpr uint32_t kDefaultViewportPtr = 0x82FEFDE4u;
constexpr uint32_t kOccluderCountArray = 0x82CDBB20u;  // uint32_t[12] up to 0x82CDBB50
constexpr uint32_t kSphereEnableByte = 0x82A996CCu;
constexpr uint32_t kSphereRadiusLimit = 0x82A996D0u;
constexpr uint32_t kPrimarySetEnableByte = 0x82A93185u;

constexpr uint32_t kLrWorldScan = 0x821D6A1Cu;
constexpr uint32_t kLrEntityScan = 0x821D8DECu;
constexpr uint32_t kLrStreamA = 0x821D8A20u;
constexpr uint32_t kLrStreamB = 0x821D8B5Cu;
constexpr uint32_t kLrCoronaSphere = 0x822B2F4Cu;

struct LeafCounters {
  std::atomic<uint64_t> calls{0};
  std::atomic<uint64_t> occluded{0};
  std::atomic<uint64_t> mismatches{0};
};

std::atomic<uint64_t> g_calls{0};
std::atomic<uint64_t> g_occluded{0};
std::atomic<uint64_t> g_exempt{0};
std::atomic<uint64_t> g_world_calls{0};
std::atomic<uint64_t> g_world_occ{0};
std::atomic<uint64_t> g_scan_calls{0};
std::atomic<uint64_t> g_scan_occ{0};
std::atomic<uint64_t> g_stream_calls{0};
std::atomic<uint64_t> g_stream_occ{0};

LeafCounters g_leaf_0cc0{};
LeafCounters g_leaf_2388{};
LeafCounters g_leaf_22c0_ent{};
LeafCounters g_leaf_22c0_corona{};
std::atomic<uint64_t> g_mismatch_logs{0};
std::atomic<int64_t> g_last_report_ns{0};

inline int64_t NowNs() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

inline int32_t ZoneOccluders(uint8_t* base, int32_t index) {
  if (index < 0 || index >= 12) return -1;
  return static_cast<int32_t>(LoadU32(base, kOccluderCountArray + uint32_t(index) * 4u));
}

inline void RecordLeaf(const char* name, uint32_t lr, bool hook_occluded, bool compared,
                       bool orig_occluded) {
  LeafCounters* target = nullptr;
  if (name[8] == '0') {
    target = &g_leaf_0cc0;
  } else if (name[8] == '3') {
    target = &g_leaf_2388;
  } else if (lr == kLrCoronaSphere) {
    target = &g_leaf_22c0_corona;
  } else {
    target = &g_leaf_22c0_ent;
  }
  target->calls.fetch_add(1, std::memory_order_relaxed);
  if (hook_occluded) target->occluded.fetch_add(1, std::memory_order_relaxed);
  if (compared && hook_occluded != orig_occluded) {
    target->mismatches.fetch_add(1, std::memory_order_relaxed);
    if (g_mismatch_logs.fetch_add(1, std::memory_order_relaxed) < 16) {
      const std::string msg = fmt::format("gta4-occlusion: MISMATCH func={} lr={:08X} hook={} orig={}",
                                          name, lr, hook_occluded ? 1 : 0, orig_occluded ? 1 : 0);
      REXLOG_WARN("{}", msg);
#if defined(__ANDROID__)
      __android_log_print(ANDROID_LOG_WARN, "LibertyRecomp", "%s", msg.c_str());
#endif
    }
  }
}

inline double Pct(uint64_t num, uint64_t den) {
  return den ? 100.0 * double(num) / double(den) : 0.0;
}

inline void Report(uint8_t* base, uint32_t viewport, int64_t now, int64_t last) {
  const uint64_t calls = g_calls.exchange(0, std::memory_order_relaxed);
  const uint64_t occluded = g_occluded.exchange(0, std::memory_order_relaxed);
  const uint64_t exempt = g_exempt.exchange(0, std::memory_order_relaxed);
  const uint64_t w_calls = g_world_calls.exchange(0, std::memory_order_relaxed);
  const uint64_t w_occ = g_world_occ.exchange(0, std::memory_order_relaxed);
  const uint64_t s_calls = g_scan_calls.exchange(0, std::memory_order_relaxed);
  const uint64_t s_occ = g_scan_occ.exchange(0, std::memory_order_relaxed);
  const uint64_t st_calls = g_stream_calls.exchange(0, std::memory_order_relaxed);
  const uint64_t st_occ = g_stream_occ.exchange(0, std::memory_order_relaxed);

  const uint64_t c0 = g_leaf_0cc0.calls.exchange(0, std::memory_order_relaxed);
  const uint64_t o0 = g_leaf_0cc0.occluded.exchange(0, std::memory_order_relaxed);
  const uint64_t m0 = g_leaf_0cc0.mismatches.exchange(0, std::memory_order_relaxed);

  const uint64_t c23 = g_leaf_2388.calls.exchange(0, std::memory_order_relaxed);
  const uint64_t o23 = g_leaf_2388.occluded.exchange(0, std::memory_order_relaxed);
  const uint64_t m23 = g_leaf_2388.mismatches.exchange(0, std::memory_order_relaxed);

  const uint64_t ce = g_leaf_22c0_ent.calls.exchange(0, std::memory_order_relaxed);
  const uint64_t oe = g_leaf_22c0_ent.occluded.exchange(0, std::memory_order_relaxed);
  const uint64_t me = g_leaf_22c0_ent.mismatches.exchange(0, std::memory_order_relaxed);

  const uint64_t cc = g_leaf_22c0_corona.calls.exchange(0, std::memory_order_relaxed);
  const uint64_t oc = g_leaf_22c0_corona.occluded.exchange(0, std::memory_order_relaxed);
  const uint64_t mc = g_leaf_22c0_corona.mismatches.exchange(0, std::memory_order_relaxed);

  const double seconds = last ? double(now - last) / 1e9 : 5.0;
  int32_t set_a = -1, set_b = -1;
  uint32_t flags = 0;
  if (viewport) {
    set_a = static_cast<int32_t>(LoadU32(base, viewport + 2284));
    set_b = static_cast<int32_t>(LoadU32(base, viewport + 2288));
    flags = LoadU32(base, viewport + 2272);
  }
  const uint8_t sphere_enable = LoadU8(base, kSphereEnableByte);
  const uint8_t primary_enable = LoadU8(base, kPrimarySetEnableByte);
  const float radius_limit = std::bit_cast<float>(LoadU32(base, kSphereRadiusLimit));
  const bool batch9_disabled = REXCVAR_GET(gta4_disable_batch9_occlusion_hooks);

  const std::string msg = fmt::format(
      "gta4-occlusion: {:.1f}s batch9={}: 2AA8={}/{} ({:.1f}%) exempt={} [world={}/{} scan={}/{} stream={}/{}] "
      "vp={:08X} set2284={}(occ={}) set2288={}(occ={}) fl={:08X} use2288={} sph={} rlim={:.2f} s2284en={} | "
      "0CC0={}/{}({:.1f}%,mis={}) 2388={}/{}({:.1f}%,mis={}) 22C0_ent={}/{}({:.1f}%,mis={}) corona={}/{}({:.1f}%,mis={})",
      seconds, batch9_disabled ? "OFF" : "ON", occluded, calls, Pct(occluded, calls), exempt,
      w_occ, w_calls, s_occ, s_calls, st_occ, st_calls,
      viewport, set_a, ZoneOccluders(base, set_a), set_b, ZoneOccluders(base, set_b), flags,
      (flags & 0x400u) ? 1 : 0, sphere_enable, radius_limit, primary_enable,
      o0, c0, Pct(o0, c0), m0, o23, c23, Pct(o23, c23), m23,
      oe, ce, Pct(oe, ce), me, oc, cc, Pct(oc, cc), mc);
  REXLOG_WARN("{}", msg);
#if defined(__ANDROID__)
  __android_log_print(ANDROID_LOG_WARN, "LibertyRecomp", "%s", msg.c_str());
#endif
}

}  // namespace gta4::occlusion_report

extern "C" void sub_822A2AA8(PPCContext& ctx, uint8_t* base) {
  if (!REXCVAR_GET(gta4_occlusion_report)) [[likely]] {
    __imp__sub_822A2AA8(ctx, base);
    return;
  }
  using namespace gta4::occlusion_report;
  const uint32_t lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t entity = ctx.r3.u32;
  const uint32_t viewport = ctx.r4.u32 ? ctx.r4.u32 : LoadU32(base, kDefaultViewportPtr);
  const bool exempt = entity && (LoadU32(base, entity + 36) & 0x10u) != 0;

  __imp__sub_822A2AA8(ctx, base);

  const bool occ = (ctx.r3.u32 & 0xFFu) != 0;
  g_calls.fetch_add(1, std::memory_order_relaxed);
  if (occ) g_occluded.fetch_add(1, std::memory_order_relaxed);
  if (exempt) g_exempt.fetch_add(1, std::memory_order_relaxed);

  if (lr == kLrWorldScan) {
    g_world_calls.fetch_add(1, std::memory_order_relaxed);
    if (occ) g_world_occ.fetch_add(1, std::memory_order_relaxed);
  } else if (lr == kLrEntityScan) {
    g_scan_calls.fetch_add(1, std::memory_order_relaxed);
    if (occ) g_scan_occ.fetch_add(1, std::memory_order_relaxed);
  } else if (lr == kLrStreamA || lr == kLrStreamB) {
    g_stream_calls.fetch_add(1, std::memory_order_relaxed);
    if (occ) g_stream_occ.fetch_add(1, std::memory_order_relaxed);
  }

  const int64_t now = NowNs();
  int64_t last = g_last_report_ns.load(std::memory_order_relaxed);
  if (now - last >= 5'000'000'000LL &&
      g_last_report_ns.compare_exchange_strong(last, now, std::memory_order_relaxed)) {
    Report(base, viewport, now, last);
  }
}
