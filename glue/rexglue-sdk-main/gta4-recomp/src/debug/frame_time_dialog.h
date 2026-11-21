#pragma once

#include <rex/ui/imgui_dialog.h>

#include <cstddef>
#include <cstdint>

// FPS / frame-time overlay.
//
// Frame times come from the presenter's publish clock (one timestamp per
// published guest frame, see rex_overlay_frame_clock_read in presenter.cpp).
// Without that hook the overlay falls back to the UI paint interval and marks
// itself "[ui]": UI paints also happen on input, so that fallback is noisy.
//
// Sensors and per-thread CPU load are read by a background thread once per
// second, never on the UI thread.
//
// Controls: tap the window body = full/compact, "x" = hide to a small "fps"
// tab (tap it to bring the overlay back). Gamepad: L3+R3 cycles
// full -> compact -> hidden. The window takes no gamepad navigation, so the
// cross/A button no longer presses its buttons.
class FrameTimeDialog : public rex::ui::ImGuiDialog {
 public:
  explicit FrameTimeDialog(rex::ui::ImGuiDrawer* drawer);
  bool WantsContinuousRepaint() const override;

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  enum class Mode : uint8_t { kFull, kCompact, kHidden };

  struct Periodic {
    int period = 0;          // every Nth frame is slower...
    float excess_ms = 0.0f;  // ...by this much on average
    float z = 0.0f;          // significance of that excess
  };

  struct Stats {
    float fps = 0.0f;
    float avg_ms = 0.0f;
    float p50_ms = 0.0f;
    float p99_ms = 0.0f;
    float p999_ms = 0.0f;
    float max_ms = 0.0f;
    uint32_t frames = 0;   // frames in the 10 s stats window
    uint32_t hitches = 0;  // frames slower than hitch_ms (1.5x median)
    float hitch_ms = 0.0f;
    uint32_t period_frames = 0;  // continuous frames used for the period search
    Periodic periodic[2];        // strongest, and strongest after removing it
  };

  void ConsumeFrames(ImGuiIO& io, uint64_t now_ns);
  void PushFrame(float ms, uint64_t end_ns, uint64_t index);
  void RecomputeStats(uint64_t now_ns);
  void AnalyzePeriods(Stats& stats) const;
  void HandleToggle();
  void DrawHiddenTab();
  void DrawWindow(bool compact);

  static constexpr size_t kRing = 2048;  // ~35-70 s of frames
  static constexpr size_t kGraph = 240;
  static constexpr uint64_t kStatsWindowNs = 10'000'000'000ull;

  float frame_ms_[kRing]{};
  uint64_t frame_end_ns_[kRing]{};
  uint64_t frame_index_[kRing]{};  // publish index; gaps break the period search
  uint64_t fallback_index_ = 0;
  size_t ring_pos_ = 0;
  size_t ring_count_ = 0;

  // Presenter publish clock.
  bool clock_checked_ = false;
  bool clock_available_ = false;
  uint64_t clock_seen_ = 0;
  uint64_t clock_last_ns_ = 0;
  uint64_t clock_last_progress_ns_ = 0;
  bool using_ui_fallback_ = true;

  Stats stats_{};
  uint64_t stats_time_ns_ = 0;
  uint64_t periods_time_ns_ = 0;

  Mode mode_ = Mode::kCompact;
  bool combo_down_ = false;

  uint32_t paints_ = 0;
  uint64_t paints_window_ns_ = 0;
  float paints_per_s_ = 0.0f;
  float overlay_ms_ = 0.0f;
  int ui_tid_ = 0;
};