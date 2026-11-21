#include "input/context_touch_draw.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace gta4::input {
namespace {

void DrawContextTouchLayer(ImDrawList* draw, ImFont* font, float font_size,
                             const ContextTouchLayout& layout, const ContextTouchOverlaySnapshot* input, float fade_alpha,
                             float logical_width, float logical_height,
                             ContextTouchIconResolver resolver, void* icon_context,
                             ContextTouchSkin skin) {
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
    if (skin == ContextTouchSkin::kRockstarMobile) {
      // Rockstar mobile: dark glass disc, thin white rim, white glyph; pressed = amber.
      draw->AddCircleFilled(center, radius,
                            active ? color(236, 150, 40, 150.0f) : color(0, 0, 0, 105.0f), 48);
      draw->AddCircle(center, radius,
                      active ? color(255, 214, 120, 255.0f) : color(255, 255, 255, 150.0f), 48,
                      std::max(1.2f, radius * (active ? 0.06f : 0.035f)));
      const bool stick = control.kind == ContextTouchControlKind::kMovementStick ||
                         control.kind == ContextTouchControlKind::kRightStick;
      if (stick) {
        const bool right = control.kind == ContextTouchControlKind::kRightStick;
        const float x = std::clamp(right ? (input ? input->right_x : 0.0f) : (input ? input->movement_x : 0.0f), -255.0f, 255.0f) / 255.0f;
        const float y = std::clamp(right ? (input ? input->right_y : 0.0f) : (input ? input->movement_y : 0.0f), -255.0f, 255.0f) / 255.0f;
        draw->AddCircle(center, radius * 0.62f, color(255, 255, 255, 60.0f), 40, 1.2f);
        const ImVec2 thumb(center.x + x * radius * 0.55f, center.y + y * radius * 0.55f);
        draw->AddCircleFilled(thumb, radius * 0.30f, color(255, 255, 255, active ? 235.0f : 190.0f), 32);
        draw->AddCircle(thumb, radius * 0.30f, color(0, 0, 0, 90.0f), 32, 1.0f);
        continue;
      }
      const ImU32 ink = active ? color(255, 255, 255, 255.0f) : color(255, 255, 255, 235.0f);
      const ImU32 accent = color(255, 190, 70, 245.0f);
      if (DrawRockstarTouchGlyph(draw, control.action, center, radius, ink, accent)) continue;
      // No glyph for this action: icon or label below, on the mobile disc.
    }
    const float max_halo = skin == ContextTouchSkin::kRockstarMobile ? 0.0f : std::max(
        0.0f, std::min({center.x, center.y, logical_width - center.x, logical_height - center.y}) -
                  radius);
    const float halo_delta = std::min(std::max(1.5f, radius * 0.08f), max_halo);
    if (halo_delta >= 0.75f) {
      draw->AddCircle(center, radius + halo_delta,
                      active ? color(255, 190, 75, 150.0f) : color(120, 205, 255, 70.0f), 48,
                      std::max(1.5f, radius * 0.045f));
    }
    if (skin != ContextTouchSkin::kRockstarMobile) {
    draw->AddCircleFilled(center, radius, active ? color(236, 135, 42, 255.0f)
                                               : color(12, 18, 27, 210.0f), 48);
    draw->AddCircle(center, radius,
                    active ? color(255, 230, 140, 255.0f) : color(185, 230, 255, 235.0f), 48,
                    std::max(1.3f, radius * (active ? 0.045f : 0.032f)));
    }
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
                             ContextTouchIconResolver resolver, void* icon_context,
                             ContextTouchSkin skin) {
  if (!snapshot.visible) return;
  if (snapshot.outgoing_layout && snapshot.outgoing_alpha > 0.0f)
    DrawContextTouchLayer(draw, font, font_size, *snapshot.outgoing_layout, nullptr,
        snapshot.fade_alpha * snapshot.outgoing_alpha, logical_width, logical_height, resolver, icon_context, skin);
  DrawContextTouchLayer(draw, font, font_size, snapshot.layout, &snapshot,
      snapshot.fade_alpha * snapshot.layout_alpha, logical_width, logical_height, resolver, icon_context, skin);
}

// 71: procedural Rockstar-mobile glyphs. Everything is in units of the button radius r
// around c, stroked with |ink|; |accent| marks the "action" part (muzzle flash, arrow tip).
bool DrawRockstarTouchGlyph(ImDrawList* draw, TouchAction action, ImVec2 c, float r,
                            ImU32 ink, ImU32 accent) {
  if (!draw || !std::isfinite(r) || r <= 1.0f) return false;
  const float w = std::max(1.5f, r * 0.085f);  // stroke
  const auto P = [c, r](float x, float y) { return ImVec2(c.x + x * r, c.y + y * r); };
  const auto line = [&](float x0, float y0, float x1, float y1, ImU32 col) {
    draw->AddLine(P(x0, y0), P(x1, y1), col, w);
  };
  const auto chevron = [&](float x, float y, float s, bool up, ImU32 col) {
    const float d = up ? -s : s;
    draw->AddPolyline(std::array<ImVec2, 3>{P(x - s, y - d * 0.5f), P(x, y + d * 0.5f),
                                            P(x + s, y - d * 0.5f)}.data(),
                      3, col, 0, w);
  };
  const auto chevron_h = [&](float x, float y, float s, bool right, ImU32 col) {
    const float d = right ? s : -s;
    draw->AddPolyline(std::array<ImVec2, 3>{P(x - d * 0.5f, y - s), P(x + d * 0.5f, y),
                                            P(x - d * 0.5f, y + s)}.data(),
                      3, col, 0, w);
  };
  const auto triangle = [&](float x, float y, float s, float dir_x, float dir_y, ImU32 col) {
    // filled triangle pointing along (dir_x, dir_y)
    const float px = -dir_y, py = dir_x;
    draw->AddTriangleFilled(P(x + dir_x * s, y + dir_y * s),
                            P(x - dir_x * s * 0.6f + px * s * 0.8f, y - dir_y * s * 0.6f + py * s * 0.8f),
                            P(x - dir_x * s * 0.6f - px * s * 0.8f, y - dir_y * s * 0.6f - py * s * 0.8f),
                            col);
  };
  const auto arc = [&](float x, float y, float rr, float a0, float a1, ImU32 col) {
    draw->PathClear();
    draw->PathArcTo(P(x, y), rr * r, a0, a1, 24);
    draw->PathStroke(col, 0, w);
  };
  constexpr float kPi = 3.14159265f;
  using A = TouchAction;
  switch (action) {
    case A::kFire:
    case A::kVehicleFire: {  // bullet with a muzzle burst
      draw->AddRectFilled(P(-0.34f, -0.09f), P(0.12f, 0.09f), ink, r * 0.03f);
      draw->AddTriangleFilled(P(0.12f, -0.09f), P(0.30f, 0.0f), P(0.12f, 0.09f), ink);
      for (int i = -1; i <= 1; ++i) line(0.36f, i * 0.12f, 0.52f, i * 0.22f, accent);
      return true;
    }
    case A::kVehicleAltFire: {  // rocket
      draw->AddRectFilled(P(-0.30f, -0.08f), P(0.16f, 0.08f), ink, r * 0.03f);
      draw->AddTriangleFilled(P(0.16f, -0.08f), P(0.36f, 0.0f), P(0.16f, 0.08f), ink);
      draw->AddTriangleFilled(P(-0.30f, -0.08f), P(-0.42f, -0.22f), P(-0.20f, -0.08f), ink);
      draw->AddTriangleFilled(P(-0.30f, 0.08f), P(-0.42f, 0.22f), P(-0.20f, 0.08f), ink);
      line(-0.48f, 0.0f, -0.62f, 0.0f, accent);
      return true;
    }
    case A::kAim:
    case A::kFreeAim: {  // crosshair
      draw->AddCircle(c, r * 0.36f, ink, 32, w);
      line(0.0f, -0.56f, 0.0f, -0.20f, ink); line(0.0f, 0.20f, 0.0f, 0.56f, ink);
      line(-0.56f, 0.0f, -0.20f, 0.0f, ink); line(0.20f, 0.0f, 0.56f, 0.0f, ink);
      draw->AddCircleFilled(c, r * 0.06f, action == A::kAim ? accent : ink, 12);
      return true;
    }
    case A::kRunSprint: {  // double chevron forward
      chevron_h(-0.14f, 0.0f, 0.26f, true, ink);
      chevron_h(0.14f, 0.0f, 0.26f, true, accent);
      return true;
    }
    case A::kJumpClimb: {  // arrow up over a ledge
      line(0.0f, 0.22f, 0.0f, -0.30f, ink);
      chevron(0.0f, -0.24f, 0.22f, true, ink);
      line(-0.40f, 0.38f, 0.40f, 0.38f, accent);
      return true;
    }
    case A::kCrouch: {  // arrow down onto the floor
      line(0.0f, -0.36f, 0.0f, 0.14f, ink);
      chevron(0.0f, 0.10f, 0.22f, false, ink);
      line(-0.40f, 0.36f, 0.40f, 0.36f, accent);
      return true;
    }
    case A::kCover: {  // wall with a figure tucked behind it
      draw->AddRect(P(0.06f, -0.42f), P(0.30f, 0.42f), ink, 0.0f, 0, w);
      draw->AddCircleFilled(P(-0.18f, -0.22f), r * 0.10f, ink, 16);
      line(-0.18f, -0.10f, -0.18f, 0.20f, ink);
      line(-0.18f, 0.20f, -0.30f, 0.40f, ink); line(-0.18f, 0.20f, -0.04f, 0.40f, ink);
      return true;
    }
    case A::kReload: {  // circular arrow
      arc(0.0f, 0.0f, 0.34f, -kPi * 0.35f, kPi * 1.35f, ink);
      triangle(0.34f * std::cos(-kPi * 0.35f), 0.34f * std::sin(-kPi * 0.35f), 0.14f,
               std::cos(-kPi * 0.35f - kPi * 0.5f) * -1.0f, std::sin(-kPi * 0.35f - kPi * 0.5f) * -1.0f,
               accent);
      return true;
    }
    case A::kContext: {  // door with an arrow into it (enter / exit / interact)
      draw->AddRect(P(0.02f, -0.40f), P(0.36f, 0.40f), ink, 0.0f, 0, w);
      draw->AddCircleFilled(P(0.26f, 0.02f), r * 0.04f, ink, 10);
      line(-0.44f, 0.0f, -0.08f, 0.0f, accent);
      chevron_h(-0.12f, 0.0f, 0.14f, true, accent);
      return true;
    }
    case A::kWeaponNext:
    case A::kWeaponPrevious: {
      const bool next = action == A::kWeaponNext;
      draw->AddRect(P(-0.26f, -0.16f), P(0.26f, 0.16f), ink, r * 0.05f, 0, w);
      chevron_h(next ? 0.44f : -0.44f, 0.0f, 0.14f, next, accent);
      return true;
    }
    case A::kWeaponWheel:
    case A::kWeaponSelect: {  // segmented wheel
      for (int i = 0; i < 6; ++i) {
        const float a0 = kPi * 2.0f * float(i) / 6.0f + 0.12f;
        arc(0.0f, 0.0f, 0.38f, a0, a0 + kPi * 2.0f / 6.0f - 0.24f, i == 0 ? accent : ink);
      }
      draw->AddCircleFilled(c, r * 0.08f, ink, 12);
      return true;
    }
    case A::kPhone: {  // handset
      draw->AddRect(P(-0.18f, -0.40f), P(0.18f, 0.40f), ink, r * 0.07f, 0, w);
      line(-0.08f, -0.30f, 0.08f, -0.30f, ink);
      draw->AddRectFilled(P(-0.11f, -0.20f), P(0.11f, 0.20f), accent, r * 0.02f);
      return true;
    }
    case A::kAccelerate: {  // pedal arrow up
      triangle(0.0f, -0.10f, 0.34f, 0.0f, -1.0f, ink);
      line(-0.30f, 0.34f, 0.30f, 0.34f, accent);
      return true;
    }
    case A::kBrake: {  // pedal arrow down
      triangle(0.0f, 0.10f, 0.34f, 0.0f, 1.0f, ink);
      line(-0.30f, -0.34f, 0.30f, -0.34f, accent);
      return true;
    }
    case A::kHandbrake: {  // (P) with brackets
      draw->AddCircle(c, r * 0.32f, ink, 32, w);
      arc(0.0f, 0.0f, 0.50f, kPi * 0.70f, kPi * 1.30f, accent);
      arc(0.0f, 0.0f, 0.50f, -kPi * 0.30f, kPi * 0.30f, accent);
      line(-0.08f, 0.18f, -0.08f, -0.18f, ink);
      arc(-0.02f, -0.08f, 0.10f, -kPi * 0.5f, kPi * 0.5f, ink);
      return true;
    }
    case A::kHorn: {  // speaker with waves
      draw->AddRectFilled(P(-0.40f, -0.10f), P(-0.24f, 0.10f), ink);
      draw->AddTriangleFilled(P(-0.24f, -0.10f), P(-0.04f, -0.28f), P(-0.04f, 0.28f), ink);
      draw->AddTriangleFilled(P(-0.24f, -0.10f), P(-0.04f, 0.28f), P(-0.24f, 0.10f), ink);
      arc(-0.04f, 0.0f, 0.22f, -kPi * 0.25f, kPi * 0.25f, accent);
      arc(-0.04f, 0.0f, 0.38f, -kPi * 0.25f, kPi * 0.25f, accent);
      return true;
    }
    case A::kRadioNext:
    case A::kRadioPrevious: {  // radio dial + skip arrow
      draw->AddCircle(P(0.0f, 0.06f), r * 0.24f, ink, 24, w);
      line(-0.16f, -0.24f, 0.18f, -0.42f, ink);
      const bool next = action == A::kRadioNext;
      triangle(next ? 0.44f : -0.44f, 0.06f, 0.14f, next ? 1.0f : -1.0f, 0.0f, accent);
      return true;
    }
    case A::kCameraCycle: {  // camera body
      draw->AddRect(P(-0.36f, -0.20f), P(0.36f, 0.24f), ink, r * 0.05f, 0, w);
      draw->AddRectFilled(P(-0.14f, -0.30f), P(0.10f, -0.20f), ink);
      draw->AddCircle(P(0.0f, 0.02f), r * 0.12f, accent, 20, w);
      return true;
    }
    case A::kLookBehind: {  // U-turn arrow
      arc(0.0f, 0.04f, 0.24f, kPi, kPi * 2.0f, ink);
      line(-0.24f, 0.04f, -0.24f, 0.36f, ink);
      line(0.24f, 0.04f, 0.24f, 0.30f, ink);
      triangle(0.24f, 0.36f, 0.14f, 0.0f, 1.0f, accent);
      return true;
    }
    case A::kHeliAscend:
    case A::kHeliDescend: {
      const bool up = action == A::kHeliAscend;
      chevron(0.0f, up ? -0.12f : 0.12f, 0.26f, up, accent);
      chevron(0.0f, up ? 0.12f : -0.12f, 0.26f, up, ink);
      return true;
    }
    case A::kHeliYawLeft:
    case A::kHeliYawRight: {
      const bool right = action == A::kHeliYawRight;
      arc(0.0f, 0.10f, 0.32f, kPi * 1.15f, kPi * 1.85f, ink);
      const float a = right ? kPi * 1.85f : kPi * 1.15f;
      triangle(0.32f * std::cos(a), 0.10f + 0.32f * std::sin(a), 0.13f, right ? 1.0f : -1.0f,
               0.35f, accent);
      return true;
    }
    case A::kDeploy: {  // parachute canopy
      arc(0.0f, 0.0f, 0.40f, kPi, kPi * 2.0f, ink);
      line(-0.40f, 0.0f, 0.40f, 0.0f, ink);
      line(-0.36f, 0.0f, 0.0f, 0.38f, ink); line(0.36f, 0.0f, 0.0f, 0.38f, ink);
      draw->AddCircleFilled(P(0.0f, 0.40f), r * 0.07f, accent, 12);
      return true;
    }
    case A::kParachuteBrakeLeft:
    case A::kParachuteBrakeRight: {
      const bool right = action == A::kParachuteBrakeRight;
      arc(0.0f, 0.10f, 0.34f, kPi, kPi * 2.0f, ink);
      line(right ? 0.34f : -0.34f, 0.10f, right ? 0.34f : -0.34f, 0.42f, accent);
      return true;
    }
    case A::kDetach: {  // broken link
      draw->AddRect(P(-0.44f, -0.12f), P(-0.06f, 0.12f), ink, r * 0.10f, 0, w);
      draw->AddRect(P(0.06f, -0.12f), P(0.44f, 0.12f), ink, r * 0.10f, 0, w);
      line(-0.04f, -0.30f, 0.04f, -0.18f, accent); line(-0.04f, 0.30f, 0.04f, 0.18f, accent);
      return true;
    }
    case A::kHeadlights: {  // headlight beam
      arc(-0.10f, 0.0f, 0.24f, kPi * 0.5f, kPi * 1.5f, ink);
      line(-0.10f, -0.24f, -0.10f, 0.24f, ink);
      for (int i = -1; i <= 1; ++i) line(0.04f, i * 0.16f, 0.40f, i * 0.16f + 0.06f, accent);
      return true;
    }
    default:
      return false;
  }
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

  // 71: tab header strip. Barely-there underline per tab cell, lit while pressed, so the
  // retail headers stay the visual and the touch target is still discoverable.
  if (view.modern && view.tab_count && view.tab_band.valid()) {
    const gta4::touch::PauseTabStrip strip{view.tab_band, view.tab_count};
    for (uint32_t i = 0; i < view.tab_count; ++i) {
      const gta4::touch::Rect cell = strip.TabBounds(i);
      const float x0 = cell.left * logical_width + 3.0f;
      const float x1 = cell.right * logical_width - 3.0f;
      const float y1 = cell.bottom * logical_height;
      if (x1 - x0 < 6.0f) continue;
      const bool pressed = view.pressed_tab == int32_t(i);
      if (pressed) {
        draw->AddRectFilled(ImVec2(x0, cell.top * logical_height), ImVec2(x1, y1),
                            IM_COL32(236, 135, 42, 70), 6.0f);
      }
      draw->AddLine(ImVec2(x0, y1), ImVec2(x1, y1),
                    pressed ? IM_COL32(255, 205, 110, 230) : IM_COL32(150, 220, 255, 70),
                    pressed ? 3.0f : 1.5f);
    }
  }

  // Edge action hitboxes (< TAB, TAB >, UP, < -, + >, DN, ZOOM +/-, BACK, RESUME); the
  // modern layout keeps only BACK / RESUME and the map's ZOOM.
  for (const auto& zone : gta4::touch::kFrontendEdgeZones) {
    if (!zone.MatchesMode(view.map_active) || !(zone.bit & view.visible_edge_mask)) continue;
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
  const char* hint = view.modern
      ? (view.map_active
             ? "Map: Drag = Pan  |  Tap = Waypoint  |  Pinch / ZOOM = Scale  |  Tap a tab = Switch"
             : "Menu: Tap = Select  |  Drag / flick = Scroll  |  Drag row sideways = Adjust  |  Tap a tab = Switch")
      : (view.map_active
             ? "Map: 1-finger drag = Pan  |  Tap = Waypoint  |  Pinch / ZOOM = Scale  |  Edges = Tabs / Back"
             : "Menu: Tap row = Select  |  Swipe empty = Scroll  |  Edges = Tabs / Scroll / Adjust / Back");
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
