//
// created by : Timothée Feuillet
// date: 2022-7-17
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

#include <ntools/chrono.hpp>

#include <hydra/hydra_debug.hpp>
#include <hydra/engine/engine.hpp>
#include <hydra/engine/engine_module.hpp>
#include <hydra/glfw/glfw_engine_module.hpp>  // Add the glfw module
#include <hydra/imgui/imgui_engine_module.hpp>  // Add the imgui module
#include <hydra/renderer/renderer_engine_module.hpp>
#include <hydra/imgui/generic_ui.hpp>

#include "fs_watcher.hpp"
#include "options.hpp"
#include "packer_engine_module.hpp"

namespace neam::hydra
{
  class packer_ui_module : private engine_module<packer_ui_module>
  {
    public:
    private:
      // imgui resources. Delay the opening of the window until those resources are present
      static constexpr id_t imgui_vs_rid = "shaders/engine/imgui/imgui.hsf:spirv(main_vs)"_rid;
      static constexpr id_t imgui_fs_rid = "shaders/engine/imgui/imgui.hsf:spirv(main_fs)"_rid;

    private:
      static constexpr const char* module_name = "packer-ui";

      static bool is_compatible_with(runtime_mode m)
      {
        // exclusion flags:
        if ((m & runtime_mode::packer_less) != runtime_mode::none)
          return false;
        // necessary stuff:
        if ((m & runtime_mode::hydra_context) != runtime_mode::hydra_context)
          return false;

        return true;
      }

