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
#include <ntools/enum.hpp>

#include <hydra/hydra_debug.hpp>
#include <hydra/engine/engine.hpp>
#include <hydra/engine/engine_module.hpp>
#include <hydra/engine/core_modules/core_module.hpp>
#include <hydra/glfw/glfw_engine_module.hpp>  // Add the glfw module
#include <hydra/imgui/imgui_engine_module.hpp>  // Add the imgui module
#include <hydra/imgui/utilities/imgui_folder_view.hpp>  // Add the folder view
#include <hydra/renderer/renderer_engine_module.hpp>
#include <hydra/imgui/generic_ui.hpp>

#include "fs_watcher.hpp"
#include "options.hpp"
#include "packer_engine_module.hpp"


namespace neam::hydra
{

  enum class packer_state_t
  {
    none = 0,
    has_error = 1<<0,
    has_warnings = 1<<1,
    packing = 1<<2,
    idle = 1<<3
  };
  N_ENUM_FLAG(packer_state_t);

  class packer_ui_module : private engine_module<packer_ui_module>
  {
    public:
    private:
      // imgui resources. Delay the opening of the window until those resources are present
      static constexpr id_t imgui_vs_rid = "shaders/engine/imgui/imgui.hsf:spirv(main_vs)"_rid;
      static constexpr id_t imgui_fs_rid = "shaders/engine/imgui/imgui.hsf:spirv(main_fs)"_rid;

      imgui::folder_view folder_view;
      std::filesystem::path selected_file;

      conf::gen_conf resource_ctx_conf;

    private:
      static constexpr neam::string_t module_name = "packer-ui";

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
        auto* pck = engine->get_module<packer_engine_module>("packer"_rid);
        auto* cm = engine->get_module<core_module>("core"_rid);
        auto* imgui = engine->get_module<imgui::imgui_module>("imgui"_rid);
        pck->stall_task_manager = false;

        on_resource_queued_tk = pck->on_resource_queued.add(*this, &packer_ui_module::on_resource_queued);
        on_resource_packed_tk = pck->on_resource_packed.add(*this, &packer_ui_module::on_resource_packed);
        on_index_saved_tk = pck->on_index_saved.add(*this, &packer_ui_module::on_index_saved);
        on_packing_started_tk = pck->on_packing_started.add(*this, &packer_ui_module::on_packing_started);
        on_packing_ended_tk = pck->on_packing_ended.add(*this, &packer_ui_module::on_packing_ended);

        auto* glfw_mod = engine->get_module<glfw::glfw_module>("glfw"_rid);
        window_state = glfw_mod->create_window(glm::uvec2{1200, 600}, "HYDRA RESOURCE SERVER");
        imgui->create_context(window_state);
        // window_state->window.iconify();
        //window_state->only_render_on_event = true;
        set_window_icon();

        folder_view.extra_columns = 2;
        folder_view.entry_extra_ui = [this](const std::filesystem::directory_entry& entry)
        {
          ImGui::TableNextColumn();

          std::set<id_t> ids = cctx->res.get_db().get_resources(((std::filesystem::path)(entry)).lexically_relative(cctx->res.source_folder));
          if (ids.size() > 0)
          {
            ImGui::Text("[%d]", (int)ids.size());
          }
          else
          {
            ImGui::Text("---");
          }
        };
        on_item_selected_tk = folder_view.on_selected.add([this](std::filesystem::path p)
        {
          selected_file = p;
        });

        imgui->register_function("dockspace"_rid, []()
        {
          ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        });

