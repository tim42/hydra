//
// created by : Timothée Feuillet
// date: Mon May 24 2021 17:27:01 GMT+0200 (CEST)
//
//
// Copyright (c) 2021 Timothée Feuillet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef __N_23433187981268130206_308313449_IMGUI_CONTEXT_HPP__
#define __N_23433187981268130206_308313449_IMGUI_CONTEXT_HPP__

#include <optional>
#include <vulkan/vulkan.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <hydra/utilities/memory_allocator.hpp>
#include <hydra/engine/hydra_context.hpp>
#include <hydra/assets/raw.hpp>

#include <hydra/glfw/glfw_events.hpp>

namespace neam
{
  namespace hydra
  {
    namespace imgui
    {
      // indices:
      static constexpr uint32_t regular = 0;
      static constexpr uint32_t bold = 1;
      static constexpr uint32_t italic = 2;
      static constexpr uint32_t bold_italic = bold | italic;
      static constexpr uint32_t _mode_count = 4;

      // font types:
      static constexpr uint32_t default_font = 0 * _mode_count;
      static constexpr uint32_t monospace_font = 1 * _mode_count;
      static constexpr uint32_t _font_count = 2 * _mode_count;

      static ImFont* get_font(uint32_t idx)
      {
        // first, try to fallback to the default_font, keeping the same mode
        if (idx >= (uint32_t)ImGui::GetIO().Fonts->Fonts.size())
          idx &= (_mode_count - 1);
        // secondly, fallback to the default_font without keeping the mode (it means don't have a font familly loaded)
        if (idx >= (uint32_t)ImGui::GetIO().Fonts->Fonts.size())
          idx = 0;
        return ImGui::GetIO().Fonts->Fonts[idx];
      }

      // NOTE: We cannot use the imgui vulkan implementation as the resource management is somewhat incorrect
      // (buffers are destroyed when they are still in use)
      // So we made our own using hydra facilities
      class imgui_context : public glfw::events::raw_keyboard_listener, public glfw::events::raw_mouse_listener
      {
        public:
          imgui_context(core_context& _ctx, glfw::window& win,  glfw::events::manager& _emgr)
          : ctx(_ctx),
            window(win),
            emgr(_emgr),
            context(*ImGui::CreateContext()),
            io(ImGui::GetIO())
          {
            IMGUI_CHECKVERSION();
            switch_to();

            //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

            io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines;

            content_scale = win.get_content_scale();
            const float scale = (std::max(content_scale.x, content_scale.y));

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            hydra_dark_theme();

            // Scale everything:
            ImGui::GetStyle().ScaleAllSizes(scale);
            ImGui::GetStyle().AntiAliasedLinesUseTex = false;

            // FIXME: a custom implem of this one, as hydra can handle multiple windows. For now it's OK, but in the future we'll need to fix this.
            ImGui_ImplGlfw_InitForVulkan(window._get_glfw_handle(), false);
            emgr.register_keyboard_listener(this);
            emgr.register_mouse_listener(this);

            // Setup capabilities:
            io.BackendRendererName = "hydra/vk";
            // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
            io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
          }

          virtual ~imgui_context()
          {
            emgr.unregister_keyboard_listener(this);
            emgr.unregister_mouse_listener(this);

            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext(&context);
          }

          void load_default_fonts()
          {
//             ctx.res.read_resource<assets::raw_asset>("fonts/Cousine-Regular.ttf:raw"_rid)
//             ctx.res.read_resource<assets::raw_asset>("fonts/Hack/Hack-Regular.ttf:raw"_rid)
            _load_font("fonts/NotoSans/NotoSans-Regular.ttf:raw"_rid, default_font|regular);
            _load_font("fonts/NotoSans/NotoSans-Bold.ttf:raw"_rid, default_font|bold);
            _load_font("fonts/NotoSans/NotoSans-Italic.ttf:raw"_rid, default_font|italic);
            _load_font("fonts/NotoSans/NotoSans-BoldItalic.ttf:raw"_rid, default_font|bold_italic);

            _load_font("fonts/Hack/Hack-Regular.ttf:raw"_rid, monospace_font|regular);
            _load_font("fonts/Hack/Hack-Bold.ttf:raw"_rid, monospace_font|bold);
            _load_font("fonts/Hack/Hack-Italic.ttf:raw"_rid, monospace_font|italic);
            _load_font("fonts/Hack/Hack-BoldItalic.ttf:raw"_rid, monospace_font|bold_italic);
          }

          void switch_to()
          {
            ImGui::SetCurrentContext(&context);
          }

          bool is_current_context() const
          {
            return ImGui::GetCurrentContext() == &context;
          }

          void new_frame()
          {
            const glm::vec2 new_content_scale = window.get_content_scale();
            if (new_content_scale != content_scale || has_font_change)
            {
              const float old_scale = (std::max(content_scale.x, content_scale.y));
              content_scale = new_content_scale;
              const float scale = (std::max(content_scale.x, content_scale.y));

              const float scale_fct = scale / old_scale;

              // Scale the style:
              ImGui::GetStyle().ScaleAllSizes(scale_fct);

              // scale the fonts:
              {
                io.Fonts->Clear();
                std::lock_guard _l(font_lock);
                has_font_change = false;
                io.FontGlobalScale = 1;
                for (uint32_t i = 0; i < _font_count; i += _mode_count)
                {
                  const float font_base_size = (i == default_font ? 18 : 13);
                  if (ttf_fonts[i].size > 0)
                  {
                    for (uint32_t m = 0; m < _mode_count; ++m)
                    {
                      uint32_t font_idx = (ttf_fonts[i + m].size > 0 ? i + m : i);
                      ImFontConfig cfg;
                      cfg.FontDataOwnedByAtlas = false;
                      io.Fonts->AddFontFromMemoryTTF(ttf_fonts[font_idx].data.get(), ttf_fonts[font_idx].size, std::roundf(font_base_size * scale), &cfg);
                    }
                  }
                  else
                  {
                    for (uint32_t m = 0; m < _mode_count; ++m)
                    {
                      ImFontConfig cfg;
                      cfg.SizePixels = std::roundf(font_base_size * scale);
                      io.Fonts->AddFontDefault(&cfg);
                    }
                  }
                }
              }

              // rebuild the fonts:
              io.Fonts->Build();
              regenerate_fonts = true;
            }

            switch_to();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
          }

