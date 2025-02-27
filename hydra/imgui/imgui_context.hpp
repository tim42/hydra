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
#include "shader_structs.hpp"
#include "ui_elements.hpp"

namespace neam::hydra::imgui
{
  namespace internals
  {
    class setup_pass;
  }
  namespace components
  {
    class render_pass;
  }

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
  struct win_event_listener;

  // NOTE: We cannot use the imgui vulkan implementation as the resource management is somewhat incorrect
  // (buffers are destroyed when they are still in use)
  // So we made our own using hydra facilities
  class imgui_context : public events::raw_keyboard_listener, public events::raw_mouse_listener
  {
    private:
      // FIXME: move that elsewhere

    public:
      imgui_context(hydra_context& _ctx, engine_t& _engine, glfw::window_state_t& main_viewport);
      virtual ~imgui_context();

      void load_default_fonts();
      void on_resource_index_loaded();

      void switch_to();
      bool is_current_context() const;

      float get_content_scale() const { return (std::max(content_scale.x, content_scale.y)); }

      void new_frame();

      ImGuiIO& get_io() { return io; }
      const ImGuiIO& get_io() const { return io; }

      bool should_regenerate_fonts() const { return regenerate_fonts; }
      void set_font_as_regenerated() { regenerate_fonts = false; }

      void _load_font(string_id rid, uint32_t font, float scale = 1.0f);

    private: // imgui stuff
      void ImGui_ImplGlfw_UpdateMonitors();

      static void add_render_pass_to_window(imgui_context* ctx, glfw::window_state_t& ws, ImGuiViewport* vp);

    private: // inmgui multi-viewport suppory
      static void _t_platform_create_window(ImGuiViewport* vp);
      static void _t_platform_destroy_window(ImGuiViewport* vp);
      static void _t_platform_show_window(ImGuiViewport* vp);
      static void _t_platform_set_window_pos(ImGuiViewport* vp, ImVec2 pos);
      static ImVec2 _t_platform_get_window_pos(ImGuiViewport* vp);
      static void _t_platform_set_window_size(ImGuiViewport* vp, ImVec2 size);
      static ImVec2 _t_platform_get_window_size(ImGuiViewport* vp);
      static void _t_platform_set_window_focus(ImGuiViewport* vp);
      static bool _t_platform_get_window_focus(ImGuiViewport* vp);
      static bool _t_platform_get_window_minimized(ImGuiViewport* vp);
      static void _t_platform_set_window_title(ImGuiViewport* vp, const char* title);
      static void _t_platform_set_window_opacity(ImGuiViewport* vp, float alpha);
      static void _t_platform_render_window(ImGuiViewport*, void*);

    private: // style:
      static void hydra_dark_theme();

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

      double old_time = 0;

      vk::sampler font_sampler { ctx.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, -1000, 1000 };
      std::optional<image_holder> font_texture;

      imgui_configuration conf;
      cr::event_token_t on_configuration_changed_tk;

      friend class render_pass;
      friend class imgui_setup_pass;
      friend internals::setup_pass;
      friend components::render_pass;
      friend struct win_event_listener;
  };
} // namespace neam::hydra::imgui




