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
#include <hydra/engine/engine.hpp>
#include <hydra/engine/vk_context.hpp>
#include <hydra/init/feature_requesters/gen_feature_requester.hpp>
#include <hydra/renderer/renderer_engine_module.hpp>

#include <ntools/mt_check/vector.hpp>

#include "glfw.hpp"
#include "glfw_window.hpp"
#include "glfw_events.hpp"

namespace neam::hydra::glfw
{
  struct window_state_t
  {
    ecs::entity render_entity;
    std::unique_ptr<window> win;
  };

  class glfw_module final : private engine_module<glfw_module>
  {
    public:
      template<typename... WinArgs>
      window_state_t create_window(WinArgs&&... win_args)
      {
        std::unique_ptr<window> win { new window(*hctx, this, std::forward<WinArgs>(win_args)...) };
        ecs::entity entity = create_render_entity(*win.get());
        return
        {
          std::move(entity),
          std::move(win),
        };
      }

      /// \brief Check if any of the windows created by this module is focused
      bool is_app_focused() const
      {
        return was_focused;
      }

      bool need_render() const { return has_contexts_needing_render; }

      /// \brief Return whether there is any window that are valid
      /// \note if this is false, functions that check the state of the app are invalid (like is_app_focused)
      bool has_any_window() const
      {
        return has_any_window_ready;
      }

      /// \brief Instead of polling event, wait for them
      void wait_for_events(bool should) { should_wait_for_events = should; }
      bool get_wait_for_events() const { return should_wait_for_events; }

    private:
      glfw_module();
      ~glfw_module();

      static constexpr neam::string_t module_name = "glfw";

      static bool is_compatible_with(runtime_mode m);

      void init_vulkan_interface(gen_feature_requester& gfr, bootstrap& /*hydra_init*/) override;

      bool filter_queue(vk::instance& instance, int/*VkQueueFlagBits*/ queue_type, size_t qindex, const neam::hydra::vk::physical_device& gpu) override;

      void add_task_groups(threading::task_group_dependency_tree& tgd) override;
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override;

      void on_context_initialized() override;

      void on_shutdown_post_idle_gpu() override;
      void on_shutdown() override;

      ecs::entity create_render_entity(window& win);

    private:
      void init_cursors();
      void destroy_cursors();

      void execute_on_main_thread(threading::function_t&& fnc) const;
      void assert_is_main_thread() const;

    private:
      bool was_focused = true;
      bool has_any_window_ready = false;
      bool has_contexts_needing_render = false;
      bool should_wait_for_events = false;
      uint32_t frames_since_any_event = ~0u;
      static constexpr uint32_t k_max_frame_to_render_after_event = 2;

      GLFWcursor* cursors[(uint32_t)cursor::_count] = {nullptr};

      static std::atomic<uint32_t> s_module_count;

      friend engine_t;
      friend engine_module<glfw_module>;
      friend class renderer_module;
      friend class window;
  };
}

