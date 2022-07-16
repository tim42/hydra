//
// created by : Timothée Feuillet
// date: 2022-5-23
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

#pragma once

#include <functional>

#include <hydra/engine/engine_module.hpp>
#include <ntools/threading/threading.hpp>
#include <ntools/id/string_id.hpp>

#include "imgui_context.hpp"
#include "imgui_renderpass.hpp"

namespace neam::hydra::imgui
{
  class imgui_module final : public engine_module<imgui_module>
  {
    public: // public interface:
      imgui_context& get_imgui_context() { return *context; };
      const imgui_context& get_imgui_context() const { return *context; };

      void register_function(id_t fid, std::function<void()> func)
      {
        functions.emplace_back(fid, std::move(func));
      }
      void unregister_function(id_t fid)
      {
        functions.erase(std::remove_if(functions.begin(), functions.end(), [fid](auto& x) { return x.first == fid; }), functions.end());
      }

      void create_context(glfw::glfw_module::state_ref_t& viewport)
      {
        context.emplace(*hctx, *engine, viewport);
        viewport.pm.add_pass<imgui::render_pass>(get_imgui_context(), *hctx, ImGui::GetMainViewport());
        if (res_have_loaded)
          on_resource_index_loaded();
      }

    private: // module interface:
      static constexpr const char* module_name = "imgui";

      static bool is_compatible_with(runtime_mode m)
      {
        // we need vulkan for imgui to be active
        if ((m & runtime_mode::hydra_context) == runtime_mode::none)
          return false;
        return true;
      }

      void add_task_groups(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_task_group("imgui"_rid, "imgui");
      }
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_dependency("imgui"_rid, "events"_rid);
        tgd.add_dependency("imgui"_rid, "io"_rid);
        tgd.add_dependency("render"_rid, "imgui"_rid);
      }

      void on_context_initialized() override
      {
        hctx->tm.set_start_task_group_callback("imgui"_rid, [this]
        {
          // no context, nothing to do
          if (!context.has_value())
            return;

          get_imgui_context().new_frame();

          for (auto& it : functions)
            it.second();
          ImGui::Render();
          ImGui::UpdatePlatformWindows();
          ImGui::RenderPlatformWindowsDefault();
        });
      }

      void on_resource_index_loaded() override
      {
        res_have_loaded = true;
        if (!context.has_value())
          return;
        get_imgui_context().load_default_fonts();
      }

      void on_start_shutdown() override
      {
        context.reset();
      }


    private:
      std::optional<imgui_context> context;

      std::vector<std::pair<id_t, std::function<void()>>> functions;

      bool res_have_loaded = false;

      friend class engine_t;
      friend engine_module<imgui_module>;
  };
}