      void on_context_initialized() override
      {
        window_state = engine->get_module<glfw::glfw_module>("glfw"_rid)->create_window(glm::uvec2{800, 200}, "HYDRA RESOURCE SERVER");
        window_state->window.iconify();

        auto* pck = engine->get_module<packer_engine_module>("packer"_rid);
        pck->stall_task_manager = false;

        auto* imgui = engine->get_module<imgui::imgui_module>("imgui"_rid);
        imgui->register_function("dockspace"_rid, [this]()
        {
          ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        });

        imgui->register_function("main"_rid, [this, pck]()
        {
          ImGui::ShowDemoWindow();

          ImGui::Begin("RelDB", nullptr, 0);
          imgui::generate_ui(cctx->res._get_serialized_reldb(), rel_db_metadata);
          ImGui::End();

          ImGui::Begin("Packer", nullptr, 0);
          if (pck->is_packing())
          {
            const uint32_t total = (uint32_t)(pck->get_total_entry_to_pack() == 0 ? 1 : pck->get_total_entry_to_pack());
            const uint32_t current_count = (uint32_t)(pck->get_packed_entries() == 0 ? 1 : pck->get_packed_entries());
            ImGui::Text("Status: Packing in progress : %u%%...", current_count * 100 / total);
            ImGui::ProgressBar((float)current_count / (float)total, ImVec2(-1, 0),
                               fmt::format("{} / {}", pck->get_packed_entries(), pck->get_total_entry_to_pack()).c_str());
          }
          else
          {
            ImGui::TextUnformatted("Status: Idle");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, 0xFF606060);
            ImGui::ProgressBar(1, ImVec2(-1, 0), "idle");
            ImGui::PopStyleColor();
          }

          if (ImGui::Button("Force Repack Everything"))
          {
            pck->packer_options.force = true;
          }


          ImGui::Separator();
          ImGui::Text("io: in flight: %u, pending: %u", cctx->io.get_in_flight_operations_count(), cctx->io.get_pending_operations_count());
          ImGui::Text("io: read: %.3f kb/s, write: %.3f kb/s", last_average_read_rate/1000, last_average_write_rate/1000);
          ImGui::Text("tm: pending tasks: %u", cctx->tm.get_pending_tasks_count());
          ImGui::Text("frametime: %.3f ms framerate: %.3f fps", last_average_frametime * 1000, 1.0f / last_average_frametime);

          ImGui::End();

          if (window_state->window.should_close())
            ImGui::OpenPopup("Confirm Exit");

          if (ImGui::BeginPopupModal("Confirm Exit", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
          {
            ImGui::TextUnformatted("Are you sure you want to exit the resource server ?");

            if (pck->is_packing())
            {
              ImGui::PushFont(imgui::get_font(imgui::bold));
              ImGui::TextUnformatted("The resource packer is still packing");
              ImGui::TextUnformatted("This may leave the index in an incoherent state");
              ImGui::PopFont();
              ImGui::Separator();
              bool value = !pck->packer_options.watch;
              ImGui::Checkbox("Exit when packing is done", &value);
              pck->packer_options.watch = !value;
            }

            ImGui::Separator();
            if (ImGui::Button("Cancel", ImVec2(200, 0)))
            {
              window_state->window.should_close(false);
              ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();

            ImGui::SameLine();

            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 200);
            if (ImGui::Button("Exit", ImVec2(200, 0)))
              engine->sync_teardown();
            ImGui::EndPopup();
          }
        });

        auto* renderer = engine->get_module<renderer_module>("renderer"_rid);
        renderer->min_frame_time = 0.016f;

        renderer->register_on_render_start("packer-ui"_rid, [this, pck, renderer]
        {
          cctx->tm.get_task([this, pck, renderer]
          {
            // stats:
            ++frame_cnt;
            const double max_dt_s = 1.0f;
            if (chrono.get_accumulated_time() >= max_dt_s)
            {
              const double dt = chrono.delta();
              uint64_t current_read_bytes = cctx->io.get_total_read_bytes();
              uint64_t current_written_bytes = cctx->io.get_total_written_bytes();

              last_average_frametime = (dt / frame_cnt);
              last_average_read_rate = (current_read_bytes - initial_read_bytes) / dt;
              last_average_write_rate = (current_written_bytes - initial_written_bytes) / dt;

              frame_cnt = 0;
              initial_read_bytes = current_read_bytes;
              initial_written_bytes = current_written_bytes;
            }


            // window:
            if (pck->is_packing())
            {
              current_state = packer_state_t::packing;
              renderer->min_frame_time = 0.016f;
            }
            else
            {
              current_state = packer_state_t::idle;
              renderer->min_frame_time = 0.0f;
              if (window_state->window.is_focused())
              {
                std::this_thread::sleep_for(std::chrono::milliseconds{16});
              }
              else
              {
                cctx->tm.request_stop([this]()
                {
                  std::this_thread::sleep_for(std::chrono::milliseconds{100});
                  cctx->tm.get_frame_lock().unlock();
                });
              }
            }

            if (!is_setup && window_state)
              window_state->window.iconify();

            set_window_icon();
          });
        });
      }

      void on_resource_index_loaded() override
      {
        cctx->tm.get_long_duration_task([this]
        {
          check_for_resources();
        });
      }

      void on_start_shutdown() override
      {
        window_state.reset();
      }

    private:
      void set_window_icon()
      {
        if (!window_state)
          return;

        if (icon_state == current_state)
          return;
        icon_state = current_state;
        uint32_t color = 0x00;
        switch (icon_state)
        {
          case packer_state_t::none:
          case packer_state_t::idle:
            color = 0x636363;
            break;
          case packer_state_t::has_error:
            color = 0x0034e0;
            break;
          case packer_state_t::has_warnings:
            color = 0x00bbe0;
            break;
          case packer_state_t::packing:
            color = 0xffffff;
            break;
        };
        window_state->window._set_hydra_icon(color);
      }

      void setup_window()
      {
        cr::out().debug("found imgui shaders, creating application main window/render-context...");
        auto* imgui = engine->get_module<imgui::imgui_module>("imgui"_rid);
        imgui->create_context(*window_state.get());
        is_setup = true;
        window_state->window.show();
      }

      void check_for_resources()
      {
        const bool can_launch = cctx->res.has_resource(imgui_vs_rid) && cctx->res.has_resource(imgui_fs_rid);
        if (can_launch /*&& current_state == packer_state_t::idle*/)
        {
          return setup_window();
        }
        else
        {
          // finish by launching the same task, again
          cctx->tm.get_long_duration_task([this]
          {
            // TODO: only when a resource has been packed ?
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
            check_for_resources();
          });
        }
      }

    private:
      cr::chrono chrono;
      uint32_t frame_cnt = 0;
      uint64_t initial_read_bytes = 0;
      uint64_t initial_written_bytes = 0;
      float last_average_frametime = 0;
      float last_average_read_rate = 0;
      float last_average_write_rate = 0;


      rle::serialization_metadata rel_db_metadata = rle::generate_metadata<resources::rel_db>();

      std::unique_ptr<glfw::glfw_module::state_ref_t> window_state;
      bool is_setup = false;

      enum class packer_state_t
      {
        none,
        has_error,
        has_warnings,
        packing,
        idle
      };
      packer_state_t icon_state = packer_state_t::none;
      packer_state_t current_state = packer_state_t::idle;

      friend class engine_t;
      friend engine_module<packer_ui_module>;
  };
}
