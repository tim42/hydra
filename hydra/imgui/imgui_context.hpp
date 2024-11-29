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

#pragma once


#include <optional>
#include <vulkan/vulkan.h>
#ifdef IMGUI_ENABLE_FREETYPE
  #include <imgui/misc/freetype/imgui_freetype.h>
#endif

#include <imgui.h>
#include <implot.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <hydra/utilities/memory_allocator.hpp>
#include <hydra/utilities/holders.hpp>
#include <hydra/engine/hydra_context.hpp>
#include <hydra/engine/conf/conf.hpp>
#include <hydra/assets/raw.hpp>

#include <hydra/glfw/glfw_engine_module.hpp>
#include <hydra/glfw/glfw_events.hpp>

#include "imgui_drawdata.hpp"
#include "ui_elements.hpp"

namespace neam::hydra::imgui
{
  struct imgui_configuration : hydra::conf::hconf<imgui_configuration, "imgui.hcnf", conf::location_t::index_program_local_dir>
  {
    std::string data;

    // TODO: Add presets
  };
}
N_METADATA_STRUCT(neam::hydra::imgui::imgui_configuration)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(data)
  >;
};

namespace neam::hydra::imgui
{
  // NOTE: We cannot use the imgui vulkan implementation as the resource management is somewhat incorrect
  // (buffers are destroyed when they are still in use)
  // So we made our own using hydra facilities
  class imgui_context : public events::raw_keyboard_listener, public events::raw_mouse_listener
  {
    private:
      // FIXME: move that elsewhere
      struct win_event_listener : public events::raw_keyboard_listener, public events::raw_mouse_listener, public events::window_listener
      {
        static ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int key)
        {
          switch (key)
          {
            case GLFW_KEY_TAB: return ImGuiKey_Tab;
            case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
            case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
            case GLFW_KEY_UP: return ImGuiKey_UpArrow;
            case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
            case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
            case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
            case GLFW_KEY_HOME: return ImGuiKey_Home;
            case GLFW_KEY_END: return ImGuiKey_End;
            case GLFW_KEY_INSERT: return ImGuiKey_Insert;
            case GLFW_KEY_DELETE: return ImGuiKey_Delete;
            case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
            case GLFW_KEY_SPACE: return ImGuiKey_Space;
            case GLFW_KEY_ENTER: return ImGuiKey_Enter;
            case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
            case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
            case GLFW_KEY_COMMA: return ImGuiKey_Comma;
            case GLFW_KEY_MINUS: return ImGuiKey_Minus;
            case GLFW_KEY_PERIOD: return ImGuiKey_Period;
            case GLFW_KEY_SLASH: return ImGuiKey_Slash;
            case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
            case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
            case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
            case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
            case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
            case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
            case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
            case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
            case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
            case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
            case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
            case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
            case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
            case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
            case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
            case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
            case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
            case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
            case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
            case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
            case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
            case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
            case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
            case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
            case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
            case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
            case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
            case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
            case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
            case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
            case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
            case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
            case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
            case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
            case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
            case GLFW_KEY_MENU: return ImGuiKey_Menu;
            case GLFW_KEY_0: return ImGuiKey_0;
            case GLFW_KEY_1: return ImGuiKey_1;
            case GLFW_KEY_2: return ImGuiKey_2;
            case GLFW_KEY_3: return ImGuiKey_3;
            case GLFW_KEY_4: return ImGuiKey_4;
            case GLFW_KEY_5: return ImGuiKey_5;
            case GLFW_KEY_6: return ImGuiKey_6;
            case GLFW_KEY_7: return ImGuiKey_7;
            case GLFW_KEY_8: return ImGuiKey_8;
            case GLFW_KEY_9: return ImGuiKey_9;
            case GLFW_KEY_A: return ImGuiKey_A;
            case GLFW_KEY_B: return ImGuiKey_B;
            case GLFW_KEY_C: return ImGuiKey_C;
            case GLFW_KEY_D: return ImGuiKey_D;
            case GLFW_KEY_E: return ImGuiKey_E;
            case GLFW_KEY_F: return ImGuiKey_F;
            case GLFW_KEY_G: return ImGuiKey_G;
            case GLFW_KEY_H: return ImGuiKey_H;
            case GLFW_KEY_I: return ImGuiKey_I;
            case GLFW_KEY_J: return ImGuiKey_J;
            case GLFW_KEY_K: return ImGuiKey_K;
            case GLFW_KEY_L: return ImGuiKey_L;
            case GLFW_KEY_M: return ImGuiKey_M;
            case GLFW_KEY_N: return ImGuiKey_N;
            case GLFW_KEY_O: return ImGuiKey_O;
            case GLFW_KEY_P: return ImGuiKey_P;
            case GLFW_KEY_Q: return ImGuiKey_Q;
            case GLFW_KEY_R: return ImGuiKey_R;
            case GLFW_KEY_S: return ImGuiKey_S;
            case GLFW_KEY_T: return ImGuiKey_T;
            case GLFW_KEY_U: return ImGuiKey_U;
            case GLFW_KEY_V: return ImGuiKey_V;
            case GLFW_KEY_W: return ImGuiKey_W;
            case GLFW_KEY_X: return ImGuiKey_X;
            case GLFW_KEY_Y: return ImGuiKey_Y;
            case GLFW_KEY_Z: return ImGuiKey_Z;
            case GLFW_KEY_F1: return ImGuiKey_F1;
            case GLFW_KEY_F2: return ImGuiKey_F2;
            case GLFW_KEY_F3: return ImGuiKey_F3;
            case GLFW_KEY_F4: return ImGuiKey_F4;
            case GLFW_KEY_F5: return ImGuiKey_F5;
            case GLFW_KEY_F6: return ImGuiKey_F6;
            case GLFW_KEY_F7: return ImGuiKey_F7;
            case GLFW_KEY_F8: return ImGuiKey_F8;
            case GLFW_KEY_F9: return ImGuiKey_F9;
            case GLFW_KEY_F10: return ImGuiKey_F10;
            case GLFW_KEY_F11: return ImGuiKey_F11;
            case GLFW_KEY_F12: return ImGuiKey_F12;
            default: return ImGuiKey_None;
          }
        }
        void ImGui_ImplGlfw_UpdateKeyModifiers(int mods)
        {
          io.AddKeyEvent(ImGuiKey_ModCtrl, (mods & GLFW_MOD_CONTROL) != 0);
          io.AddKeyEvent(ImGuiKey_ModShift, (mods & GLFW_MOD_SHIFT) != 0);
          io.AddKeyEvent(ImGuiKey_ModAlt, (mods & GLFW_MOD_ALT) != 0);
          io.AddKeyEvent(ImGuiKey_ModSuper, (mods & GLFW_MOD_SUPER) != 0);
        }
        static int ImGui_ImplGlfw_KeyToModifier(int key)
        {
          if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
            return GLFW_MOD_CONTROL;
          if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
            return GLFW_MOD_SHIFT;
          if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
            return GLFW_MOD_ALT;
          if (key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER)
            return GLFW_MOD_SUPER;
          return 0;
        }

