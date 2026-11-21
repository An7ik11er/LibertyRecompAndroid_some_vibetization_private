#include "input/context_touch_overlay.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <vector>
#include <rex/cvar.h>
#include <rex/input/absolute_pointer.h>
#include <rex/input/input.h>
#include <rex/ui/image_decode.h>

#include "gta4_touch_coordinator.h"
#include "input/context_touch_settings.h"
#include "input/gamepad_touch_overlay.h"

REXCVAR_DEFINE_STRING(gta4_touch_skin, "rockstar", "GTA IV/Input/Touch",
                      "Context touch controls look: classic or rockstar (GTA SA/VC mobile style "
                      "procedural vector glyphs; same layout and saved placements)");

namespace gta4::input {

namespace {
ContextTouchSkin CurrentContextTouchSkin() {
  const std::string skin = REXCVAR_GET(gta4_touch_skin);
  return (skin == "rockstar" || skin == "rockstar_mobile" || skin == "mobile")
             ? ContextTouchSkin::kRockstarMobile
             : ContextTouchSkin::kClassic;
}
}  // namespace

ContextTouchOverlay::ContextTouchOverlay(rex::ui::ImGuiDrawer* drawer,
                                         rex::ui::ImmediateDrawer* immediate,
                                         std::filesystem::path icon_directory,
                                         rex::ui::Window* window)
    : ImGuiDialog(drawer), immediate_(immediate), icon_directory_(std::move(icon_directory)) {
  if (window) editor_mouse_ = std::make_unique<ContextTouchEditorMouse>(window);
}

void ContextTouchOverlay::SetIconResolver(ContextTouchIconResolver resolver, void* context) noexcept {
  icon_resolver_ = resolver;
  icon_context_ = context;
}

bool ContextTouchOverlay::ResolveIcon(void* context, std::string_view id, ContextTouchIcon* icon) {
  auto& self = *static_cast<ContextTouchOverlay*>(context);
  if (!icon || !self.immediate_ || self.icon_directory_.empty() || id.empty() ||
      !std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::isalnum(c) || c == '_'; })) return false;
  const std::string key(id);
  auto found = self.icons_.find(key);
  if (found == self.icons_.end()) {
    if (self.icons_.size() >= 256) return false;
    std::unique_ptr<rex::ui::ImmediateTexture> texture;
    std::ifstream file(self.icon_directory_ / (key + ".png"), std::ios::binary | std::ios::ate);
    if (file) {
      const auto length = file.tellg();
      if (length > 0 && length <= 4194304) {
        std::vector<uint8_t> bytes(static_cast<size_t>(length));
        file.seekg(0);
        if (file.read(reinterpret_cast<char*>(bytes.data()), length)) {
          int width = 0, height = 0;
          const auto rgba = rex::ui::DecodeImageRGBA(bytes.data(), bytes.size(), width, height, 1024, 1024);
          if (!rgba.empty() && width > 0 && height > 0 &&
              rgba.size() <= 33554432 - self.decoded_icon_bytes_) {
            texture = self.immediate_->CreateTexture(static_cast<uint32_t>(width),
                static_cast<uint32_t>(height), rex::ui::ImmediateTextureFilter::kLinear, false, rgba.data());
            if (texture) self.decoded_icon_bytes_ += rgba.size();
          }
        }
      }
    }
    found = self.icons_.emplace(key, std::move(texture)).first;
  }
  if (!found->second) return false;
  icon->texture = static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(found->second.get()));
  icon->aspect = float(found->second->width) / float(found->second->height);
  return true;
}

void ContextTouchOverlay::OnDraw(ImGuiIO& io) {
  (void)FlushContextTouchSettings();
  if (GamepadTouchLayoutSelected() && !ContextTouchEditorCapturesInput()) {
    repaint_needed_ = UpdateAndDrawGamepadTouchOverlay(
        ImGui::GetForegroundDrawList(), ImGui::GetFont(), ImGui::GetFontSize(), io.DisplaySize.x,
        io.DisplaySize.y);
    return;
  }
  const auto snapshot = GetContextTouchDrawableOverlaySnapshot();
  repaint_needed_ = snapshot.visible && snapshot.layout.mode != ContextTouchMode::kFrontend &&
                    snapshot.layout.mode != ContextTouchMode::kMap;
  DrawContextTouchOverlay(ImGui::GetForegroundDrawList(), ImGui::GetFont(), ImGui::GetFontSize(),
                          snapshot, io.DisplaySize.x, io.DisplaySize.y,
                          icon_resolver_ ? icon_resolver_ : ResolveIcon,
                          icon_resolver_ ? icon_context_ : this, CurrentContextTouchSkin());
  if (!ContextTouchEditorCapturesInput() && rex::input::TouchPointerInputActive() &&
      !GTA4_TouchTitleInputOwned()) {
    const auto frontend_overlay = GTA4_ReadFrontendTouchOverlayState();
    if (frontend_overlay.active) {
      repaint_needed_ = true;
      DrawContextTouchFrontendOverlay(
          ImGui::GetForegroundDrawList(), ImGui::GetFont(), ImGui::GetFontSize(),
          {.active = frontend_overlay.active,
           .map_active = frontend_overlay.map_active,
           .pressed_mask = frontend_overlay.pressed_mask,
           .rows = frontend_overlay.rows,
           .modern = frontend_overlay.modern,
           .visible_edge_mask = frontend_overlay.visible_edge_mask,
           .tab_band = frontend_overlay.tab_band,
           .tab_count = frontend_overlay.tab_count,
           .pressed_tab = frontend_overlay.pressed_tab},
          io.DisplaySize.x, io.DisplaySize.y);
    }
  }
}

}  // namespace gta4::input
