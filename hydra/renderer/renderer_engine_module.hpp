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

#include <hydra/engine/engine_module.hpp>
#include <hydra/engine/hydra_context.hpp>
#include <ntools/threading/threading.hpp>
#include <ntools/id/string_id.hpp>
#include <ntools/chrono.hpp>

#include "ecs/universe.hpp"

namespace neam::hydra
{
  namespace renderer::internals
  {
    class gpu_task_order;
  }

  /// \brief Manages the render task group + VRD
  class renderer_module final : public engine_module<renderer_module>
  {
    public: // conf:
      float min_frame_time = 0.005f; // in s. Set to 0 to remove.

    public: // render contexts:
      ecs::entity create_render_entity();

    public: // render task group API:
      cr::event<> on_render_start;
      cr::event<> on_render_end;

    private: // module interface:
      static constexpr neam::string_t module_name = "renderer";

      static bool is_compatible_with(runtime_mode m)
      {
        // we only need a full hydra context:
        // (the renderer works for passive/offscreen, as long as there's a full hydra context)
        if ((m & runtime_mode::hydra_context) != runtime_mode::hydra_context)
          return false;
        return true;
      }

      void init_vulkan_interface(gen_feature_requester& gfr, bootstrap& /*hydra_init*/) override;

      void add_named_threads(threading::threads_configuration& tc) override;
      void add_task_groups(threading::task_group_dependency_tree& tgd) override;
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override;

      void on_context_set() override;
      void on_context_initialized() override;

      void on_shutdown_post_idle_gpu() override;
      void on_start_shutdown() override;

    private: // internal api:
      void prepare_renderer();

    private:
      std::unique_ptr<ecs::universe> universe;
      renderer::internals::gpu_task_order* task_order = nullptr;

      cr::chrono chrono;
      bool skip_frame = false;

      cr::event_token_t on_frame_start_tk;
      // FIXME?
      //render_pass_dependencies_builder render_pass_dependencies;

      friend class engine_t;
      friend engine_module<renderer_module>;
  };
}

