
//
// created by : Timothée Feuillet
// date: 2022-6-24
//
//
// Copyright (c) 2022 Timothée Feuillet
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

#include "imgui_context.hpp"
#include "imgui_renderpass.hpp"
#include "imgui_even_listener.hpp"

namespace neam::hydra::imgui
{
  imgui_context::imgui_context(hydra_context& _ctx, engine_t& _engine, glfw::window_state_t& main_viewport)
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
    win_event_listener* wevt = new win_event_listener(io, main_viewport, *this, false);
    im_main_viewport->PlatformUserData = wevt;
    main_vp = wevt;

    content_scale = main_viewport.win->get_content_scale();
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

    glm::uvec2 sz = main_vp->win_state.win->get_size();
    glm::uvec2 fb_sz = main_vp->win_state.win->get_framebuffer_size();
    io.DisplaySize = ImVec2((float)sz.x, (float)sz.y);
    if (sz.x > 0 && sz.y > 0)
      io.DisplayFramebufferScale = ImVec2((float)fb_sz.x / (float)sz.x, (float)fb_sz.y / (float)sz.y);
  }

  imgui_context::~imgui_context()
  {
    ImPlot::DestroyContext(&plot_context);
    ImGui::DestroyContext(&context);
  }

  void imgui_context::load_default_fonts()
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

  void imgui_context::on_resource_index_loaded()
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

  void imgui_context::switch_to()
  {
    ImGui::SetCurrentContext(&context);
    ImPlot::SetCurrentContext(&plot_context);
  }

  bool imgui_context::is_current_context() const
  {
    return ImGui::GetCurrentContext() == &context;
  }

  void imgui_context::new_frame()
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
    const glm::vec2 new_content_scale = !main_vp ? glm::vec2(1, 1) : main_vp->win_state.win->get_content_scale();
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
        glfw::window_state_t* ws = (glfw::window_state_t*)platform_io.Viewports[n]->PlatformHandle;
        if (imgui_cursor == ImGuiMouseCursor_None)
        {
          ws->win->disable_cursor(true);
        }
        else if (io.MouseDrawCursor)
        {
          // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
          ws->win->hide_cursor(true);
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
          ws->win->set_cursor(c);
        }
      }
    }

    ImGui::NewFrame();
  }


  void imgui_context::_load_font(string_id rid, uint32_t font, float scale)
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

  void imgui_context::ImGui_ImplGlfw_UpdateMonitors()
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


  void imgui_context::_t_platform_create_window(ImGuiViewport* vp)
  {
    TRACY_SCOPED_ZONE;
    imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
    auto* glfw = ctx->engine.get_module<glfw::glfw_module>();
    auto window_state = std::make_unique<glfw::window_state_t>(glfw->create_window(glm::uvec2(vp->Size.x, vp->Size.y), "[hydra imgui window]",
      (vp->Flags & ImGuiViewportFlags_NoTaskBarIcon) ? glfw::window::_window_type::utility : glfw::window::_window_type::dialog,
      std::initializer_list<std::pair<int, int>>
      {
        {GLFW_VISIBLE, false},
        {GLFW_FOCUSED, false},
        {GLFW_TRANSPARENT_FRAMEBUFFER, false},
        {GLFW_FOCUS_ON_SHOW, false},
        {GLFW_DECORATED, (vp->Flags & ImGuiViewportFlags_NoDecoration) ? false : true},
        {GLFW_FLOATING, (vp->Flags & ImGuiViewportFlags_TopMost) ? true : false},
      }));
    cr::out().debug("imgui-context: creating a new window (ID: {})", vp->ID);
    window_state->win->set_position(glm::uvec2((uint32_t)vp->Pos.x, (uint32_t)vp->Pos.y));

    // FIXME!
    //window_state.win->_ctx_ref.clear_framebuffer = true;

    add_render_pass_to_window(ctx, *window_state, vp);
    win_event_listener* wevt = new win_event_listener(ctx->io, *window_state, *ctx, true);
    vp->PlatformHandle = window_state.release();
    vp->PlatformUserData = wevt;
  }
  void imgui_context::_t_platform_destroy_window(ImGuiViewport* vp)
  {
    TRACY_SCOPED_ZONE;
    imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;

    win_event_listener* wevt = (win_event_listener*)vp->PlatformUserData;
    const bool owned = wevt->owned;
    cr::out().debug("imgui-context: destroying a window (ID: {}, owned: {})", vp->ID, owned);
    if (ctx->main_vp == wevt)
      ctx->main_vp = nullptr;
    delete wevt;

    if (owned)
    {
      glfw::window_state_t* ws = (glfw::window_state_t*)vp->PlatformHandle;
      delete ws;
    }
    if (vp->RendererUserData)
    {
      delete ((draw_data_t*)vp->RendererUserData);
      vp->RendererUserData = nullptr;
    }
    vp->PlatformHandle = nullptr;
    vp->PlatformUserData = nullptr;
  }
  void imgui_context::_t_platform_show_window(ImGuiViewport* vp)
  {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
    glfw::window_state_t* ws = (glfw::window_state_t*)vp->PlatformHandle;
    ws->win->show();
  }

  void imgui_context::_t_platform_set_window_pos(ImGuiViewport* vp, ImVec2 pos)
  {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
    glfw::window_state_t* ws = (glfw::window_state_t*)vp->PlatformHandle;
    ws->win->set_position({pos.x, pos.y});
  }
  ImVec2 imgui_context::_t_platform_get_window_pos(ImGuiViewport* vp)
  {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
    glfw::window_state_t* ws = (glfw::window_state_t*)vp->PlatformHandle;
    glm::uvec2 r = ws->win->get_position();
    return {(float)r.x, (float)r.y};
  }

  void imgui_context::_t_platform_set_window_size(ImGuiViewport* vp, ImVec2 size)
  {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
    glfw::window_state_t* ws = (glfw::window_state_t*)vp->PlatformHandle;
    ws->win->set_size({size.x, size.y});
  }
  ImVec2 imgui_context::_t_platform_get_window_size(ImGuiViewport* vp)
  {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
    glfw::window_state_t* ws = (glfw::window_state_t*)vp->PlatformHandle;
    glm::uvec2 r = ws->win->get_size();
    return {(float)r.x, (float)r.y};
  }

  void imgui_context::_t_platform_set_window_focus(ImGuiViewport* vp)
  {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
    glfw::window_state_t* ws = (glfw::window_state_t*)vp->PlatformHandle;
    ws->win->focus();
  }
  bool imgui_context::_t_platform_get_window_focus(ImGuiViewport* vp)
  {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
    glfw::window_state_t* ws = (glfw::window_state_t*)vp->PlatformHandle;
    return ws->win->is_focused();
  }

  bool imgui_context::_t_platform_get_window_minimized(ImGuiViewport* vp)
  {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
    glfw::window_state_t* ws = (glfw::window_state_t*)vp->PlatformHandle;
    return ws->win->is_iconified();
  }

  void imgui_context::_t_platform_set_window_title(ImGuiViewport* vp, const char* title)
  {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
    glfw::window_state_t* ws = (glfw::window_state_t*)vp->PlatformHandle;
    ws->win->set_title(title);
  }
  void imgui_context::_t_platform_set_window_opacity(ImGuiViewport* vp, float alpha)
  {
//             imgui_context* ctx = (imgui_context*)ImGui::GetIO().BackendPlatformUserData;
    // viewport_ref_t* vpref = (viewport_ref_t*)vp->PlatformHandle;
    // vpref->window.set_opacity(alpha);
  }

  void imgui_context::_t_platform_render_window(ImGuiViewport*, void*) {}

  void imgui_context::hydra_dark_theme()
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

  void imgui_context::add_render_pass_to_window(imgui_context* ctx, glfw::window_state_t& ws, ImGuiViewport* vp)
  {
    // FIXME! should be one single setup pass for the engine
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(ws.render_entity.get_lock()));
    ws.render_entity.add<internals::setup_pass>(ctx->ctx);
    ws.render_entity.add<components::render_pass>(ctx->ctx, vp);
  }
}