        static int ImGui_ImplGlfw_TranslateUntranslatedKey(int key, int scancode)
        {
            // GLFW 3.1+ attempts to "untranslate" keys, which goes the opposite of what every other framework does, making using lettered shortcuts difficult.
            // (It had reasons to do so: namely GLFW is/was more likely to be used for WASD-type game controls rather than lettered shortcuts, but IHMO the 3.1 change could have been done differently)
            // See https://github.com/glfw/glfw/issues/1502 for details.
            // Adding a workaround to undo this (so our keys are translated->untranslated->translated, likely a lossy process).
            // This won't cover edge cases but this is at least going to cover common cases.
            if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_EQUAL)
                return key;
            const char* key_name = glfwGetKeyName(key, scancode);
            if (key_name && key_name[0] != 0 && key_name[1] == 0)
            {
              const char char_names[] = "`-=[]\\,;\'./";
              const int char_keys[] = { GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_MINUS, GLFW_KEY_EQUAL, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_BACKSLASH, GLFW_KEY_COMMA, GLFW_KEY_SEMICOLON, GLFW_KEY_APOSTROPHE, GLFW_KEY_PERIOD, GLFW_KEY_SLASH, 0 };
              IM_ASSERT(IM_ARRAYSIZE(char_names) == IM_ARRAYSIZE(char_keys));
              if (key_name[0] >= '0' && key_name[0] <= '9')               { key = GLFW_KEY_0 + (key_name[0] - '0'); }
              else if (key_name[0] >= 'A' && key_name[0] <= 'Z')          { key = GLFW_KEY_A + (key_name[0] - 'A'); }
              else if (key_name[0] >= 'a' && key_name[0] <= 'z')          { key = GLFW_KEY_A + (key_name[0] - 'a'); }
              else if (const char* p = strchr(char_names, key_name[0]))   { key = char_keys[p - char_names]; }
            }
            // if (action == GLFW_PRESS) printf("key %d scancode %d name '%s'\n", key, scancode, key_name);
            return key;
        }

