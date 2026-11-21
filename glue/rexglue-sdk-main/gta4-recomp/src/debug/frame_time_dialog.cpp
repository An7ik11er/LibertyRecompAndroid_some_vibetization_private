#include "frame_time_dialog.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <rex/input/absolute_pointer.h>
#include <rex/input/input.h>
#include "input/gamepad_touch_overlay.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace {

uint64_t NowNs() {
  return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count());
}

int CurrentTid() { return int(syscall(SYS_gettid)); }

void LogLine(const std::string& line) {
#if defined(__ANDROID__)
  for (size_t offset = 0; offset < line.size(); offset += 900) {
    __android_log_print(ANDROID_LOG_WARN, "LibertyRecomp", "overlay-sensors: %s",
                        line.substr(offset, 900).c_str());
  }
#else
  std::fprintf(stderr, "overlay-sensors: %s\n", line.c_str());
#endif
}

// Small sysfs/procfs reads with open/read/close (no stdio buffering).
// Returns the byte count, or -errno.
int ReadSmall(const char* path, char* buffer, size_t capacity) {
  const int fd = ::open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) return -errno;
  const ssize_t n = ::read(fd, buffer, capacity - 1);
  const int error = errno;
  ::close(fd);
  if (n < 0) return -error;
  buffer[n] = 0;
  return int(n);
}

bool ReadLong(const char* path, long& out) {
  char buffer[64];
  if (ReadSmall(path, buffer, sizeof(buffer)) <= 0) return false;
  char* end = nullptr;
  const long value = std::strtol(buffer, &end, 10);
  if (end == buffer) return false;
  out = value;
  return true;
}

bool ReadTwoLongs(const char* path, long& first, long& second) {
  char buffer[96];
  if (ReadSmall(path, buffer, sizeof(buffer)) <= 0) return false;
  char* end = nullptr;
  first = std::strtol(buffer, &end, 10);
  if (end == buffer) return false;
  char* end2 = nullptr;
  second = std::strtol(end, &end2, 10);
  return end2 != end;
}

std::string ReadString(const char* path) {
  char buffer[128];
  if (ReadSmall(path, buffer, sizeof(buffer)) <= 0) return {};
  std::string value(buffer);
  while (!value.empty() && (value.back() == '\n' || value.back() == ' ')) value.pop_back();
  return value;
}

