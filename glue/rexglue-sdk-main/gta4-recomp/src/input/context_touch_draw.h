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
  // 71: modern pause-menu touch. Edge zones outside visible_edge_mask are neither drawn nor
  // hit; the tab strip is drawn as a subtle underline per tab header cell.
  bool modern = false;
  uint32_t visible_edge_mask = gta4::touch::kAllFrontendEdgeBits;
  gta4::touch::Rect tab_band{};
  uint32_t tab_count = 0;
  int32_t pressed_tab = -1;
};

using ContextTouchIconResolver = bool (*)(void* context, std::string_view id,
                                        ContextTouchIcon* icon);

// 71: visual skin of the context touch controls (gta4_touch_skin). kRockstarMobile draws
// translucent dark discs with thin white rings and procedural white/amber vector glyphs per
// TouchAction (GTA SA / VC mobile look, no PNG icon pack); layout and placement files are
// untouched, actions without a glyph keep their icon or label.
enum class ContextTouchSkin : uint8_t { kClassic, kRockstarMobile };

// Procedural glyph for |action| centred at |center| (radius |radius|); false if the action
// has none. Exposed for tests and for other overlays.
bool DrawRockstarTouchGlyph(ImDrawList* draw, TouchAction action, ImVec2 center, float radius,
                            ImU32 ink, ImU32 accent);

// No global ImGui state, frame ownership, guest memory, or input access.
void DrawContextTouchOverlay(ImDrawList* draw, ImFont* font, float font_size,
                             const ContextTouchOverlaySnapshot& snapshot,
                             float logical_width, float logical_height,
                             ContextTouchIconResolver resolver = nullptr,
                             void* icon_context = nullptr,
                             ContextTouchSkin skin = ContextTouchSkin::kClassic);

void DrawContextTouchFrontendOverlay(ImDrawList* draw, ImFont* font, float font_size,
                                     const ContextTouchFrontendOverlayView& view,
                                     float logical_width, float logical_height);

}  // namespace gta4::input