        void on_mouse_button(int button, int action, int modifiers) final override
        {
          ImGui_ImplGlfw_UpdateKeyModifiers(modifiers);

          if (button >= 0 && button < ImGuiMouseButton_COUNT)
            io.AddMouseButtonEvent(button, action == GLFW_PRESS);
        }
        void on_mouse_wheel(double x, double y) final override
        {
          io.AddMouseWheelEvent((float)x, (float)y);
        }
        void on_mouse_move(double x, double y) final override
        {
          if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
          {
            const glm::uvec2 pos = ref->window.get_position();
            x += pos.x;
            y += pos.y;
          }
          io.AddMousePosEvent((float)x, (float)y);
        }

        void on_key(int keycode, int scancode, int action, int mods) final override
        {
          // Workaround: X11 does not include current pressed/released modifier key in 'mods' flags. https://github.com/glfw/glfw/issues/1630
          int keycode_to_mod = ImGui_ImplGlfw_KeyToModifier(keycode);
          if (keycode_to_mod)
            mods = (action == GLFW_PRESS) ? (mods | keycode_to_mod) : (mods & ~keycode_to_mod);
          ImGui_ImplGlfw_UpdateKeyModifiers(mods);

          // FIXME:
//               if (keycode >= 0 && keycode < IM_ARRAYSIZE(bd->KeyOwnerWindows))
//                   bd->KeyOwnerWindows[keycode] = (action == GLFW_PRESS) ? window : NULL;

          keycode = ImGui_ImplGlfw_TranslateUntranslatedKey(keycode, scancode);

          ImGuiKey imgui_key = ImGui_ImplGlfw_KeyToImGuiKey(keycode);
          io.AddKeyEvent(imgui_key, (action == GLFW_PRESS));
          io.SetKeyEventNativeData(imgui_key, keycode, scancode); // To support legacy indexing (<1.87 user code)
        }
        void on_unicode_input(unsigned int code) final override
        {
          io.AddInputCharacter(code);
        }

        void window_closed() final override
        {
          ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(ref);
          if (viewport)
            viewport->PlatformRequestClose = true;
        }
        void window_focused(bool focused) final override { io.AddFocusEvent(focused != 0); }
        void window_iconified(bool /*iconified*/) final override {}

        void on_mouse_entered(bool entered) final override
        {
          // FIXME:
//               if (entered)
//               {
//                   bd->MouseWindow = window;
//                   io.AddMousePosEvent(bd->LastValidMousePos.x, bd->LastValidMousePos.y);
//               }
//               else if (!entered && bd->MouseWindow == window)
//               {
//                   bd->LastValidMousePos = io.MousePos;
//                   bd->MouseWindow = NULL;
//                   io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
//               }
        }
        void window_resized(const glm::vec2 &/*new_size*/) final override
        {
          ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(ref);
          if (viewport)
          {
//                   bool ignore_event = (ImGui::GetFrameCount() <= vd->IgnoreWindowSizeEventFrame + 1);
//                   //data->IgnoreWindowSizeEventFrame = -1;
//                   if (ignore_event)
//                       return;
//               }
            viewport->PlatformRequestResize = true;
          }
          if (this == ctx.main_vp)
          {
            glm::uvec2 sz = ref->window.get_size();
            glm::uvec2 fb_sz = ref->window.get_framebuffer_size();
            io.DisplaySize = ImVec2((float)sz.x, (float)sz.y);
            if (sz.x > 0 && sz.y > 0)
                io.DisplayFramebufferScale = ImVec2((float)fb_sz.x / (float)sz.x, (float)fb_sz.y / (float)sz.y);
          }
        }

