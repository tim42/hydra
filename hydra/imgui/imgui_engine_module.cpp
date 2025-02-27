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

#include "imgui_engine_module.hpp"
#include "imgui_renderpass.hpp"

namespace neam::hydra::imgui
{
  void imgui_module::register_function(id_t fid, std::function<void()> func)
  {
    functions.emplace_back(fid, std::move(func));
  }
  void imgui_module::unregister_function(id_t fid)
  {
    functions.erase(std::remove_if(functions.begin(), functions.end(), [fid](auto & x)
    {
      return x.first == fid;
    }), functions.end());
  }

  void imgui_module::create_context(glfw::window_state_t& ws)
  {
    check::debug::n_check(!context, "creating an imgui context over an existing imgui context");
    {
      std::lock_guard _lg(lock);
      context.emplace(*hctx, *engine, ws);
    }
    {
      std::lock_guard _el(spinlock_exclusive_adapter::adapt(ws.render_entity.get_lock()));
      ws.render_entity.add<internals::setup_pass>(*hctx);
      ws.render_entity.add<components::render_pass>(*hctx, ImGui::GetMainViewport());
    }
    if (res_have_loaded)
      on_resource_index_loaded();
  }

  void imgui_module::reload_fonts()
  {
    if (res_have_loaded)
      on_resource_index_loaded();
  }

  bool imgui_module::is_compatible_with(runtime_mode m)
  {
    // we need vulkan for imgui to be active
    if ((m & runtime_mode::hydra_context) != runtime_mode::hydra_context)
      return false;
    return true;
  }

  void imgui_module::add_task_groups(threading::task_group_dependency_tree& tgd)
  {
    tgd.add_task_group("imgui_render"_rid);

    // restricted to main because it needs to interract with GLFW in a sync way
    tgd.add_task_group("imgui"_rid, { .restrict_to_named_thread = "main"_rid, });
  }
  void imgui_module::add_task_groups_dependencies(threading::task_group_dependency_tree& tgd)
  {
    tgd.add_dependency("imgui"_rid, "glfw/events"_rid);
    tgd.add_dependency("imgui_render"_rid, "imgui"_rid);
    //tgd.add_dependency("imgui_render"_rid, "render"_rid);

    // tgd.add_dependency("imgui"_rid, "render"_rid); // TODO: remove
    // tgd.add_dependency("imgui_render"_rid, "render"_rid);
     tgd.add_dependency("render"_rid, "imgui_render"_rid);
     tgd.add_dependency("glfw/framebuffer_acquire"_rid, "imgui"_rid);
  }

  void imgui_module::on_context_initialized()
  {
    // static bool has_rendered_anything = false;
    hctx->tm.set_start_task_group_callback("imgui_render"_rid, [this]
    {
      if (!context.has_value())
        return;

      hctx->tm.get_task([]
      {
        // if (!has_rendered_anything)
        // return;
        TRACY_SCOPED_ZONE;
        ImGui::Render();

        // Duplication of the draw data
        {
          ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
          for (int i = 0; i < platform_io.Viewports.Size; i++)
          {
            auto* vp = platform_io.Viewports[i];
            if (vp->RendererUserData)
              delete ((draw_data_t*) vp->RendererUserData);
            vp->RendererUserData = nullptr;

            if ((vp->Flags & ImGuiViewportFlags_IsMinimized) == 0 && vp->DrawData != nullptr)
            {
              vp->RendererUserData = new draw_data_t(*vp->DrawData);
            }
          }
        }

        // has_rendered_anything = false;
      });
    });

    hctx->tm.set_start_task_group_callback("imgui"_rid, [this]
    {
      // no context, nothing to do
      if (!context.has_value())
        return;

      hctx->tm.get_task([this]
      {
        TRACY_SCOPED_ZONE;
        get_imgui_context().new_frame();

        for (auto& it: functions)
          it.second();

        ImGui::EndFrame();
        // has_rendered_anything = true;
        ImGui::UpdatePlatformWindows();
      });
    });
  }

  void imgui_module::on_resource_index_loaded()
  {
    std::lock_guard _lg(lock);
    res_have_loaded = true;
    if (!context.has_value())
      return;
    get_imgui_context().load_default_fonts();
    if (!has_loaded_conf)
    {
      has_loaded_conf = true;
      get_imgui_context().on_resource_index_loaded();
    }
  }

  void imgui_module::on_shutdown_post_idle_gpu()
  {
    context.reset();
  }
}