          ImGuiIO& get_io() { return io; }
          const ImGuiIO& get_io() const { return io; }

          bool should_regenerate_fonts() const { return regenerate_fonts; }
          void set_font_as_regenerated() { regenerate_fonts = false; }

          void _load_font(string_id rid, uint32_t font)
          {
            ctx.res.read_resource<assets::raw_asset>(rid)
            .then([this, idx = font, rid](assets::raw_asset&& font, resources::status st)
            {
              if (st == resources::status::success)
              {
                cr::out().debug("loaded font: {} (index: {})", rid, idx);
              }
              else
              {
                cr::out().warn("failed to load font: {} (index: {})", rid, idx);
                return;
              }
              std::lock_guard _l(font_lock);
              ttf_fonts[idx] = (std::move(font.data));
              has_font_change = true;
            });
          }

        private: // events
          void on_mouse_button(int button, int action, int modifiers) final override
          {
            ImGui_ImplGlfw_MouseButtonCallback(window._get_glfw_handle(), button, action, modifiers);
          }
          void on_mouse_wheel(double x, double y) final override
          {
            ImGui_ImplGlfw_ScrollCallback(window._get_glfw_handle(), x, y);
          }
          void on_mouse_move(double /*x*/, double /*y*/) final override {}

          void on_key(int key, int scancode, int action, int modifiers) final override
          {
            ImGui_ImplGlfw_KeyCallback(window._get_glfw_handle(), key, scancode, action, modifiers);
          }
          void on_unicode_input(unsigned int code) final override
          {
            ImGui_ImplGlfw_CharCallback(window._get_glfw_handle(), code);
          }

        private: // style:
          static void hydra_dark_theme()
          {
            ImVec4* colors = ImGui::GetStyle().Colors;
            colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_PopupBg]                = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
            colors[ImGuiCol_Border]                 = ImVec4(0.69f, 0.69f, 0.69f, 0.29f);
            colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
            colors[ImGuiCol_FrameBg]                = ImVec4(0.01f, 0.01f, 0.01f, 0.60f);
            colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.08f, 0.08f, 0.08f, 0.60f);
            colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.22f, 0.23f, 0.70f);
            colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
            colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
            colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
            colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
            colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
            colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
            colors[ImGuiCol_SliderGrab]             = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
            colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
            colors[ImGuiCol_Button]                 = ImVec4(0.00f, 0.51f, 0.95f, 0.54f);
            colors[ImGuiCol_ButtonHovered]          = ImVec4(0.20f, 0.60f, 1.00f, 0.54f);
            colors[ImGuiCol_ButtonActive]           = ImVec4(0.03f, 0.30f, 0.75f, 0.54f);
            colors[ImGuiCol_Header]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
            colors[ImGuiCol_HeaderHovered]          = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
            colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
            colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
            colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
            colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
            colors[ImGuiCol_ResizeGrip]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
            colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
            colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
            colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
            colors[ImGuiCol_TabHovered]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
            colors[ImGuiCol_TabUnfocused]           = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
            colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_DockingPreview]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
            colors[ImGuiCol_DockingEmptyBg]         = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            colors[ImGuiCol_PlotLines]              = ImVec4(0.00f, 0.50f, 0.90f, 1.00f);
            colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            colors[ImGuiCol_PlotHistogram]          = ImVec4(0.00f, 0.50f, 0.90f, 1.00f);
            colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
            colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
            colors[ImGuiCol_TableBorderLight]       = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
            colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
            colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
            colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
            colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
            colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
            colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowPadding                     = ImVec2(8.00f, 8.00f);
            style.FramePadding                      = ImVec2(5.00f, 2.00f);
            style.CellPadding                       = ImVec2(6.00f, 6.00f);
            style.ItemSpacing                       = ImVec2(4.00f, 4.00f);
            style.ItemInnerSpacing                  = ImVec2(6.00f, 6.00f);
            style.TouchExtraPadding                 = ImVec2(0.00f, 0.00f);
            style.IndentSpacing                     = 25;
            style.ScrollbarSize                     = 15;
            style.GrabMinSize                       = 10;
            style.WindowBorderSize                  = 1;
            style.ChildBorderSize                   = 1;
            style.PopupBorderSize                   = 1;
            style.FrameBorderSize                   = 1;
            style.TabBorderSize                     = 1;
            style.WindowRounding                    = 7;
            style.ChildRounding                     = 4;
            style.FrameRounding                     = 3;
            style.PopupRounding                     = 4;
            style.ScrollbarRounding                 = 9;
            style.GrabRounding                      = 3;
            style.LogSliderDeadzone                 = 4;
            style.TabRounding                       = 4;
          }

        private:
          core_context& ctx;
          glfw::window& window;
          glfw::events::manager& emgr;

          ImGuiContext& context;
          ImGuiIO& io;

          glm::vec2 content_scale;
          bool regenerate_fonts = true;

          spinlock font_lock;
          raw_data ttf_fonts[_font_count];
          bool has_font_change = true;
      };
    } // namespace imgui
  } // namespace hydra
} // namespace neam

#endif // __N_23433187981268130206_308313449_IMGUI_CONTEXT_HPP__


