#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <imgui.h>

#include "gta4_touch_policy.h"
#include "input/context_touch_controls.h"

namespace gta4::input {

struct ContextTouchIcon {
  ImTextureID texture{};
  ImVec2 uv_min{0.0f, 0.0f};
  ImVec2 uv_max{1.0f, 1.0f};
  float aspect = 1.0f;
};

struct ContextTouchFrontendOverlayView {
  bool active = false;
  bool map_active = false;
  uint32_t pressed_mask = 0;
  std::span<const gta4::touch::FrontendRow> rows;
};

using ContextTouchIconResolver = bool (*)(void* context, std::string_view id,
                                        ContextTouchIcon* icon);

// No global ImGui state, frame ownership, guest memory, or input access.
void DrawContextTouchOverlay(ImDrawList* draw, ImFont* font, float font_size,
                             const ContextTouchOverlaySnapshot& snapshot,
                             float logical_width, float logical_height,
                             ContextTouchIconResolver resolver = nullptr,
                             void* icon_context = nullptr);

void DrawContextTouchFrontendOverlay(ImDrawList* draw, ImFont* font, float font_size,
                                     const ContextTouchFrontendOverlayView& view,
                                     float logical_width, float logical_height);

}  // namespace gta4::input