        imgui->register_function("main"_rid, [this, pck, glfw_mod]()
        {
          ImGui::ShowDemoWindow();

          if (ImGui::Begin("RelDB", nullptr, 0))
          {
            imgui::generate_ui(cctx->res._get_serialized_reldb(), rel_db_metadata);
          }
          ImGui::End();
          if (ImGui::Begin("ProjectView", nullptr, 0))
          {
            folder_view.root = cctx->res.source_folder;
            folder_view.render();
          }
          ImGui::End();
          if (ImGui::Begin("FileInspector", nullptr, 0))
          {
            ImGui::Text("%s", ((std::filesystem::path)(selected_file)).lexically_relative(cctx->res.source_folder).c_str());

            // FIXME: Proper preview of the thing:
            {
              id_t img_id = specialize(string_id::_runtime_build_from_string((((std::filesystem::path)(selected_file)).lexically_relative(cctx->res.source_folder)).c_str()), "image");
              if (cctx->res.get_index().has_entry(img_id))
              {
                float width = ImGui::GetContentRegionAvail().x * 3 / 4;
                ImGui::Image(img_id, ImVec2(width, width));
              }
            }

            // if (selected_file)
            {
              std::set<id_t> ids = cctx->res.get_db().get_referenced_metadata_types(((std::filesystem::path)(selected_file)).lexically_relative(cctx->res.source_folder));

              for (auto id : ids)
              {
                ImGui::Separator();
                auto md = cctx->res.get_db().get_type_metadata(id);

                ImGui::Text("%s", md.entry_name.c_str());
                if (md.description.size() > 0)
                  ImGui::Text("  description: %s", md.description.c_str());

                imgui::generate_ui(md.type_metadata.generate_default_value(), md.type_metadata);
                ImGui::Separator();
              }
            }
          }
          ImGui::End();
          if (ImGui::Begin("Conf", nullptr, 0))
          {
            if (resource_ctx_conf.is_loaded())
            {
              const raw_data& initial_rd = resource_ctx_conf.conf_data;
              raw_data new_rd = imgui::generate_ui(initial_rd, rle::deserialize<rle::serialization_metadata>(resource_ctx_conf.get_hconf_metadata()));
              if (!raw_data::is_same(initial_rd, new_rd))
              {
                resource_ctx_conf.conf_data = std::move(new_rd);
                cctx->hconf.write_conf(resource_ctx_conf);
              }
            }
            else
            {
              ImGui::Text("Loading...");
            }
          }
          ImGui::End();
          if (ImGui::Begin("Controls", nullptr, 0))
          {
            if (ImGui::Button("Force Repack Everything", ImVec2(-1, 0)))
            {
              pck->packer_options.force = true;
            }

            ImGui::Separator();
            ImGui::Text("io: in flight: %u, pending: %u", cctx->io.get_in_flight_operations_count(), cctx->io.get_pending_operations_count());
            ImGui::Text("io: read: %.3f kb/s, write: %.3f kb/s", last_average_read_rate/1000, last_average_write_rate/1000);
            ImGui::Text("tm: pending tasks: %u", cctx->tm.get_pending_tasks_count());
            ImGui::Text("tm: running tasks: %u", cctx->tm.get_running_tasks_count());
            ImGui::Text("frametime: %.3f ms framerate: %.3f fps", last_average_frametime * 1000, 1.0f / last_average_frametime);
            ImGui::Text("framecount: %u", frame_cnt);
            ImGui::Separator();
            if (pck->is_packing())
              ImGui::Text("state: packing in progress");
            else if (glfw_mod->is_app_focused())
              ImGui::Text("state: idle / in focus");
            else
              ImGui::Text("state: idle / low-framerate (app not in focus)");
          }
          ImGui::End();

          if (ImGui::Begin("Resource Messages", nullptr, ImGuiWindowFlags_AlwaysHorizontalScrollbar))
          {
            if (selected_res.empty())
            {
              ImGui::TextUnformatted("No resource selected");
            }
            else
            {
              ImGui::TextUnformatted("file ");
              ImGui::SameLine();
              imgui::link("file://" + (std::string)(hctx->res.source_folder / selected_res.c_str()), selected_res.c_str());
              ImGui::SameLine();
              ImGui::TextUnformatted(": ");
              ImGui::SameLine();
              imgui::link("file://" + (std::string)(hctx->res.source_folder / selected_res.c_str()).remove_filename(), "[open folder]");

              ImGui::Indent();
              const auto& db = cctx->res.get_db();
              const std::set<id_t> subres = db.get_resources(selected_res, true);
              for (const id_t it : subres)
              {
                auto msg_list = db.get_messages(it).list;

                // remove debug entries:
                msg_list.erase(std::find_if(msg_list.begin(), msg_list.end(), [](const auto & x) { return x.severity == cr::logger::severity::debug;}), msg_list.end());

                if (!msg_list.empty())
                {
                  ImGui::Text("sub-resource: %s:", cctx->res.resource_name(it).c_str());
                  ImGui::Indent();
                  for (auto& msg : msg_list)
                  {
                    if (msg.severity == cr::logger::severity::error)
                      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.05f, 0.00f, 1.0f));
                    if (msg.severity == cr::logger::severity::warning)
                      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.72f, 0.00f, 1.0f));
                    if (msg.severity == cr::logger::severity::debug)
                      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.50f, 1.0f));

                    ImGui::TextUnformatted(msg.message.c_str());

                    if (msg.severity == cr::logger::severity::error
                      || msg.severity == cr::logger::severity::warning
                      || msg.severity == cr::logger::severity::debug)
                      ImGui::PopStyleColor();
                  }
                  ImGui::Unindent();
                }
              }
              ImGui::Unindent();
            }
          }
          ImGui::End();

          if (ImGui::Begin("Packer", nullptr, 0))
          {
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

            if (ImGui::BeginChildFrame(ImGui::GetID("log frame"), ImVec2(-1, -1), ImGuiWindowFlags_AlwaysHorizontalScrollbar))
            {
              std::lock_guard _l(res_lock);
              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.05f, 0.00f, 1.0f));
              for (auto& it : resources_with_errors)
              {
                if (ImGui::Selectable(it.c_str(), it == selected_res))
                  selected_res = it;
              }
              ImGui::PopStyleColor();

              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.72f, 0.00f, 1.0f));
              for (auto& it : resources_with_warnings)
              {
                if (ImGui::Selectable(it.c_str(), it == selected_res))
                  selected_res = it;
              }
              ImGui::PopStyleColor();

              if (!resources_in_progress.empty())
              {
                ImGui::Separator();
                ImGui::Text("Packing %u resources...", (uint32_t)resources_in_progress.size());
                ImGui::BeginDisabled();
                for (auto& it : resources_in_progress)
                {
                  ImGui::Text("packing %s...", it.c_str());
                }
                ImGui::EndDisabled();
              }
            }
            ImGui::EndChildFrame();
          }
          ImGui::End();

          if (window_state.win->should_close())
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
            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
              window_state.win->should_close(false);
              ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();

            ImGui::SameLine();

            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 100);
            if (ImGui::Button("Exit", ImVec2(100, 0)))
            {
              is_quiting = true;
            }
            ImGui::EndPopup();
          }
        });

        auto* renderer = engine->get_module<renderer_module>("renderer"_rid);
        renderer->min_frame_time = 0.016f;

        on_render_start_tk = renderer->on_render_start.add([this, pck, renderer, cm, glfw_mod]
        {
          cctx->tm.get_task([this, pck, renderer, cm, glfw_mod]
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


            if (is_quiting)
            {
              engine->sync_teardown();
              is_quiting = false;
            }
            else
            {
              // window:
              if (pck->is_packing())
              {
                // max fps should be 60, but we don't want to sleep, so we just "skip" frames instead.
                renderer->min_frame_time = 0.033f;
                cm->min_frame_length = std::chrono::milliseconds{0};
                glfw_mod->wait_for_events(false);
              }
              else
              {
                renderer->min_frame_time = 0.0f;
                cm->min_frame_length = std::chrono::milliseconds{16};
                glfw_mod->wait_for_events(false);
              }
            }

            if (!is_setup && window_state.win)
              window_state.win->iconify();
          });
        });
      }

      void on_engine_boot_complete() override
      {
        cctx->tm.get_long_duration_task([this]
        {
          load_conf_for_ui();
          check_for_resources();
        });
      }

      void on_shutdown() override
      {
        on_packing_started_tk.release();
        on_resource_queued_tk.release();
        on_resource_packed_tk.release();
        on_index_saved_tk.release();
        on_packing_ended_tk.release();
        on_item_selected_tk.release();
        on_render_start_tk.release();

        resource_ctx_conf.remove_watch();
        window_state = {};
      }

    private:
      void load_conf_for_ui()
      {
        cctx->hconf.read_conf
        (
          resource_ctx_conf,
         resources::resource_configuration::default_source,
         (std::string)resources::resource_configuration::default_source.view()
        ).then([this](bool success)
        {
          if (!success)
          {
            cctx->tm.get_delayed_task(std::chrono::seconds{1}, [this]
            {
              load_conf_for_ui();
            });
          }
        });
      }

      void set_window_icon()
      {
        if (!window_state.win)
          return;

        if (icon_state == current_state)
          return;
        icon_state = current_state;
        glm::u8vec4 color = {0, 0, 0, 0};

        if ((icon_state & packer_state_t::has_error) != packer_state_t::none)
            color = { 0xe0, 0x34, 0x00, 0x00 };
        else if ((icon_state & packer_state_t::has_warnings) != packer_state_t::none)
            color = { 0xe0, 0xbb, 0x00, 0x00 };
        else if (icon_state != packer_state_t::none)
            color = { 0xff, 0xff, 0xff, 0x00 };

        if ((icon_state & packer_state_t::idle) != packer_state_t::none)
          color /= 2;

        window_state.win->_set_hydra_icon(*reinterpret_cast<uint32_t*>(&color));
      }

      void setup_window()
      {
        cr::out().debug("found imgui shaders, creating application main window/render-context...");
        auto* imgui = engine->get_module<imgui::imgui_module>("imgui"_rid);
        is_setup = true;
        window_state.win->show();
        hctx->shmgr.refresh();
      }

      void check_for_resources()
      {
        async::multi_chain<bool>
        (
          true,
          cr::construct<std::vector>(cctx->res.has_resource(imgui_vs_rid), cctx->res.has_resource(imgui_fs_rid)),
          [](bool & state, bool res) { state = state && res; }
        )
        .then([this](bool can_launch)
        {
          if (can_launch /*&& current_state == packer_state_t::idle*/)
          {
            return setup_window();
          }
          else
          {
            // finish by launching the same task, again
            cctx->tm.get_delayed_task(std::chrono::milliseconds{120}, [this]
            {
              // TODO: only when a resource has been packed ?
              check_for_resources();
            });
          }
        });
      }

      void on_resource_queued(const std::filesystem::path& res)
      {
        std::lock_guard _l(res_lock);
        resources_in_progress.emplace(res);
      }

      void on_resource_packed(const std::filesystem::path& res, resources::status st)
      {
        std::lock_guard _l(res_lock);
        resources_in_progress.erase(res);

        if (st == resources::status::failure)
        {
          current_state |= packer_state_t::has_error;
          resources_with_errors.emplace(res);
          resources_with_warnings.erase(res);
        }
        else if (st == resources::status::partial_success)
        {
          resources_with_errors.erase(res);
          resources_with_warnings.emplace(res);
          current_state |= packer_state_t::has_warnings;
        }
        else
        {
          resources_with_errors.erase(res);
          resources_with_warnings.erase(res);
        }
        if (resources_with_errors.empty())
          current_state &= ~packer_state_t::has_error;
        if (resources_with_warnings.empty())
          current_state &= ~packer_state_t::has_warnings;
        set_window_icon();
      }

      void on_index_saved(resources::status st)
      {
        // reload shaders / fonts:
        auto* imgui = engine->get_module<imgui::imgui_module>("imgui"_rid);
        imgui->reload_fonts();
      }

      void on_packing_started(uint32_t /* modified */, uint32_t /* indirect_mod */,  uint32_t /* added */, uint32_t /* to_remove */)
      {
        current_state &= ~packer_state_t::idle;
        current_state |= packer_state_t::packing;
        set_window_icon();
      }

      void on_packing_ended()
      {
        current_state &= ~packer_state_t::packing;
        current_state |= packer_state_t::idle;
        set_window_icon();
      }

    private:
      cr::chrono chrono;
      uint32_t frame_cnt = 0;
      uint64_t initial_read_bytes = 0;
      uint64_t initial_written_bytes = 0;
      float last_average_frametime = 0;
      float last_average_read_rate = 0;
      float last_average_write_rate = 0;

      spinlock res_lock;
      std::set<std::filesystem::path> resources_in_progress;
      std::set<std::filesystem::path> resources_with_errors;
      std::set<std::filesystem::path> resources_with_warnings;
      std::filesystem::path selected_res;

      rle::serialization_metadata rel_db_metadata = rle::generate_metadata<resources::rel_db>();

      glfw::window_state_t window_state;

      cr::event_token_t on_packing_started_tk;
      cr::event_token_t on_resource_queued_tk;
      cr::event_token_t on_resource_packed_tk;
      cr::event_token_t on_index_saved_tk;
      cr::event_token_t on_packing_ended_tk;

      cr::event_token_t on_item_selected_tk;

      cr::event_token_t on_render_start_tk;

      bool is_setup = false;
      bool is_quiting = false;

      packer_state_t icon_state = packer_state_t::none;
      packer_state_t current_state = packer_state_t::idle;

      friend class engine_t;
      friend engine_module<packer_ui_module>;
  };
}
