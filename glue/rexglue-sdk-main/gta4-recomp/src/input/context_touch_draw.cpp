#include "input/context_touch_draw.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace gta4::input {
namespace {

void DrawContextTouchLayer(ImDrawList* draw, ImFont* font, float font_size,
                             const ContextTouchLayout& layout, const ContextTouchOverlaySnapshot* input, float fade_alpha,
                             float logical_width, float logical_height,
                             ContextTouchIconResolver resolver, void* icon_context) {
  if (!draw || !font || !std::isfinite(font_size) || font_size <= 0.0f ||
      layout.mode == ContextTouchMode::kFrontend ||
      layout.mode == ContextTouchMode::kMap) return;
  const auto transform = BuildContextTouchOverlayTransform(layout.viewport,
                                                          logical_width, logical_height);
  if (!transform.valid) return;
  const float radius_scale = std::min(transform.scale_x, transform.scale_y);
  const float opacity = std::clamp(layout.opacity, 0.2f, 1.0f) *
                        std::clamp(fade_alpha, 0.0f, 1.0f);
  if (!std::isfinite(opacity) || opacity <= 0.0f) return;
  const auto color = [opacity](int r, int g, int b, float alpha) {
    return IM_COL32(r, g, b, static_cast<int>(std::clamp(alpha * opacity, 0.0f, 255.0f)));
  };
  const auto point = [&](float x, float y) {
    return ImVec2(transform.offset_x + x * transform.scale_x,
                  transform.offset_y + y * transform.scale_y);
  };
  for (size_t i = 0; i < layout.control_count; ++i) {
    const auto& control = layout.controls[i];
    if (!control.visible || control.native_hud || control.kind == ContextTouchControlKind::kLookSurface) continue;
    const ImVec2 center = control.kind == ContextTouchControlKind::kMovementStick && (input && input->movement_owned)
        ? point((input ? input->movement_origin_x : 0.0f), (input ? input->movement_origin_y : 0.0f))
        : point(control.center_x, control.center_y);
    const float radius = control.radius * radius_scale;
    if (!std::isfinite(radius) || radius <= 0.0f) continue;
    const bool active = (input ? input->active[i] : 0) != 0;
    if (control.kind == ContextTouchControlKind::kActivitySurface) {
      const auto lo = point(control.minimum_x, control.minimum_y);
      const auto hi = point(control.maximum_x, control.maximum_y);
      const float rounding = radius * 0.18f;
      draw->AddRectFilled(lo, hi, color(10, 10, 10, active ? 140.0f : 85.0f), rounding);
      const float pad_x = std::min({2.5f, lo.x, std::max(0.0f, logical_width - hi.x)});
      const float pad_y = std::min({2.5f, lo.y, std::max(0.0f, logical_height - hi.y)});
      if (pad_x > 0.5f && pad_y > 0.5f) {
        draw->AddRect(ImVec2(lo.x - pad_x, lo.y - pad_y), ImVec2(hi.x + pad_x, hi.y + pad_y),
                      active ? color(255, 195, 80, 130.0f) : color(135, 215, 255, 70.0f),
                      rounding + 2.0f, 0, 2.0f);
      }
      draw->AddRect(lo, hi,
                    active ? color(255, 220, 125, 245.0f) : color(170, 225, 255, 175.0f),
                    rounding, 0, active ? 2.4f : 1.6f);
      const float label_size = std::min(font_size, radius * 0.22f);
      const auto size = font->CalcTextSizeA(label_size, FLT_MAX, 0.0f, control.label.data());
      draw->AddText(font, label_size, ImVec2(center.x - size.x * 0.5f, lo.y + rounding),
                    color(255, 255, 255, 245.0f), control.label.data());
      using G = TouchActivityGesture;
      const auto gesture = control.activity_gesture;
      const bool circular = gesture == G::kCircleRight;
      const bool horizontal = gesture == G::kAlternatingHorizontal || gesture == G::kHorizontalLeft;
      const bool vertical = gesture == G::kStrokeRight || gesture == G::kAlternatingVertical;
      const float length = radius * 0.55f, tip = radius * 0.15f;
      if (circular) {
        draw->AddCircle(center, length, color(255,255,255,160.0f), 32, 2.0f);
        draw->AddTriangleFilled(ImVec2(center.x + length, center.y + tip),
            ImVec2(center.x + length - tip, center.y - tip),
            ImVec2(center.x + length + tip, center.y - tip), color(255,255,255,200.0f));
      } else if (horizontal || vertical) {
        const ImVec2 a(center.x - (horizontal ? length : 0.0f), center.y - (vertical ? length : 0.0f));
        const ImVec2 b(center.x + (horizontal ? length : 0.0f), center.y + (vertical ? length : 0.0f));
        draw->AddLine(a, b, color(255,255,255,150.0f), 2.0f);
        if (horizontal) {
          draw->AddTriangleFilled(a, ImVec2(a.x+tip,a.y-tip), ImVec2(a.x+tip,a.y+tip), color(255,255,255,190.0f));
          draw->AddTriangleFilled(b, ImVec2(b.x-tip,b.y-tip), ImVec2(b.x-tip,b.y+tip), color(255,255,255,190.0f));
        } else {
          draw->AddTriangleFilled(a, ImVec2(a.x-tip,a.y+tip), ImVec2(a.x+tip,a.y+tip), color(255,255,255,190.0f));
          draw->AddTriangleFilled(b, ImVec2(b.x-tip,b.y-tip), ImVec2(b.x+tip,b.y-tip), color(255,255,255,190.0f));
        }
      } else {
        draw->AddCircle(center, length, color(255,255,255,120.0f), 24, 1.0f);
        draw->AddCircleFilled(center, tip, color(255,255,255,180.0f), 16);
      }
      continue;
    }
    const float max_halo = std::max(
        0.0f, std::min({center.x, center.y, logical_width - center.x, logical_height - center.y}) -
                  radius);
    const float halo_delta = std::min(std::max(1.5f, radius * 0.08f), max_halo);
    if (halo_delta >= 0.75f) {
      draw->AddCircle(center, radius + halo_delta,
                      active ? color(255, 190, 75, 150.0f) : color(120, 205, 255, 70.0f), 48,
                      std::max(1.5f, radius * 0.045f));
    }
    draw->AddCircleFilled(center, radius, active ? color(236, 135, 42, 255.0f)
                                               : color(12, 18, 27, 210.0f), 48);
    draw->AddCircle(center, radius,
                    active ? color(255, 230, 140, 255.0f) : color(185, 230, 255, 235.0f), 48,
                    std::max(1.3f, radius * (active ? 0.045f : 0.032f)));
    if (control.kind == ContextTouchControlKind::kMovementStick || control.kind == ContextTouchControlKind::kRightStick) {
      const bool right = control.kind == ContextTouchControlKind::kRightStick;
      const float x = std::clamp(right ? (input ? input->right_x : 0.0f) : (input ? input->movement_x : 0.0f), -255.0f, 255.0f) / 255.0f;
      const float y = std::clamp(right ? (input ? input->right_y : 0.0f) : (input ? input->movement_y : 0.0f), -255.0f, 255.0f) / 255.0f;
      draw->AddCircleFilled(ImVec2(center.x + x * radius * 0.55f,
                                  center.y + y * radius * 0.55f),
                            radius * 0.34f, color(255, 255, 255, 210.0f), 32);
    }
    ContextTouchIcon icon;
    bool has_icon = false;
    if (resolver && control.icon_id[0]) {
      has_icon = resolver(icon_context, control.icon_id.data(), &icon) && icon.texture &&
                 std::isfinite(icon.aspect) && icon.aspect > 0.0f;
    }
    if (has_icon) {
      const float bound = radius * 0.62f;
      const float half_width = icon.aspect >= 1.0f ? bound : bound * icon.aspect;
      const float half_height = icon.aspect >= 1.0f ? bound / icon.aspect : bound;
      draw->AddImage(icon.texture, ImVec2(center.x - half_width, center.y - half_height),
                     ImVec2(center.x + half_width, center.y + half_height), icon.uv_min,
                     icon.uv_max, color(255, 255, 255, 255.0f));
    } else if (control.label[0]) {
      const ImVec2 natural = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, control.label.data());
      const float fitted_size = natural.x > 0.0f
          ? std::min(font_size, font_size * radius * 1.55f / natural.x) : font_size;
      const ImVec2 size = font->CalcTextSizeA(fitted_size, FLT_MAX, 0.0f, control.label.data());
      draw->AddText(font, fitted_size, ImVec2(center.x - size.x * 0.5f, center.y - size.y * 0.5f),
                     color(255, 255, 255, 255.0f), control.label.data());
    }
  }
}

}  // namespace