// Thermal zone units differ between drivers; same rules as the CPU Info app
// (github.com/kamgurgul/cpu-info, TemperatureProvider.android.kt).
float DecodeTemperature(long raw) {
  if (raw <= 0) return NAN;
  if (raw > 1'500'000) return raw > 20'000'000 ? NAN : float(raw) / 100'000.0f;
  if (raw > 15'000) return raw > 200'000 ? NAN : float(raw) / 1000.0f;
  if (raw > 150) return raw > 2000 ? NAN : float(raw) / 10.0f;
  return float(raw);
}

bool StartsWith(const std::string& value, const char* prefix) {
  return value.rfind(prefix, 0) == 0;
}

bool Contains(const std::string& value, const char* part) {
  return value.find(part) != std::string::npos;
}

// Qualcomm exposes battery-current/voltage limiters as thermal zones too.
bool IsNonTemperatureZone(const std::string& type) {
  static const char* kSkip[] = {"socd", "ibat", "vbat", "vph", "bcl", "-lvl", "cdev", "freq",
                                "limit", "isolate", "hotplug"};
  for (const char* part : kSkip) {
    if (Contains(type, part)) return true;
  }
  return false;
}

constexpr const char* kThermalStatusNames[] = {"none",     "light",     "moderate", "severe",
                                               "critical", "emergency", "shutdown"};

struct ThreadRow {
  char name[20] = {};
  int tid = 0;
  float percent = 0.0f;  // of one core
  int core = -1;
};

struct SensorSnapshot {
  bool valid = false;
  int policies = 0;
  std::array<int, 4> policy_first_cpu{};
  std::array<int, 4> policy_last_cpu{};
  std::array<long, 4> cpu_cur_mhz{};
  std::array<long, 4> cpu_cap_mhz{};
  int gpu_busy = -1;
  long gpu_mhz = -1;
  long gpu_cap_mhz = -1;
  float temp_cpu = NAN;
  float temp_gpu = NAN;
  float temp_skin = NAN;
  float temp_battery = NAN;
  int thermal_status = -2;  // -2 = API unavailable, -1 = error
  float process_cores = 0.0f;
  int thread_rows = 0;
  std::array<ThreadRow, 8> rows{};
  bool ui_row_valid = false;
  ThreadRow ui_row{};
  float sample_ms = 0.0f;
};

// Reads sensors and /proc/self/task once per second on its own thread while
// the overlay is visible (pinged within the last 3 s).
class SensorSampler {
 public:
  static SensorSampler& Instance() {
    // Never destroyed: the detached thread may outlive static destruction.
    static SensorSampler* instance = new SensorSampler();
    return *instance;
  }

  void Ping(int ui_tid) {
    last_ping_ns_.store(NowNs(), std::memory_order_relaxed);
    ui_tid_.store(ui_tid, std::memory_order_relaxed);
  }

  SensorSnapshot Get() {
    std::lock_guard lock(mutex_);
    return snapshot_;
  }

 private:
  enum class GpuBusyKind : uint8_t { kNone, kBusyTotal, kPercent };

  struct Policy {
    int first_cpu = 0;
    int last_cpu = 0;
    std::string cur_path;
    std::string cap_path;
  };

  struct Zone {
    std::string type;
    std::string path;
  };

  SensorSampler() {
    std::thread([this] { Run(); }).detach();
  }

  void Run() {
    SetupThisThread();
    Discover();
    uint64_t previous_sample_ns = 0;
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      const uint64_t now = NowNs();
      if (now - last_ping_ns_.load(std::memory_order_relaxed) > 3'000'000'000ull) {
        previous_sample_ns = 0;  // idle: thread deltas restart on resume
        continue;
      }
      const uint64_t begin = NowNs();
      SensorSnapshot next;
      SampleHardware(next);
      SampleThreads(next, previous_sample_ns ? double(now - previous_sample_ns) * 1e-9 : 0.0);
      previous_sample_ns = now;
      next.sample_ms = float(double(NowNs() - begin) * 1e-6);
      next.valid = true;
      std::lock_guard lock(mutex_);
      snapshot_ = next;
    }
  }

  void SetupThisThread() {
    pthread_setname_np(pthread_self(), "OverlaySensors");
    setpriority(PRIO_PROCESS, 0, 10);
    // Keep this thread off the big cores used by the recorder: run on the
    // cores of the first cpufreq policy (the little cluster).
    std::string related =
        ReadString("/sys/devices/system/cpu/cpufreq/policy0/related_cpus");
    cpu_set_t set;
    CPU_ZERO(&set);
    int count = 0;
    for (const char* p = related.c_str(); *p;) {
      char* end = nullptr;
      const long cpu = std::strtol(p, &end, 10);
      if (end == p) break;
      if (cpu >= 0 && cpu < CPU_SETSIZE) {
        CPU_SET(int(cpu), &set);
        ++count;
      }
      p = end;
    }
    if (!count) {
      for (int cpu = 0; cpu < 4; ++cpu) CPU_SET(cpu, &set);
    }
    sched_setaffinity(0, sizeof(set), &set);
  }

  void Discover() {
    std::string report;

    // CPU clusters: one cpufreq policy per cluster.
    if (DIR* dir = opendir("/sys/devices/system/cpu/cpufreq")) {
      std::vector<int> ids;
      while (dirent* entry = readdir(dir)) {
        if (std::strncmp(entry->d_name, "policy", 6) == 0) {
          ids.push_back(std::atoi(entry->d_name + 6));
        }
      }
      closedir(dir);
      std::sort(ids.begin(), ids.end());
      for (int id : ids) {
        if (policies_.size() >= 4) break;
        Policy policy;
        const std::string base = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(id);
        policy.cur_path = base + "/scaling_cur_freq";
        policy.cap_path = base + "/scaling_max_freq";
        policy.first_cpu = id;
        policy.last_cpu = id;
        const std::string related = ReadString((base + "/related_cpus").c_str());
        for (const char* p = related.c_str(); *p;) {
          char* end = nullptr;
          const long cpu = std::strtol(p, &end, 10);
          if (end == p) break;
          policy.last_cpu = std::max(policy.last_cpu, int(cpu));
          p = end;
        }
        policies_.push_back(policy);
      }
    }
    if (policies_.empty()) {
      for (int cpu : {0, 4, 7}) {
        Policy policy;
        const std::string base =
            "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cpufreq";
        policy.cur_path = base + "/scaling_cur_freq";
        policy.cap_path = base + "/scaling_max_freq";
        policy.first_cpu = policy.last_cpu = cpu;
        policies_.push_back(policy);
      }
    }
    report += "cpu-policies=";
    for (const Policy& policy : policies_) {
      long value = 0;
      report += std::to_string(policy.first_cpu) + "-" + std::to_string(policy.last_cpu) +
                (ReadLong(policy.cur_path.c_str(), value) ? "(ok) " : "(denied) ");
    }

    // Adreno. CLO vendor sepolicy lets every app read kgsl-3d0/gpubusy and
    // max_gpuclk through the real device path; gpuclk, devfreq and
    // gpu_busy_percentage are usually denied for untrusted apps. The
    // /sys/class/kgsl symlink itself may also be unreadable, so try the
    // /sys/devices paths first.
    static const char* kKgslDevices[] = {
        "/sys/devices/platform/soc/3d00000.qcom,kgsl-3d0",  // 865/870/888/8 Gen x
        "/sys/devices/platform/soc/2c00000.qcom,kgsl-3d0",  // 855/860
        "/sys/devices/platform/soc/5000000.qcom,kgsl-3d0",  // 845 and some 7xx
        "/sys/devices/platform/soc/5900000.qcom,kgsl-3d0",  // 6xx/4xx
    };
    std::vector<std::string> kgsl_bases;
    std::vector<std::string> devfreq_bases;
    for (const char* device : kKgslDevices) {
      kgsl_bases.push_back(std::string(device) + "/kgsl/kgsl-3d0");
      const char* id = std::strrchr(device, '/') + 1;
      devfreq_bases.push_back(std::string(device) + "/devfreq/" + id);
    }
    kgsl_bases.push_back("/sys/class/kgsl/kgsl-3d0");
    devfreq_bases.push_back("/sys/class/kgsl/kgsl-3d0/devfreq");

    int first_errno = 0;
    auto readable = [&first_errno](const std::string& path) {
      char buffer[64];
      const int result = ReadSmall(path.c_str(), buffer, sizeof(buffer));
      if (result <= 0 && !first_errno) first_errno = -result;
      return result > 0;
    };
    for (const std::string& base : kgsl_bases) {
      if (gpu_busy_kind_ == GpuBusyKind::kNone && readable(base + "/gpubusy")) {
        gpu_busy_path_ = base + "/gpubusy";
        gpu_busy_kind_ = GpuBusyKind::kBusyTotal;
      }
      if (gpu_busy_kind_ == GpuBusyKind::kNone && readable(base + "/gpu_busy_percentage")) {
        gpu_busy_path_ = base + "/gpu_busy_percentage";
        gpu_busy_kind_ = GpuBusyKind::kPercent;
      }
      if (gpu_clock_path_.empty() && readable(base + "/gpuclk")) gpu_clock_path_ = base + "/gpuclk";
      if (gpu_clock_path_.empty() && readable(base + "/clock_mhz")) {
        gpu_clock_path_ = base + "/clock_mhz";
      }
      if (gpu_cap_path_.empty() && readable(base + "/max_gpuclk")) {
        gpu_cap_path_ = base + "/max_gpuclk";
      }
    }
    for (const std::string& base : devfreq_bases) {
      if (gpu_clock_path_.empty() && readable(base + "/cur_freq")) {
        gpu_clock_path_ = base + "/cur_freq";
      }
    }
    if (gpu_busy_kind_ == GpuBusyKind::kNone && readable("/sys/kernel/gpu/gpu_busy")) {
      gpu_busy_path_ = "/sys/kernel/gpu/gpu_busy";
      gpu_busy_kind_ = GpuBusyKind::kPercent;
    }
    if (gpu_clock_path_.empty() && readable("/sys/kernel/gpu/gpu_clock")) {
      gpu_clock_path_ = "/sys/kernel/gpu/gpu_clock";
    }
    if (gpu_cap_path_.empty() && readable("/sys/kernel/gpu/gpu_max_clock")) {
      gpu_cap_path_ = "/sys/kernel/gpu/gpu_max_clock";
    }
    report += "gpu-busy=" + (gpu_busy_path_.empty() ? std::string("none") : gpu_busy_path_) +
              " gpu-clk=" + (gpu_clock_path_.empty() ? std::string("none") : gpu_clock_path_) +
              " gpu-cap=" + (gpu_cap_path_.empty() ? std::string("none") : gpu_cap_path_) +
              " first-denial=" + std::to_string(first_errno) + " ";

    // Thermal zones: read every type once, then keep only the useful ones.
    std::vector<Zone> zones;
    std::string all_types;
    int unreadable = 0;
    auto add_zone = [&](const std::string& directory) {
      const std::string type = ReadString((directory + "/type").c_str());
      if (type.empty()) return;
      all_types += directory.substr(directory.rfind('/') + 1) + "=" + type + " ";
      long raw = 0;
      if (!ReadLong((directory + "/temp").c_str(), raw)) {
        ++unreadable;
        return;
      }
      zones.push_back({type, directory + "/temp"});
    };
    if (DIR* dir = opendir("/sys/class/thermal")) {
      std::vector<std::string> names;
      while (dirent* entry = readdir(dir)) {
        if (std::strncmp(entry->d_name, "thermal_zone", 12) == 0) names.push_back(entry->d_name);
      }
      closedir(dir);
      std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b) {
        return std::atoi(a.c_str() + 12) < std::atoi(b.c_str() + 12);
      });
      for (const std::string& name : names) add_zone("/sys/class/thermal/" + name);
    } else {
      for (int index = 0; index < 120; ++index) {
        add_zone("/sys/class/thermal/thermal_zone" + std::to_string(index));
      }
    }
    bool have_usr_cpu = false;
    for (const Zone& zone : zones) {
      have_usr_cpu |= StartsWith(zone.type, "cpu") && Contains(zone.type, "usr");
    }
    static const char* kSkinTypes[] = {"shell_back", "shell_front", "shell_frame",
                                       "skin-msm-therm", "skin-therm", "quiet-therm",
                                       "sdm-therm", "xo-therm"};
    static const char* kBatteryTypes[] = {"battery", "batt-therm", "batt_therm",
                                          "battery-therm", "bms"};
    int skin_rank = 1000;
    int battery_rank = 1000;
    for (const Zone& zone : zones) {
      if (IsNonTemperatureZone(zone.type)) continue;
      if (StartsWith(zone.type, "cpu") && (!have_usr_cpu || Contains(zone.type, "usr"))) {
        if (cpu_zones_.size() < 2) cpu_zones_.push_back(zone);
        continue;
      }
      if (StartsWith(zone.type, "gpu")) {
        if (gpu_zones_.size() < 1) gpu_zones_.push_back(zone);
        continue;
      }
      for (int rank = 0; rank < int(std::size(kSkinTypes)); ++rank) {
        if (zone.type == kSkinTypes[rank] && rank < skin_rank) {
          skin_rank = rank;
          skin_zone_ = zone;
        }
      }
      for (int rank = 0; rank < int(std::size(kBatteryTypes)); ++rank) {
        if (zone.type == kBatteryTypes[rank] && rank < battery_rank) {
          battery_rank = rank;
          battery_zone_ = zone;
        }
      }
    }
    report += "zones-readable=" + std::to_string(zones.size()) +
              " zones-denied=" + std::to_string(unreadable) +
              " cpu-zones=" + std::to_string(cpu_zones_.size()) +
              " gpu-zones=" + std::to_string(gpu_zones_.size()) +
              " skin=" + (skin_zone_.type.empty() ? std::string("none") : skin_zone_.type) +
              " battery=" + (battery_zone_.type.empty() ? std::string("none") : battery_zone_.type);

