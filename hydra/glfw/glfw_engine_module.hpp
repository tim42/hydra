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
  class glfw_module final : private engine_module<glfw_module>
  {
    public:
      struct state_ref_t
      {
        glfw::window& window;

        pass_manager& pm;
        glfw_module& mod;

        render_context_t& _ctx_ref;

        cr::event<> on_context_destroyed;

        bool only_render_on_event = false;

        ~state_ref_t()
        {
          on_context_destroyed();
          mod._request_removal(*this);
        }
      };

      template<typename... Args>
      std::unique_ptr<state_ref_t> create_window(Args&&... args)
      {
        auto& renderer = *engine->get_module<renderer_module>("renderer"_rid);

        std::unique_ptr<render_context_ref_t<state_t>> st_ref = renderer.create_render_context<state_t>(this, std::forward<Args>(args)...);
        std::unique_ptr<state_ref_t> r {new state_ref_t{(*st_ref)->window, (*st_ref)->pm, *this, **st_ref}};

        (*st_ref)->glfw_ref = r.get();
        (*st_ref)->window.set_size((glm::uvec2)((glm::vec2)(*st_ref)->window.get_size() * (*st_ref)->window.get_content_scale()));

        std::lock_guard _l(state_change_lock);
        states_to_add.push_back(std::move(st_ref));
        return r;
      }

      // automatically called on destruction of state_ref_t
      void _request_removal(state_ref_t& ref)
      {
        std::lock_guard _l(state_change_lock);
        states_to_remove.push_back(&ref);
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


    private:
      glfw_module();
      ~glfw_module();

      static constexpr const char* module_name = "glfw";

      static bool is_compatible_with(runtime_mode m);

      void init_vulkan_interface(gen_feature_requester& gfr, bootstrap& /*hydra_init*/) override;

      bool filter_queue(vk::instance& instance, int/*VkQueueFlagBits*/ queue_type, size_t qindex, const neam::hydra::vk::physical_device& gpu) override;

      void add_task_groups(threading::task_group_dependency_tree& tgd) override;
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override;

      void on_context_initialized() override;

      void on_shutdown_post_idle_gpu() override;
      void on_shutdown() override;

    private:
      void init_cursors();
      void destroy_cursors();

      void execute_on_main_thread(threading::function_t&& fnc) const;
      void assert_is_main_thread() const;

    private:
      struct state_t final : public render_context_t
      {
        template<typename... Args>
        state_t(hydra_context& _hctx, Args&&... args)
         : render_context_t(_hctx)
         , window(_hctx, std::forward<Args>(args)...)
         , image_ready(hctx.device, nullptr)
         , render_finished(hctx.device, nullptr)
        {
          output_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        ~state_t()
        {
          hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.gqueue), std::move(render_finished));
        }

        glfw::window window;

        // vk stuff:
        vk::semaphore image_ready;
        vk::semaphore render_finished;

        state_ref_t* glfw_ref;

        glm::uvec2 new_size {0, 0};
        uint32_t framebuffer_index = 0;
        bool need_reset = true;

        spinlock swapchain_lock;

        std::vector<VkFormat> get_framebuffer_format() const final override
        {
          return { window.get_swapchain().get_image_format(), };
        }

        void begin() final override
        {
          new_size = window.get_framebuffer_size();
          if (need_reset || size != new_size)
            refresh();

          image_ready = vk::semaphore{ hctx.device, "glfw::state_t::image_ready" };
          // std::lock_guard _l(hctx.gqueue.queue_lock);
          // std::lock_guard _l(window.get_swapchain().lock);
          framebuffer_index = window.get_swapchain().get_next_image_index(image_ready, std::numeric_limits<uint64_t>::max(), &need_reset);
        }

        void pre_render(vk::submit_info& si)  final override
        {
          si.wait(image_ready, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
          hctx.dfe.defer_destruction(hctx.dfe.queue_mask(*si.get_current_queue()), std::move(image_ready));
        }

        void post_render(vk::submit_info& si) final override
        {
          hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.gqueue), std::move(render_finished));
          render_finished = vk::semaphore{ hctx.device, "glfw::state_t::render_finished" };
          si.signal(render_finished);
        }

        void post_submit() final override
        {
          bool recreate = false;
          hctx.gqueue.present(hctx.dqe, window.get_swapchain(), framebuffer_index, { &render_finished }, &recreate);
          need_reset = need_reset || recreate;
          hctx.dqe.defer_execution(hctx.gqueue.queue_id, [this]()
          {
//          framebuffer_index = window.get_swapchain().get_next_image_index(image_ready, std::numeric_limits<uint64_t>::max(), &need_reset);
          });
        }

        void end() final override
        {
          if (need_reset)
            refresh();
        }

        std::vector<vk::image*> get_images() final override { return { &window.get_swapchain().get_image_vector()[framebuffer_index] }; }
        std::vector<vk::image_view*> get_images_views() final override { return { &window.get_swapchain().get_image_view_vector()[framebuffer_index] }; }

        void refresh()
        {
          need_reset = false;
          size = window.get_framebuffer_size();
          new_size = size;
          // refresh without waiting:
          std::lock_guard _l(hctx.gqueue.queue_lock);
          hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.gqueue), window.get_swapchain().recreate_swapchain(size));
        }
      };

    private:
      mutable spinlock state_lock;
      std::mtc_vector<std::unique_ptr<render_context_ref_t<state_t>>> states;

      mutable spinlock state_change_lock;
      std::mtc_vector<std::unique_ptr<render_context_ref_t<state_t>>> states_to_add;
      std::mtc_vector<state_ref_t*> states_to_remove;

      bool was_focused = true;
      bool has_any_window_ready = false;
      bool has_contexts_needing_render = false;
      bool should_wait_for_events = false;
      uint32_t frames_since_any_event = ~0u;
      static constexpr uint32_t k_max_frame_to_render_after_event = 2;

      cr::event_token_t on_render_start_tk;


      GLFWcursor* cursors[(uint32_t)cursor::_count] = {nullptr};

      static std::atomic<uint32_t> s_module_count;

      friend engine_t;
      friend engine_module<glfw_module>;
      friend class renderer_module;
      friend class window;
  };
}

