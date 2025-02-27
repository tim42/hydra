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
#include <ntools/tracy.hpp>
#include <ntools/mt_check/vector.hpp>

#include "imgui_context.hpp"

namespace neam::hydra::imgui
{
  class imgui_module final : public engine_module<imgui_module>
  {
    public: // public interface:
      imgui_context& get_imgui_context() { return *context; };
      const imgui_context& get_imgui_context() const { return *context; };

      void register_function(id_t fid, std::function<void()> func);
      void unregister_function(id_t fid);

      void create_context(glfw::window_state_t& ws);

      void reload_fonts();

    public: // module interface:
      static constexpr neam::string_t module_name = "imgui";

      static bool is_compatible_with(runtime_mode m);

      void add_task_groups(threading::task_group_dependency_tree& tgd) override;
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override;

      void on_context_initialized() override;

      void on_resource_index_loaded() override;

      void on_shutdown_post_idle_gpu() override;


    private:
      spinlock lock;
      std::optional<imgui_context> context;

      std::mtc_vector<std::pair<id_t, std::function<void()>>> functions;

      bool res_have_loaded = false;
      bool has_loaded_conf = false;

      friend class engine_t;
      friend engine_module<imgui_module>;
  };
}