#if defined(__ANDROID__)
    // NDK thermal status (API 30+). getThermalHeadroom is left alone: the
    // game already polls it, and a second caller makes both return NaN.
    if (void* android = dlopen("libandroid.so", RTLD_NOW)) {
      using AcquireFn = void* (*)();
      const auto acquire =
          reinterpret_cast<AcquireFn>(dlsym(android, "AThermal_acquireManager"));
      status_ = reinterpret_cast<StatusFn>(dlsym(android, "AThermal_getCurrentThermalStatus"));
      if (acquire && status_) thermal_manager_ = acquire();
    }
#endif
    report += std::string(" thermal-status=") + (thermal_manager_ ? "yes" : "no");
    LogLine(report);
    if (!all_types.empty()) LogLine("zone-types: " + all_types);
  }

  void SampleHardware(SensorSnapshot& out) {
    out.policies = int(policies_.size());
    for (size_t index = 0; index < policies_.size(); ++index) {
      long cur = -1000, cap = -1000;
      ReadLong(policies_[index].cur_path.c_str(), cur);
      ReadLong(policies_[index].cap_path.c_str(), cap);
      out.policy_first_cpu[index] = policies_[index].first_cpu;
      out.policy_last_cpu[index] = policies_[index].last_cpu;
      out.cpu_cur_mhz[index] = cur / 1000;
      out.cpu_cap_mhz[index] = cap / 1000;
    }
    if (gpu_busy_kind_ == GpuBusyKind::kBusyTotal) {
      // busy/total of the last ~1 s window kept by KGSL (not cumulative).
      long busy = 0, total = 0;
      if (ReadTwoLongs(gpu_busy_path_.c_str(), busy, total)) {
        out.gpu_busy = total > 0 ? int(std::clamp(busy * 100 / total, 0L, 100L)) : 0;
      }
    } else if (gpu_busy_kind_ == GpuBusyKind::kPercent) {
      long percent = 0;
      if (ReadLong(gpu_busy_path_.c_str(), percent)) out.gpu_busy = int(std::clamp(percent, 0L, 100L));
    }
    auto to_mhz = [](long value) { return value > 100000 ? value / 1000000 : value; };
    long value = 0;
    if (!gpu_clock_path_.empty() && ReadLong(gpu_clock_path_.c_str(), value)) {
      out.gpu_mhz = to_mhz(value);
    }
    if (!gpu_cap_path_.empty() && ReadLong(gpu_cap_path_.c_str(), value)) {
      out.gpu_cap_mhz = to_mhz(value);
    }
    auto zone_max = [](const std::vector<Zone>& zones) {
      float result = NAN;
      for (const Zone& zone : zones) {
        long raw = 0;
        if (!ReadLong(zone.path.c_str(), raw)) continue;
        const float temperature = DecodeTemperature(raw);
        if (!std::isnan(temperature) && (std::isnan(result) || temperature > result)) {
          result = temperature;
        }
      }
      return result;
    };
    out.temp_cpu = zone_max(cpu_zones_);
    out.temp_gpu = zone_max(gpu_zones_);
    if (!skin_zone_.path.empty() && ReadLong(skin_zone_.path.c_str(), value)) {
      out.temp_skin = DecodeTemperature(value);
    }
    if (!battery_zone_.path.empty() && ReadLong(battery_zone_.path.c_str(), value)) {
      out.temp_battery = DecodeTemperature(value);
    }
    out.thermal_status = thermal_manager_ ? status_(thermal_manager_) : -2;
  }

  void SampleThreads(SensorSnapshot& out, double elapsed_s) {
    static const long kTicksPerSecond = sysconf(_SC_CLK_TCK) > 0 ? sysconf(_SC_CLK_TCK) : 100;
    DIR* dir = opendir("/proc/self/task");
    if (!dir) return;
    std::vector<ThreadRow> rows;
    std::unordered_map<int, uint64_t> ticks_now;
    double total_ticks = 0.0;
    const int ui_tid = ui_tid_.load(std::memory_order_relaxed);
    char path[64];
    char buffer[512];
    while (dirent* entry = readdir(dir)) {
      if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;
      const int tid = std::atoi(entry->d_name);
      std::snprintf(path, sizeof(path), "/proc/self/task/%d/stat", tid);
      if (ReadSmall(path, buffer, sizeof(buffer)) <= 0) continue;
      const char* open = std::strchr(buffer, '(');
      const char* close = std::strrchr(buffer, ')');
      if (!open || !close || close < open) continue;
      ThreadRow row;
      row.tid = tid;
      const size_t name_length =
          std::min(size_t(close - open - 1), sizeof(row.name) - 1);
      std::memcpy(row.name, open + 1, name_length);
      row.name[name_length] = 0;
      // Fields after ") ": state is #3; utime #14, stime #15, processor #39.
      const char* cursor = close + 2;
      uint64_t utime = 0, stime = 0;
      int field = 3;
      while (*cursor && field <= 39) {
        char* end = nullptr;
        if (field == 3) {
          end = const_cast<char*>(cursor + 1);
        } else {
          const unsigned long long number = std::strtoull(cursor, &end, 10);
          if (end == cursor) break;
          if (field == 14) utime = number;
          if (field == 15) stime = number;
          if (field == 39) row.core = int(number);
        }
        cursor = end;
        while (*cursor == ' ') ++cursor;
        ++field;
      }
      const uint64_t ticks = utime + stime;
      ticks_now[tid] = ticks;
      if (elapsed_s > 0.0) {
        const auto previous = previous_ticks_.find(tid);
        if (previous != previous_ticks_.end() && ticks >= previous->second) {
          const double delta = double(ticks - previous->second);
          total_ticks += delta;
          row.percent = float(delta * 100.0 / (double(kTicksPerSecond) * elapsed_s));
        }
      }
      if (tid == ui_tid) {
        out.ui_row = row;
        out.ui_row_valid = elapsed_s > 0.0;
      }
      rows.push_back(row);
    }
    closedir(dir);
    previous_ticks_.swap(ticks_now);
    if (elapsed_s <= 0.0) return;
    out.process_cores = float(total_ticks / (double(kTicksPerSecond) * elapsed_s));
    const size_t keep = std::min(rows.size(), out.rows.size());
    std::partial_sort(rows.begin(), rows.begin() + keep, rows.end(),
                      [](const ThreadRow& a, const ThreadRow& b) { return a.percent > b.percent; });
    for (size_t index = 0; index < keep; ++index) out.rows[index] = rows[index];
    out.thread_rows = int(keep);
  }

  using StatusFn = int (*)(void*);
  StatusFn status_ = nullptr;
  void* thermal_manager_ = nullptr;

  std::vector<Policy> policies_;
  GpuBusyKind gpu_busy_kind_ = GpuBusyKind::kNone;
  std::string gpu_busy_path_;
  std::string gpu_clock_path_;
  std::string gpu_cap_path_;
  std::vector<Zone> cpu_zones_;
  std::vector<Zone> gpu_zones_;
  Zone skin_zone_;
  Zone battery_zone_;
  std::unordered_map<int, uint64_t> previous_ticks_;

  std::atomic<uint64_t> last_ping_ns_{0};
  std::atomic<int> ui_tid_{0};
  std::mutex mutex_;
  SensorSnapshot snapshot_;
};