void DrawContextTouchOverlay(ImDrawList* draw, ImFont* font, float font_size,
                             const ContextTouchOverlaySnapshot& snapshot,
                             float logical_width, float logical_height,
                             ContextTouchIconResolver resolver, void* icon_context) {
  if (!snapshot.visible) return;
  if (snapshot.outgoing_layout && snapshot.outgoing_alpha > 0.0f)
    DrawContextTouchLayer(draw, font, font_size, *snapshot.outgoing_layout, nullptr,
        snapshot.fade_alpha * snapshot.outgoing_alpha, logical_width, logical_height, resolver, icon_context);
  DrawContextTouchLayer(draw, font, font_size, snapshot.layout, &snapshot,
      snapshot.fade_alpha * snapshot.layout_alpha, logical_width, logical_height, resolver, icon_context);
}

void DrawContextTouchFrontendOverlay(ImDrawList* draw, ImFont* font, float font_size,
                                     const ContextTouchFrontendOverlayView& view,
                                     float logical_width, float logical_height) {
  if (!draw || !font || !view.active || !std::isfinite(font_size) || font_size <= 0.0f ||
      !std::isfinite(logical_width) || !std::isfinite(logical_height) ||
      logical_width <= 0.0f || logical_height <= 0.0f) {
    return;
  }

  // Subtle hitbox outlines around selectable Pause Menu rows so tap targets are clear.
  if (!view.map_active) {
    for (const auto& row : view.rows) {
      if (!row.selectable || !row.bounds.valid()) continue;
      const float x0 = std::clamp(row.bounds.left, 0.0f, 1.0f) * logical_width;
      const float y0 = std::clamp(row.bounds.top, 0.0f, 1.0f) * logical_height;
      const float x1 = std::clamp(row.bounds.right, 0.0f, 1.0f) * logical_width;
      const float y1 = std::clamp(row.bounds.bottom, 0.0f, 1.0f) * logical_height;
      if (x1 - x0 < 4.0f || y1 - y0 < 4.0f) continue;
      const ImVec2 lo(x0, y0);
      const ImVec2 hi(x1, y1);
      draw->AddRect(lo, hi, IM_COL32(120, 210, 255, 65), 4.0f, 0, 1.0f);
      const float tick = std::min({10.0f, (x1 - x0) * 0.12f, (y1 - y0) * 0.35f});
      const ImU32 corner = IM_COL32(160, 228, 255, 140);
      draw->AddLine(lo, ImVec2(x0 + tick, y0), corner, 1.5f);
      draw->AddLine(lo, ImVec2(x0, y0 + tick), corner, 1.5f);
      draw->AddLine(ImVec2(x1 - tick, y1), hi, corner, 1.5f);
      draw->AddLine(ImVec2(x1, y1 - tick), hi, corner, 1.5f);
    }
  }

  // Edge action hitboxes (< TAB, TAB >, UP, < -, + >, DN, ZOOM +/-, BACK, RESUME).
  for (const auto& zone : gta4::touch::kFrontendEdgeZones) {
    if (!zone.MatchesMode(view.map_active)) continue;
    const ImVec2 lo(zone.bounds.left * logical_width, zone.bounds.top * logical_height);
    const ImVec2 hi(zone.bounds.right * logical_width, zone.bounds.bottom * logical_height);
    const float w = hi.x - lo.x;
    const float h = hi.y - lo.y;
    if (w <= 2.0f || h <= 2.0f) continue;
    const bool pressed = (view.pressed_mask & zone.bit) != 0u;
    const float rounding = std::clamp(std::min(w, h) * 0.16f, 6.0f, 14.0f);

    draw->AddRect(ImVec2(std::max(0.0f, lo.x - 2.0f), std::max(0.0f, lo.y - 2.0f)),
                  ImVec2(std::min(logical_width, hi.x + 2.0f),
                         std::min(logical_height, hi.y + 2.0f)),
                  pressed ? IM_COL32(255, 195, 80, 165) : IM_COL32(110, 205, 255, 65),
                  rounding + 2.0f, 0, 2.0f);
    draw->AddRectFilled(lo, hi,
                        pressed ? IM_COL32(236, 135, 42, 205) : IM_COL32(10, 18, 28, 128),
                        rounding);
    draw->AddRect(lo, hi,
                  pressed ? IM_COL32(255, 232, 145, 255) : IM_COL32(145, 222, 255, 205),
                  rounding, 0, pressed ? 2.6f : 1.7f);

    const ImVec2 natural = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, zone.label);
    const float fitted = natural.x > 0.0f
        ? std::min({font_size * 0.95f, font_size * (w * 0.84f) / natural.x, h * 0.42f})
        : font_size;
    const ImVec2 text_size = font->CalcTextSizeA(fitted, FLT_MAX, 0.0f, zone.label);
    const ImVec2 text_pos(lo.x + (w - text_size.x) * 0.5f, lo.y + (h - text_size.y) * 0.5f);
    draw->AddText(font, fitted, text_pos,
                  pressed ? IM_COL32(255, 255, 255, 255) : IM_COL32(235, 246, 255, 240),
                  zone.label);
  }

  // Compact bottom gesture hint strip.
  const char* hint = view.map_active
      ? "Map: 1-finger drag = Pan  |  Tap = Waypoint  |  Pinch / ZOOM = Scale  |  Edges = Tabs / Back"
      : "Menu: Tap row = Select  |  Swipe empty = Scroll  |  Edges = Tabs / Scroll / Adjust / Back";
  const float hint_font = std::clamp(logical_height * 0.022f, 11.0f, font_size * 0.85f);
  const ImVec2 hint_nat = font->CalcTextSizeA(hint_font, FLT_MAX, 0.0f, hint);
  const float max_hint_w = logical_width * 0.66f;
  const float fitted_hint = hint_nat.x > max_hint_w && hint_nat.x > 0.0f
      ? hint_font * (max_hint_w / hint_nat.x)
      : hint_font;
  const ImVec2 hint_size = font->CalcTextSizeA(fitted_hint, FLT_MAX, 0.0f, hint);
  const float pad_x = 12.0f;
  const float pad_y = 4.0f;
  const float cx = logical_width * 0.5f;
  const float by = logical_height * 0.985f;
  const ImVec2 pill_lo(cx - hint_size.x * 0.5f - pad_x, by - hint_size.y - pad_y * 2.0f);
  const ImVec2 pill_hi(cx + hint_size.x * 0.5f + pad_x, by);
  draw->AddRectFilled(pill_lo, pill_hi, IM_COL32(10, 16, 24, 150), 8.0f);
  draw->AddRect(pill_lo, pill_hi, IM_COL32(125, 210, 255, 130), 8.0f, 0, 1.2f);
  draw->AddText(font, fitted_hint, ImVec2(cx - hint_size.x * 0.5f, pill_lo.y + pad_y),
                IM_COL32(220, 238, 255, 225), hint);
}
}  // namespace gta4::input