        void window_position_changed(const glm::vec2 &/*new_pos*/) final override
        {
          ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(ref);
          if (viewport)
          {
            // FIXME:
//                 if (ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData)
//                 {
//                     bool ignore_event = (ImGui::GetFrameCount() <= vd->IgnoreWindowPosEventFrame + 1);
//                     //data->IgnoreWindowPosEventFrame = -1;
//                     if (ignore_event)
//                         return;
//                 }
            viewport->PlatformRequestMove = true;
          }
        }

        win_event_listener(ImGuiIO& _io, glfw::glfw_module::state_ref_t* _ref, imgui_context& _ctx, bool _owned = false)
          : io(_io)
          , ctx(_ctx)
          , ref(_ref)
          , owned(_owned)
        {
          events_tk += ref->window.get_event_manager().on_raw_mouse_event.add(this);
          events_tk += ref->window.get_event_manager().on_raw_keyboard_event.add(this);
          events_tk += ref->window.get_event_manager().on_window_event.add(this);
          events_tk += ref->on_context_destroyed.add([this]
          {
            events_tk.release();
          });
        }

        ImGuiIO& io;
        imgui_context& ctx;
        glfw::glfw_module::state_ref_t* ref;
        bool owned = false;
        cr::event_token_list_t events_tk;
      };

    public:
      using viewport_ref_t = glfw::glfw_module::state_ref_t;
      imgui_context(hydra_context& _ctx, engine_t& _engine, viewport_ref_t& main_viewport)
      : ctx(_ctx),
        engine(_engine),
        context(*ImGui::CreateContext()),
        plot_context(*ImPlot::CreateContext()),
        io(ImGui::GetIO())
      {
        IMGUI_CHECKVERSION();
        switch_to();

        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable
                        | ImGuiConfigFlags_ViewportsEnable;
//                            | ImGuiConfigFlags_DpiEnableScaleViewports
//                            | ImGuiConfigFlags_DpiEnableScaleFonts;
//             io.ConfigViewportsNoAutoMerge = true;
        io.ConfigViewportsNoTaskBarIcon = true;

        io.IniFilename = nullptr;
        on_configuration_changed_tk = conf.hconf_on_data_changed.add([this]
        {
          ImGui::LoadIniSettingsFromMemory(conf.data.c_str(), conf.data.size());
        });

//             io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines;

        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        platform_io.Platform_CreateWindow = _t_platform_create_window;
        platform_io.Platform_DestroyWindow = _t_platform_destroy_window;
        platform_io.Platform_ShowWindow = _t_platform_show_window;

        platform_io.Platform_SetWindowPos = _t_platform_set_window_pos;
        platform_io.Platform_GetWindowPos = _t_platform_get_window_pos;

        platform_io.Platform_SetWindowSize = _t_platform_set_window_size;
        platform_io.Platform_GetWindowSize = _t_platform_get_window_size;

        platform_io.Platform_SetWindowFocus = _t_platform_set_window_focus;
        platform_io.Platform_GetWindowFocus = _t_platform_get_window_focus;

        platform_io.Platform_GetWindowMinimized = _t_platform_get_window_minimized;

        platform_io.Platform_SetWindowTitle = _t_platform_set_window_title;

        platform_io.Platform_RenderWindow = _t_platform_render_window;

        platform_io.Platform_SetWindowAlpha = _t_platform_set_window_opacity;

        // FIXME:
//             io.SetClipboardTextFn = ImGui_ImplGlfw_SetClipboardText;
//             io.GetClipboardTextFn = ImGui_ImplGlfw_GetClipboardText;
//             io.ClipboardUserData = bd->Window;


        ImGuiViewport* im_main_viewport = ImGui::GetMainViewport();
        im_main_viewport->PlatformHandle = &main_viewport;
        win_event_listener* wevt = new win_event_listener(io, &main_viewport, *this, false);
        im_main_viewport->PlatformUserData = wevt;
        main_vp = wevt;

        content_scale = main_viewport.window.get_content_scale();
//             content_scale = glm::vec2(1, 1);//main_viewport.window.get_content_scale();
        const float scale = (std::max(content_scale.x, content_scale.y));


        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        hydra_dark_theme();

        // Scale everything:
        ImGui::GetStyle().ScaleAllSizes(scale);
        ImGui::GetStyle().AntiAliasedLinesUseTex = false;

        // Setup capabilities:
        io.BackendPlatformName = "hydra";
        io.BackendRendererName = "hydra";

        io.BackendPlatformUserData = this;
        // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset
                          | ImGuiBackendFlags_RendererHasViewports
                          | ImGuiBackendFlags_PlatformHasViewports;

        glm::uvec2 sz = main_vp->ref->window.get_size();
        glm::uvec2 fb_sz = main_vp->ref->window.get_framebuffer_size();
        io.DisplaySize = ImVec2((float)sz.x, (float)sz.y);
        if (sz.x > 0 && sz.y > 0)
          io.DisplayFramebufferScale = ImVec2((float)fb_sz.x / (float)sz.x, (float)fb_sz.y / (float)sz.y);
      }

      virtual ~imgui_context()
      {
        ImPlot::DestroyContext(&plot_context);
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

      void on_resource_index_loaded()
      {
        if (ctx.res._has_prefix_directory())
        {
          ctx.hconf.read_or_create_conf(conf);
        }
        else
        {
          // we read from a in-memory index, force a resource load (avoid going through IO for this)
          ctx.hconf.read_conf(conf);
        }
      }

      void switch_to()
      {
        ImGui::SetCurrentContext(&context);
        ImPlot::SetCurrentContext(&plot_context);
      }

      bool is_current_context() const
      {
        return ImGui::GetCurrentContext() == &context;
      }

      float get_content_scale() const { return (std::max(content_scale.x, content_scale.y)); }

      void new_frame()
      {
        TRACY_SCOPED_ZONE;

        {
          // check conf changes:
          if (io.WantSaveIniSettings)
          {
            if (ctx.res._has_prefix_directory())
            {
              // only save the ini settings if there's a prefix directory to save it
              std::string new_data = ImGui::SaveIniSettingsToMemory();
              if (new_data != conf.data)
              {
                conf.data = std::move(new_data);
                ctx.hconf.write_conf(conf);
              }
            }
            io.WantSaveIniSettings = false;
          }
        }

//             const glm::vec2 new_content_scale = glm::vec2(1, 1);//!main_vp ? glm::vec2(1, 1) : main_vp->ref->window.get_content_scale();
        const glm::vec2 new_content_scale = !main_vp ? glm::vec2(1, 1) : main_vp->ref->window.get_content_scale();
        if (new_content_scale != content_scale || has_font_change)
        {
          TRACY_SCOPED_ZONE;
          const float old_scale = (std::max(content_scale.x, content_scale.y));
          content_scale = new_content_scale;
          const float scale = (std::max(content_scale.x, content_scale.y));

          const float scale_fct = scale / old_scale;

          // Scale the style:
          ImGui::GetStyle().ScaleAllSizes(scale_fct);

          // scale the fonts:
          std::lock_guard _l(font_lock);
          {
            io.Fonts->Clear();
            has_font_change = false;
            io.FontGlobalScale = 1;
            for (uint32_t i = 0; i < _font_count; i += _mode_count)
            {
              const float default_font_size = 17;
              const float font_base_size = (i == monospace_font ? 13 : default_font_size);
              if (ttf_fonts[i].data.size > 0)
              {
                for (uint32_t m = 0; m < _mode_count; ++m)
                {
                  uint32_t font_idx = (ttf_fonts[i + m].data.size > 0 ? i + m : i);
                  ImFontConfig cfg;
                  cfg.FontDataOwnedByAtlas = false;
                  cfg.OversampleH = 1;
                  cfg.OversampleV = 1;

#ifdef IMGUI_ENABLE_FREETYPE
//                       cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
                  cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
#endif
                  //cfg.GlyphOffset.y = (default_font_size - font_base_size) * scale * ttf_fonts[i].scale * 0.75f;
                  io.Fonts->AddFontFromMemoryTTF(ttf_fonts[font_idx].data.data.get(), ttf_fonts[font_idx].data.size, std::roundf(font_base_size * scale * ttf_fonts[i].scale), &cfg);
                }
              }
              else
              {
                for (uint32_t m = 0; m < _mode_count; ++m)
                {
                  ImFontConfig cfg;
                  cfg.OversampleH = 1;
                  cfg.OversampleV = 1;
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
        {
          TRACY_SCOPED_ZONE;
          ImGui_ImplGlfw_UpdateMonitors();
          const double current_time = glfwGetTime();
          io.DeltaTime = (old_time <= current_time ? (1.0f / 60.0f) : (float)(current_time - old_time));
          old_time = current_time;
//               if (main_vp)
          {
//                 TRACY_SCOPED_ZONE;
            // Setup display size (every frame to accommodate for window resizing)
//                 glm::uvec2 sz = main_vp->ref->window.get_size();
//                 glm::uvec2 fb_sz = main_vp->ref->window.get_framebuffer_size();
//                 io.DisplaySize = ImVec2((float)sz.x, (float)sz.y);
//                 if (sz.x > 0 && sz.y > 0)
//                     io.DisplayFramebufferScale = ImVec2((float)fb_sz.x / (float)sz.x, (float)fb_sz.y / (float)sz.y);
          }
//               ImGui_ImplGlfw_UpdateMouseData();

          // update mouse cursor:
          ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
          ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
          for (int n = 0; n < platform_io.Viewports.Size; n++)
          {
            TRACY_SCOPED_ZONE;
            viewport_ref_t* vpref = (viewport_ref_t*)platform_io.Viewports[n]->PlatformHandle;
            if (imgui_cursor == ImGuiMouseCursor_None)
            {
              vpref->window.disable_cursor(true);
            }
            else if (io.MouseDrawCursor)
            {
              // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
              vpref->window.hide_cursor(true);
            }
            else
            {
              // Show OS mouse cursor
              glfw::cursor c = glfw::cursor::arrow;
              switch (imgui_cursor)
              {
                case ImGuiMouseCursor_Arrow: c = glfw::cursor::arrow; break;
                case ImGuiMouseCursor_TextInput: c = glfw::cursor::ibeam; break;
                case ImGuiMouseCursor_ResizeAll: c = glfw::cursor::resize_all; break;
                case ImGuiMouseCursor_ResizeNS: c = glfw::cursor::resize_ns; break;
                case ImGuiMouseCursor_ResizeEW: c = glfw::cursor::resize_ew; break;
                case ImGuiMouseCursor_ResizeNESW: c = glfw::cursor::resize_nesw; break;
                case ImGuiMouseCursor_ResizeNWSE: c = glfw::cursor::resize_nwse; break;
                case ImGuiMouseCursor_Hand: c = glfw::cursor::pointing_hand; break;
                case ImGuiMouseCursor_NotAllowed: c = glfw::cursor::not_allowed; break;
                default:
                  cr::out().error("invalid/unknown imgui cursor: {}", (int)imgui_cursor);
              }
              vpref->window.set_cursor(c);
            }
          }
        }

        ImGui::NewFrame();
      }

      ImGuiIO& get_io() { return io; }
      const ImGuiIO& get_io() const { return io; }

      bool should_regenerate_fonts() const { return regenerate_fonts; }
      void set_font_as_regenerated() { regenerate_fonts = false; }

      void _load_font(string_id rid, uint32_t font, float scale = 1.0f)
      {
        ctx.res.read_resource<assets::raw_asset>(rid)
        .then([this, scale, idx = font, rid](assets::raw_asset&& font, resources::status st)
        {
          if (st == resources::status::success)
          {
            cr::out().debug("loaded font: {} (index: {})", ctx.res.resource_name(rid), idx);
          }
          else
          {
            cr::out().warn("failed to load font: {} (index: {})", ctx.res.resource_name(rid), idx);
            return;
          }
          std::lock_guard _l(font_lock);
          ttf_fonts[idx] = {std::move(font.data), scale};
          has_font_change = true;
        });
      }

  private: // imgui stuff
    void ImGui_ImplGlfw_UpdateMonitors()
    {
      ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
      if (platform_io.Monitors.size())
        return;
      TRACY_SCOPED_ZONE;
      int monitors_count = 0;
      GLFWmonitor** glfw_monitors = glfwGetMonitors(&monitors_count);
      platform_io.Monitors.resize(0);
      for (int n = 0; n < monitors_count; n++)
      {
        ImGuiPlatformMonitor monitor;
        int x, y;
        glfwGetMonitorPos(glfw_monitors[n], &x, &y);
        const GLFWvidmode* vid_mode = glfwGetVideoMode(glfw_monitors[n]);
        monitor.MainPos = monitor.WorkPos = ImVec2((float)x, (float)y);
        monitor.MainSize = monitor.WorkSize = ImVec2((float)vid_mode->width, (float)vid_mode->height);
// #if GLFW_HAS_MONITOR_WORK_AREA
        int w, h;
        glfwGetMonitorWorkarea(glfw_monitors[n], &x, &y, &w, &h);
        if (w > 0 && h > 0) // Workaround a small GLFW issue reporting zero on monitor changes: https://github.com/glfw/glfw/pull/1761
        {
          monitor.WorkPos = ImVec2((float)x, (float)y);
          monitor.WorkSize = ImVec2((float)w, (float)h);
        }
// #endif
// #if GLFW_HAS_PER_MONITOR_DPI
        // Warning: the validity of monitor DPI information on Windows depends on the application DPI awareness settings, which generally needs to be set in the manifest or at runtime.
        float x_scale, y_scale;
        glfwGetMonitorContentScale(glfw_monitors[n], &x_scale, &y_scale);
        monitor.DpiScale = x_scale;
// #endif
        platform_io.Monitors.push_back(monitor);
      }
    }

    static void add_render_pass_to_window(imgui_context* ctx, viewport_ref_t& ref, ImGuiViewport* vp);

    private: // inmgui multi-viewport suppory
      static void _t_platform_create_window(ImGuiViewport* vp)
      {
        TRACY_SCOPED_ZONE;
//             cr::out().debug("imgui-context: creating a new window (ID: {})", vp->ID);
        imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
        auto window_state = ctx->engine.get_module<glfw::glfw_module>("glfw"_rid)->create_window(glm::uvec2(vp->Size.x, vp->Size.y), "[hydra imgui window]",
        (vp->Flags & ImGuiViewportFlags_NoTaskBarIcon) ? glfw::window::_window_type::utility : glfw::window::_window_type::dialog,
        std::initializer_list<std::pair<int, int>>
        {
          {GLFW_VISIBLE, false},
          {GLFW_FOCUSED, false},
          {GLFW_TRANSPARENT_FRAMEBUFFER, true},
          {GLFW_FOCUS_ON_SHOW, false},
          {GLFW_DECORATED, (vp->Flags & ImGuiViewportFlags_NoDecoration) ? false : true},
          {GLFW_FLOATING, (vp->Flags & ImGuiViewportFlags_TopMost) ? true : false},
        });
        window_state->window.set_position(glm::uvec2((uint32_t)vp->Pos.x, (uint32_t)vp->Pos.y));

        window_state->_ctx_ref.clear_framebuffer = true;

        win_event_listener* wevt = new win_event_listener(ctx->io, window_state.get(), *ctx, true);
        vp->PlatformUserData = wevt;
        add_render_pass_to_window(ctx, *window_state.get(), vp);
        vp->PlatformHandle = window_state.release();
      }
      static void _t_platform_destroy_window(ImGuiViewport* vp)
      {
        TRACY_SCOPED_ZONE;
        imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;

        win_event_listener* wevt = (win_event_listener*)vp->PlatformUserData;
        const bool owned = wevt->owned;
//             cr::out().debug("imgui-context: destroying a window (ID: {}, owned: {})", vp->ID, owned);
        if (ctx->main_vp == wevt)
          ctx->main_vp = nullptr;
        delete wevt;

        if (owned)
        {
          viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
          delete vpref;
        }
        if (vp->RendererUserData)
        {
          delete ((draw_data_t*)vp->RendererUserData);
          vp->RendererUserData = nullptr;
        }
        vp->PlatformHandle = nullptr;
        vp->PlatformUserData = nullptr;
      }
      static void _t_platform_show_window(ImGuiViewport* vp)
      {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
        viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
        vpref->window.show();
      }

      static void _t_platform_set_window_pos(ImGuiViewport* vp, ImVec2 pos)
      {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
        viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
        vpref->window.set_position({pos.x, pos.y});
      }
      static ImVec2 _t_platform_get_window_pos(ImGuiViewport* vp)
      {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
        viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
        glm::uvec2 r = vpref->window.get_position();
        return {(float)r.x, (float)r.y};
      }

      static void _t_platform_set_window_size(ImGuiViewport* vp, ImVec2 size)
      {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
        viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
        vpref->window.set_size({size.x, size.y});
      }
      static ImVec2 _t_platform_get_window_size(ImGuiViewport* vp)
      {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
        viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
        glm::uvec2 r = vpref->window.get_size();
        return {(float)r.x, (float)r.y};
      }

      static void _t_platform_set_window_focus(ImGuiViewport* vp)
      {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
        viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
        vpref->window.focus();
      }
      static bool _t_platform_get_window_focus(ImGuiViewport* vp)
      {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
        viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
        return vpref->window.is_focused();
      }

      static bool _t_platform_get_window_minimized(ImGuiViewport* vp)
      {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
        viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
        return vpref->window.is_iconified();
      }

      static void _t_platform_set_window_title(ImGuiViewport* vp, const char* title)
      {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
        viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
        vpref->window.set_title(title);
      }
      static void _t_platform_set_window_opacity(ImGuiViewport* vp, float alpha)
      {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
        // viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
        // vpref->window.set_opacity(alpha);
      }

      static void _t_platform_render_window(ImGuiViewport*, void*) {}

    private: // style:
      static void hydra_dark_theme()
      {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.11f, 0.10f, 0.10f, 1.00f/*0.50f*/);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.16f, 0.16f, 0.16f, 0.79f/*0.25f*/);
        colors[ImGuiCol_Border]                 = ImVec4(0.69f, 0.69f, 0.69f, 0.29f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.41f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.01f, 0.01f, 0.01f, 0.42f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.20f, 0.20f, 0.20f, 0.49f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.22f, 0.23f, 0.70f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.42f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.06f, 0.06f, 0.45f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.20f, 0.20f, 0.20f, 0.48f);
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
        colors[ImGuiCol_Separator]              = ImVec4(0.48f, 0.48f, 0.48f, 0.29f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.42f, 0.45f, 0.51f, 0.68f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.28f, 0.28f, 0.28f, 0.45f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.48f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.44f, 0.47f, 0.66f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.21f, 0.21f, 0.21f, 0.52f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.14f, 0.14f, 0.49f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.33f, 0.67f, 0.86f, 0.64f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.01f, 0.01f, 0.01f, 0.44f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.00f, 0.50f, 0.90f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.00f, 0.50f, 0.90f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.28f, 0.28f, 0.28f, 0.47f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.22f, 0.23f, 0.60f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.67f, 0.86f, 0.64f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.03f, 0.02f, 0.07f, 0.56f);

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
        style.ChildBorderSize                   = 0;
        style.PopupBorderSize                   = 1;
        style.FrameBorderSize                   = 0;
        style.TabBorderSize                     = 0;
        style.WindowRounding                    = 0;
        style.ChildRounding                     = 0;
        style.FrameRounding                     = 0;
        style.PopupRounding                     = 0;
        style.ScrollbarRounding                 = 9;
        style.GrabRounding                      = 2;
        style.TabRounding                       = 2;
        style.LogSliderDeadzone                 = 4;
        style.AntiAliasedLines                  = true;
        style.AntiAliasedLinesUseTex            = true;
        style.AntiAliasedFill                   = true;
      }

    private:
      struct font_t
      {
        raw_data data;
        float scale = 1;
      };

    private:
      hydra_context& ctx;
      engine_t& engine;
      win_event_listener* main_vp = nullptr;

      ImGuiContext& context;
      ImPlotContext& plot_context;
      ImGuiIO& io;

      glm::vec2 content_scale;
      bool regenerate_fonts = true;

      spinlock font_lock;
      font_t ttf_fonts[_font_count];
      bool has_font_change = true;

      double old_time = 0;;

      vk::sampler font_sampler { ctx.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, -1000, 1000 }; std::optional<image_holder> font_texture;
      vk::descriptor_set font_texture_ds { ctx.device, VkDescriptorSet(nullptr) };
      friend class render_pass;

      imgui_configuration conf;
      cr::event_token_t on_configuration_changed_tk;
  };
} // namespace neam::hydra::imgui