// Presenter hook: rex_overlay_frame_clock_read (presenter.cpp). Looked up at
// run time so the overlay still works, in fallback mode, without it.
using FrameClockReadFn = uint64_t (*)(uint64_t first, uint64_t* out_ns, uint32_t capacity,
                                      uint32_t* out_written);

FrameClockReadFn LookupFrameClock() {
  return reinterpret_cast<FrameClockReadFn>(dlsym(RTLD_DEFAULT, "rex_overlay_frame_clock_read"));
}

constexpr ImGuiWindowFlags kOverlayFlags =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;

ImVec4 FrameColor(float ms) {
  if (ms <= 17.5f) return ImVec4(0.45f, 0.90f, 0.45f, 1.0f);
  if (ms <= 34.0f) return ImVec4(0.95f, 0.80f, 0.30f, 1.0f);
  return ImVec4(1.00f, 0.40f, 0.35f, 1.0f);
}

}  // namespace

FrameTimeDialog::FrameTimeDialog(rex::ui::ImGuiDrawer* drawer) : rex::ui::ImGuiDialog(drawer) {}

bool FrameTimeDialog::WantsContinuousRepaint() const {
  // Кадры игры и так перерисовывают оверлей. Сами просим перерисовку только
  // когда игра ничего не публикует (загрузка, пауза) больше 0.5 с.
  return mode_ != Mode::kHidden && NowNs() - clock_last_progress_ns_ > 500'000'000ull;
}

