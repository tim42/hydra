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
#include <ntools/event.hpp>
#include <ntools/ct_string.hpp>

#include <hydra/hydra_debug.hpp>
#include <hydra/engine/engine.hpp>
#include <hydra/engine/engine_module.hpp>
#include <hydra/engine/core_modules/core_module.hpp>
#include <hydra/glfw/glfw_engine_module.hpp>  // Add the glfw module
#include <hydra/imgui/imgui_engine_module.hpp>  // Add the imgui module
#include <hydra/imgui/utilities/imgui_folder_view.hpp>  // Add the folder view
#include <hydra/renderer/renderer_engine_module.hpp>
#include <hydra/imgui/generic_ui.hpp>

#include <imgui_internal.h> // for dockbuilder stuff

#include <fmt/chrono.h>

#include "options.hpp"

namespace neam::hydra
{
  class config_editor_module : public engine_module<config_editor_module>
  {
    public: // options
      global_options options;

    public: // API

    private:
      static constexpr neam::string_t module_name = "config_editor_module";

      static bool is_compatible_with(runtime_mode m)
      {
        // necessary stuff:
        if ((m & runtime_mode::hydra_context) == runtime_mode::none)
          return false;
        if ((m & runtime_mode::offscreen) != runtime_mode::none)
          return false;

        return true;
      }

      void on_context_initialized() override
      {
        auto* core = engine->get_module<core_module>("core"_rid);
        auto* renderer = engine->get_module<renderer_module>("renderer"_rid);
        auto* glfw_mod = engine->get_module<glfw::glfw_module>("glfw"_rid);
        auto* imgui = engine->get_module<imgui::imgui_module>("imgui"_rid);

        core->min_frame_length = std::chrono::milliseconds{16};
        glfw_mod->wait_for_events(false);

        on_frame_end_tk = core->on_frame_end.add(cctx->tm, [this, core, glfw_mod]
        {
          bool wait_for_events = false;
          if (options.parameters.size() == resources.size())
          {
            wait_for_events = true;
            for (auto& it : resources)
            {
              if (it.loading || !it.is_init || it.last_edit != std::chrono::time_point<std::chrono::system_clock>{})
              {
                wait_for_events = false;
                break;
              }
            }
          }
          glfw_mod->wait_for_events(wait_for_events);

          if (glfw_mod->is_app_focused() || !wait_for_events)
          {
            cctx->unstall_all_threads();
            core->min_frame_length = std::chrono::milliseconds{16};
          }
          else
          {
            core->min_frame_length = std::chrono::milliseconds{33};
            cctx->stall_all_threads_except(1);
          }

          if (window_state.win && window_state.win->should_close())
          {
            // save all changed files:
            std::vector<async::continuation_chain> chains;
            for (uint32_t i = 0; i < (uint32_t)resources.size(); ++i)
            {
              // TODO: Check auto-save on change + if false show a dialogue
              chains.push_back(save_conf(options.parameters[i], i).to_continuation());
            }

            async::multi_chain(std::move(chains)).then([this]
            {
              engine->sync_teardown();
            });
          }
        });

        window_state = glfw_mod->create_window(glm::uvec2{800, 800}, "HYDRA CONFIG EDITOR");

        imgui->create_context(window_state);
        imgui->register_function("dockspace"_rid, []()
        {
          const ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

          ImGui::DockBuilderDockWindow("Resource", dockspace_id);

          ImGui::DockBuilderFinish(dockspace_id);
        });

        imgui->register_function("Resource"_rid, [this]
        {
          if (ImGui::Begin("Resource", nullptr, 0))
          {
            if (options.parameters.size() > 0)
            {
                imgui_render_single_conf_file(options.parameters[0], 0);
            }
          }
          ImGui::End();
        });
      }