void FrameTimeDialog::PushFrame(float ms, uint64_t end_ns, uint64_t index) {
  frame_ms_[ring_pos_] = std::clamp(ms, 0.0f, 1000.0f);
  frame_end_ns_[ring_pos_] = end_ns;
  frame_index_[ring_pos_] = index;
  ring_pos_ = (ring_pos_ + 1) % kRing;
  if (ring_count_ < kRing) ++ring_count_;
}

void FrameTimeDialog::ConsumeFrames(ImGuiIO& io, uint64_t now_ns) {
  static FrameClockReadFn read_clock = nullptr;
  if (!clock_checked_) {
    clock_checked_ = true;
    read_clock = LookupFrameClock();
    clock_available_ = read_clock != nullptr;
    clock_last_progress_ns_ = now_ns;
  }
  if (clock_available_) {
    uint64_t timestamps[256];
    uint32_t written = 0;
    const uint64_t count = read_clock(clock_seen_, timestamps, 256, &written);
    if (count != clock_seen_) {
      if (count - clock_seen_ > written || clock_seen_ == 0) {
        clock_last_ns_ = 0;  // skipped frames: no interval across the gap
      }
      for (uint32_t index = 0; index < written; ++index) {
        if (clock_last_ns_ && timestamps[index] > clock_last_ns_) {
          PushFrame(float(double(timestamps[index] - clock_last_ns_) * 1e-6), timestamps[index],
                    count - written + index);
        }
        clock_last_ns_ = timestamps[index];
      }
      clock_seen_ = count;
      clock_last_progress_ns_ = now_ns;
      using_ui_fallback_ = false;
      return;
    }
    // No publishes for a while: either the hook is not wired or the game is
    // paused. Keep the last data instead of switching sources back and forth.
    if (clock_seen_ || now_ns - clock_last_progress_ns_ < 3'000'000'000ull) return;
  }
  using_ui_fallback_ = true;
  PushFrame(io.DeltaTime * 1000.0f, now_ns, ++fallback_index_);
}

void FrameTimeDialog::RecomputeStats(uint64_t now_ns) {
  Stats stats;
  if (!ring_count_) {
    stats_ = stats;
    return;
  }
  // Chronological copy of the frames that ended inside the stats window.
  std::vector<float> window;
  window.reserve(ring_count_);
  const size_t oldest = (ring_pos_ + kRing - ring_count_) % kRing;
  uint64_t first_end = 0, last_end = 0;
  size_t second_frames = 0;
  uint64_t second_first_end = 0;
  for (size_t offset = 0; offset < ring_count_; ++offset) {
    const size_t index = (oldest + offset) % kRing;
    const uint64_t end = frame_end_ns_[index];
    // A frame may be published after now_ns was taken: compare without
    // unsigned underflow.
    if (end + kStatsWindowNs < now_ns) continue;
    if (!first_end) first_end = end;
    last_end = end;
    window.push_back(frame_ms_[index]);
    if (end + 1'000'000'000ull >= now_ns) {
      if (!second_frames) second_first_end = end;
      ++second_frames;
    }
  }
  if (window.empty()) {
    stats_ = stats;
    return;
  }
  stats.frames = uint32_t(window.size());
  double sum = 0.0;
  for (float ms : window) sum += ms;
  stats.avg_ms = float(sum / double(window.size()));
  if (second_frames >= 2 && last_end > second_first_end) {
    stats.fps = float(double(second_frames - 1) * 1e9 / double(last_end - second_first_end));
  } else {
    stats.fps = stats.avg_ms > 0.01f ? 1000.0f / stats.avg_ms : 0.0f;
  }
  std::vector<float> sorted = window;
  auto percentile = [&sorted](double fraction) {
    const size_t index = std::min(sorted.size() - 1, size_t(double(sorted.size()) * fraction));
    std::nth_element(sorted.begin(), sorted.begin() + index, sorted.end());
    return sorted[index];
  };
  stats.p50_ms = percentile(0.50);
  stats.p99_ms = percentile(0.99);
  stats.p999_ms = percentile(0.999);
  stats.max_ms = *std::max_element(window.begin(), window.end());

  stats.hitch_ms = stats.p50_ms * 1.5f;
  for (float ms : window) stats.hitches += ms > stats.hitch_ms;
  // The period search covers the whole ring; every 2 s is enough.
  if (now_ns - periods_time_ns_ >= 2'000'000'000ull) {
    periods_time_ns_ = now_ns;
    AnalyzePeriods(stats);
  } else {
    stats.period_frames = stats_.period_frames;
    stats.periodic[0] = stats_.periodic[0];
    stats.periodic[1] = stats_.periodic[1];
  }
  stats_ = stats;
}

// "Every Nth frame is slow": average the frame time per phase (frame index mod
// N) for N = 2..64 and keep the phase whose excess over the overall mean is
// least likely to be noise (z score). The strongest component is removed and
// the search repeated, so the shadow even/odd alternation (N = 2) does not
// hide a weaker period such as 30.
void FrameTimeDialog::AnalyzePeriods(Stats& stats) const {
  if (ring_count_ < 240) return;
  // Newest continuous run of publish indices (a pause or skipped frames
  // would shift the phase).
  std::vector<float> values;
  values.reserve(ring_count_);
  const size_t newest = (ring_pos_ + kRing - 1) % kRing;
  uint64_t expected = frame_index_[newest];
  for (size_t back = 0; back < ring_count_; ++back) {
    const size_t index = (newest + kRing - back) % kRing;
    if (frame_index_[index] != expected) break;
    values.push_back(frame_ms_[index]);
    --expected;
  }
  if (values.size() < 240) return;
  std::reverse(values.begin(), values.end());
  stats.period_frames = uint32_t(values.size());
  const float clamp_ms = std::max(stats.p50_ms * 3.0f, 1.0f);
  for (float& value : values) value = std::min(value, clamp_ms);

  std::array<double, 65> sums{};
  std::array<uint32_t, 65> counts{};
  auto search = [&](const std::vector<float>& series, int exclude, Periodic& out) {
    const double n = double(series.size());
    double mean = 0.0;
    for (float value : series) mean += value;
    mean /= n;
    double variance = 0.0;
    for (float value : series) variance += (value - mean) * (value - mean);
    const double sigma = std::sqrt(std::max(variance / n, 1e-6));
    out = {};
    for (int period = 2; period <= 64; ++period) {
      if (period == exclude) continue;
      sums.fill(0.0);
      counts.fill(0);
      int phase = 0;
      for (float value : series) {
        sums[phase] += value;
        ++counts[phase];
        if (++phase == period) phase = 0;
      }
      for (int index = 0; index < period; ++index) {
        if (counts[index] < 4) continue;
        const double excess = sums[index] / counts[index] - mean;
        const double z = excess * std::sqrt(double(counts[index])) / sigma;
        if (z > out.z) {
          out.period = period;
          out.excess_ms = float(excess);
          out.z = float(z);
        }
      }
    }
    // The max over ~2000 phase tests reaches z ~ 4 on pure noise. On the
    // street log the 30-frame hitch gives z 4.7 over 600 frames and ~9 over
    // the full ring.
    if (out.z < 4.5f || out.excess_ms < 1.0f) out = {};
    return mean;
  };

  const double mean = search(values, 0, stats.periodic[0]);
  const int first = stats.periodic[0].period;
  if (!first) return;
  // Remove the first component, then look for another one.
  sums.fill(0.0);
  counts.fill(0);
  int phase = 0;
  for (float value : values) {
    sums[phase] += value;
    ++counts[phase];
    if (++phase == first) phase = 0;
  }
  std::vector<float> residual(values.size());
  phase = 0;
  for (size_t index = 0; index < values.size(); ++index) {
    residual[index] = float(values[index] - (sums[phase] / counts[phase] - mean));
    if (++phase == first) phase = 0;
  }
  search(residual, first, stats.periodic[1]);
}

void FrameTimeDialog::HandleToggle() {
  const bool physical_combo =
      ImGui::IsKeyDown(ImGuiKey_GamepadL3) && ImGui::IsKeyDown(ImGuiKey_GamepadR3);
  bool virtual_combo = gta4::input::GamepadTouchL3R3Pressed();
  if (!virtual_combo) {
    rex::input::X_INPUT_GAMEPAD pad{};
    if (rex::input::ReadTouchGamepad(0, &pad)) {
      constexpr uint16_t kL3R3 = rex::input::X_INPUT_GAMEPAD_LEFT_THUMB |
                                 rex::input::X_INPUT_GAMEPAD_RIGHT_THUMB;
      if ((pad.buttons & kL3R3) == kL3R3) {
        virtual_combo = true;
      }
    }
  }
  const bool combo = physical_combo || virtual_combo;
  if (combo && !combo_down_) {
    mode_ = mode_ == Mode::kFull      ? Mode::kCompact
            : mode_ == Mode::kCompact ? Mode::kHidden
                                      : Mode::kFull;
  }
  combo_down_ = combo;
}

void FrameTimeDialog::DrawHiddenTab() {
  ImGui::SetNextWindowPos(ImVec2(4.0f, 4.0f), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.25f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 2.0f));
  if (ImGui::Begin("##FrameTimeTab", nullptr, kOverlayFlags)) {
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    if (ImGui::SmallButton("fps")) mode_ = Mode::kFull;
    ImGui::PopItemFlag();
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

void FrameTimeDialog::DrawWindow(bool compact) {
  ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.62f);
  if (!ImGui::Begin("##FrameTime", nullptr, kOverlayFlags)) {
    ImGui::End();
    return;
  }
  ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
  const Stats& s = stats_;
  const char* source = using_ui_fallback_ ? " [ui]" : "";
  bool hide_pressed = false;
  if (compact) {
    ImGui::TextColored(FrameColor(s.avg_ms), "%.1f fps", s.fps);
    ImGui::SameLine();
    const Periodic& p = s.periodic[0].period == 2 && s.periodic[1].period ? s.periodic[1]
                                                                           : s.periodic[0];
    if (p.period) {
      ImGui::Text("1%%:%.1fms  /%d +%.1fms%s", s.p99_ms, p.period, p.excess_ms, source);
    } else {
      ImGui::Text("1%%:%.1fms%s", s.p99_ms, source);
    }
    ImGui::SameLine();
    hide_pressed = ImGui::SmallButton("x");
  } else {
    ImGui::TextColored(FrameColor(s.avg_ms), "FPS %.1f  avg %.1f ms", s.fps, s.avg_ms);
    ImGui::SameLine();
    ImGui::TextDisabled("%s  ui %.0f/s  ovl %.2fms", source, paints_per_s_, overlay_ms_);
    ImGui::SameLine();
    hide_pressed = ImGui::SmallButton("x");
    ImGui::Text("med %.1f  1%% %.1f (%.0f fps)  0.1%% %.1f  max %.1f ms", s.p50_ms, s.p99_ms,
                s.p99_ms > 0.01f ? 1000.0f / s.p99_ms : 0.0f, s.p999_ms, s.max_ms);
    ImGui::Text("hitches >%.0fms: %u of %u frames in 10s", s.hitch_ms, s.hitches, s.frames);
    if (s.periodic[0].period) {
      std::string line = "periodic:";
      for (const Periodic& p : s.periodic) {
        if (!p.period) continue;
        char part[64];
        std::snprintf(part, sizeof(part), "  every %d fr +%.1fms (z%.0f)", p.period, p.excess_ms,
                      p.z);
        line += part;
      }
      ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f), "%s", line.c_str());
    } else if (s.period_frames) {
      ImGui::Text("periodic: none in %u frames", s.period_frames);
    }

    const SensorSnapshot hw = SensorSampler::Instance().Get();
    if (hw.valid) {
      ImGui::Separator();
      static uint64_t last_hw_draw_ns = 0;
      static float last_sample_ms = -1.0f;
      static std::string cached_cpu;
      static char cached_gpu[128] = {};
      static char cached_temp[128] = {};
      static char cached_thread_hdr[128] = {};
      struct CachedRow {
        char text[80];
        bool same_line;
      };
      static std::vector<CachedRow> cached_rows;
      static char cached_ui_row[80] = {};
      static bool has_cached_ui_row = false;

      const uint64_t now_draw_ns = NowNs();
      if (hw.sample_ms != last_sample_ms || now_draw_ns - last_hw_draw_ns >= 250'000'000ull) {
        last_sample_ms = hw.sample_ms;
        last_hw_draw_ns = now_draw_ns;

        cached_cpu = "CPU MHz";
        for (int index = 0; index < hw.policies; ++index) {
          char part[48];
          if (hw.cpu_cur_mhz[index] < 0) {
            std::snprintf(part, sizeof(part), "  %d-%d:n/a", hw.policy_first_cpu[index],
                          hw.policy_last_cpu[index]);
          } else {
            std::snprintf(part, sizeof(part), "  %d-%d:%ld/%ld", hw.policy_first_cpu[index],
                          hw.policy_last_cpu[index], hw.cpu_cur_mhz[index], hw.cpu_cap_mhz[index]);
          }
          cached_cpu += part;
        }

        char gpu[96];
        int used = std::snprintf(gpu, sizeof(gpu), "GPU ");
        used += hw.gpu_busy >= 0 ? std::snprintf(gpu + used, sizeof(gpu) - used, "%d%% ", hw.gpu_busy)
                                 : std::snprintf(gpu + used, sizeof(gpu) - used, "load n/a ");
        used += hw.gpu_mhz >= 0 ? std::snprintf(gpu + used, sizeof(gpu) - used, "%ld", hw.gpu_mhz)
                                : std::snprintf(gpu + used, sizeof(gpu) - used, "clk n/a");
        if (hw.gpu_cap_mhz >= 0) {
          std::snprintf(gpu + used, sizeof(gpu) - used, " cap %ld MHz", hw.gpu_cap_mhz);
        }
        const char* status = hw.thermal_status >= 0 && hw.thermal_status <= 6
                                 ? kThermalStatusNames[hw.thermal_status]
                                 : (hw.thermal_status == -2 ? "n/a" : "err");
        std::snprintf(cached_gpu, sizeof(cached_gpu), "%s   thermal: %s", gpu, status);

        auto temp = [](float value, char* out, size_t size) {
          if (std::isnan(value)) {
            std::snprintf(out, size, "--");
          } else {
            std::snprintf(out, size, "%.1f", value);
          }
        };
        char t_cpu[16], t_gpu[16], t_skin[16], t_battery[16];
        temp(hw.temp_cpu, t_cpu, sizeof(t_cpu));
        temp(hw.temp_gpu, t_gpu, sizeof(t_gpu));
        temp(hw.temp_skin, t_skin, sizeof(t_skin));
        temp(hw.temp_battery, t_battery, sizeof(t_battery));
        std::snprintf(cached_temp, sizeof(cached_temp), "Temp C  cpu %s  gpu %s  skin %s  batt %s",
                      t_cpu, t_gpu, t_skin, t_battery);

        cached_rows.clear();
        has_cached_ui_row = false;
        if (hw.thread_rows) {
          std::snprintf(cached_thread_hdr, sizeof(cached_thread_hdr),
                        "Threads, %% of one core @cpu   process %.2f cores, sampler %.1fms",
                        hw.process_cores, hw.sample_ms);
          for (int index = 0; index < hw.thread_rows; ++index) {
            const ThreadRow& row = hw.rows[index];
            CachedRow cr;
            std::snprintf(cr.text, sizeof(cr.text), "%-15s %5d %3.0f%% c%d%s", row.name, row.tid,
                          row.percent, row.core, row.tid == ui_tid_ ? " <ui" : "");
            cr.same_line = ((index & 1) == 0 && index + 1 < hw.thread_rows);
            cached_rows.push_back(cr);
          }
          if (hw.ui_row_valid) {
            bool listed = false;
            for (int index = 0; index < hw.thread_rows; ++index) listed |= hw.rows[index].tid == ui_tid_;
            if (!listed) {
              has_cached_ui_row = true;
              std::snprintf(cached_ui_row, sizeof(cached_ui_row), "%-15s %5d %3.0f%% c%d <ui",
                            hw.ui_row.name, hw.ui_row.tid, hw.ui_row.percent, hw.ui_row.core);
            }
          }
        }
      }

      ImGui::TextUnformatted(cached_cpu.c_str());
      ImGui::TextUnformatted(cached_gpu);
      ImGui::TextUnformatted(cached_temp);

      if (hw.thread_rows) {
        ImGui::Separator();
        ImGui::TextUnformatted(cached_thread_hdr);
        for (size_t index = 0; index < cached_rows.size(); ++index) {
          ImGui::TextUnformatted(cached_rows[index].text);
          if (cached_rows[index].same_line) ImGui::SameLine(0.0f, 18.0f);
        }
        if (has_cached_ui_row) {
          ImGui::TextUnformatted(cached_ui_row);
        }
      }
    }

    // Last kGraph frames, oldest first.
    std::array<float, kGraph> graph{};
    const size_t graph_count = std::min(ring_count_, kGraph);
    for (size_t offset = 0; offset < graph_count; ++offset) {
      graph[offset] = frame_ms_[(ring_pos_ + kRing - graph_count + offset) % kRing];
    }
    ImGui::PlotLines("##frametime", graph.data(), int(graph_count), 0, nullptr, 0.0f, 66.6f,
                     ImVec2(300.0f, 80.0f));
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetItemRectMin();
    const ImVec2 size = ImGui::GetItemRectSize();
    auto y_for = [&](float ms) { return origin.y + size.y * (1.0f - std::min(ms, 66.6f) / 66.6f); };
    draw->AddLine(ImVec2(origin.x, y_for(16.67f)), ImVec2(origin.x + size.x, y_for(16.67f)),
                  IM_COL32(80, 220, 80, 150), 1.0f);
    draw->AddLine(ImVec2(origin.x, y_for(33.33f)), ImVec2(origin.x + size.x, y_for(33.33f)),
                  IM_COL32(220, 180, 40, 170), 1.0f);
  }
  // Tap on the window body (not on a button) switches full/compact.
  const bool body_tap = ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
                        ImGui::IsMouseReleased(ImGuiMouseButton_Left);
  ImGui::PopItemFlag();
  ImGui::End();
  if (hide_pressed) {
    mode_ = Mode::kHidden;
  } else if (body_tap) {
    mode_ = compact ? Mode::kFull : Mode::kCompact;
  }
}

void FrameTimeDialog::OnDraw(ImGuiIO& io) {
  const uint64_t begin_ns = NowNs();
  if (!ui_tid_) ui_tid_ = CurrentTid();

  ConsumeFrames(io, begin_ns);
  HandleToggle();

  ++paints_;
  if (!paints_window_ns_) paints_window_ns_ = begin_ns;
  if (begin_ns - paints_window_ns_ >= 1'000'000'000ull) {
    paints_per_s_ = float(double(paints_) * 1e9 / double(begin_ns - paints_window_ns_));
    paints_ = 0;
    paints_window_ns_ = begin_ns;
  }

  if (mode_ == Mode::kHidden) {
    DrawHiddenTab();
    return;
  }
  if (mode_ == Mode::kFull) SensorSampler::Instance().Ping(ui_tid_);
  if (begin_ns - stats_time_ns_ >= 500'000'000ull) {
    stats_time_ns_ = begin_ns;
    RecomputeStats(begin_ns);
  }
  DrawWindow(mode_ == Mode::kCompact);

  const float spent_ms = float(double(NowNs() - begin_ns) * 1e-6);
  overlay_ms_ = overlay_ms_ * 0.9f + spent_ms * 0.1f;
}