      void imgui_render_single_conf_file(std::string_view path, uint32_t data_index)
      {
        if (resources.size() <= data_index)
          resources.resize(data_index + 1);

        res_state_t& data = resources[data_index];

        if (!data.is_init)
        {
          data.is_init = true;
          data.on_conf_reload_tk = data.conf.hconf_on_data_changed.add([this, data_index]
          {
            res_state_t& data = resources[data_index];
            data.last_load = std::chrono::system_clock::now();
          });
          cctx->hconf.read_conf(data.conf, string_id::_runtime_build_from_string(path), (std::string)path).then([data_index, this](bool res)
          {
            res_state_t& data = resources[data_index];
            data.loading = false;
            data.failed = !res;
          });
        }

        const float footer_size = ImGui::GetFontSize() * 3.25f;
        if (ImGui::BeginChild("##res", {0, -footer_size}))
        {
          if (data.loading)
          {
            ImGui::TextFmt("Loading {}...", path);
            ImGui::EndChild();
            return;
          }
          if (data.failed)
          {
            ImGui::TextFmt("Failed to load {}.", path);
            ImGui::EndChild();
            return;
          }

          // render the UI:
          raw_data conf_data = std::move(data.conf.conf_data);
          data.conf.conf_data = imgui::generate_ui(conf_data, data.conf.get_hconf_metadata());
          if (auto_save_on_change && !raw_data::is_same(conf_data, data.conf.conf_data))
          {
            data.last_edit = std::chrono::system_clock::now();
            data.changed = true;
          }

          if (data.last_edit != std::chrono::time_point<std::chrono::system_clock> {} && data.last_edit <= std::chrono::system_clock::now() - std::chrono::seconds{3})
            save_conf(path, data_index);
        }
        ImGui::EndChild();

        imgui_render_footer(path, data_index);
      }

      void imgui_render_footer(std::string_view path, uint32_t data_index)
      {
        if (ImGui::BeginChild("##footer"))
        {
          res_state_t& data = resources[data_index];

          ImGui::Separator();
          if (data.last_save != std::chrono::time_point<std::chrono::system_clock>{})
            ImGui::TextFmt("Loaded at: {:%Y-%m-%d %X}   |   Saved at: {:%Y-%m-%d %X}", std::chrono::round<std::chrono::seconds>(data.last_load), std::chrono::round<std::chrono::seconds>(data.last_save));
          else
            ImGui::TextFmt("Loaded at: {:%Y-%m-%d %X}   |   Not yet saved", std::chrono::round<std::chrono::seconds>(data.last_load));

          bool do_save;
          ImGui::Separator();
          {
            ImGui::BeginDisabled(!data.changed);
            do_save = ImGui::Button("Save"); ImGui::SameLine();
            ImGui::EndDisabled();
          }
          if (!auto_save_on_change)
          {
            if (ImGui::Button("Reload"))
              cctx->hconf.reload_conf(data.conf);
            ImGui::SameLine();
          }
          ImGui::TextUnformatted("   |   "); ImGui::SameLine();
          ImGui::TextUnformatted("Auto save on change:"); ImGui::SameLine(); ImGui::Checkbox("##asoc", &auto_save_on_change);
          if (do_save)
            save_conf(path, data_index);
        }
        ImGui::EndChild();
      }

      async::chain<bool> save_conf(std::string_view path, uint32_t data_index)
      {
        res_state_t& data = resources[data_index];
        if (!data.changed)
          return async::chain<bool>::create_and_complete(false);
        data.changed = false;
        data.last_save = std::chrono::system_clock::now();
        data.last_edit = {};

        // force a re-serialization of the metadata, to avoid stale metadataL
        data.conf._set_conf_metadata(rle::serialize(rle::deserialize<rle::serialization_metadata>(data.conf.get_hconf_metadata())));

        return cctx->hconf.write_conf(data.conf);
      }

      void on_start_shutdown() override
      {
      }

      void on_shutdown() override
      {
        on_frame_end_tk.release();

        window_state = {};
      }

    private: // state
      glfw::window_state_t window_state;

      struct res_state_t
      {
        hydra::conf::gen_conf conf;
        cr::event_token_t on_conf_reload_tk;

        std::chrono::time_point<std::chrono::system_clock> last_edit;
        std::chrono::time_point<std::chrono::system_clock> last_save;
        std::chrono::time_point<std::chrono::system_clock> last_load;

        bool changed = false;
        bool is_init = false;
        bool loading = true;
        bool failed = false;
      };

      std::vector<res_state_t> resources;
      bool auto_save_on_change = true;

      cr::event_token_t on_frame_end_tk;
      friend class engine_t;
      friend engine_module<config_editor_module>;
  };
}

