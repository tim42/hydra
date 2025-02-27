//
// created by : Timothée Feuillet
// date: 2/15/25
//
//
// Copyright (c) 2025 Timothée Feuillet
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

#include "../../ecs/ecs.hpp"
#include "../../renderer/ecs/gpu_task_producer.hpp"
#include "glfw/glfw_window.hpp"

namespace neam::hydra::glfw::components
{
  class prologue : public ecs::internal_component<prologue>, renderer::concepts::gpu_task_producer::concept_provider<prologue>
  {
    public:
      static constexpr renderer::order_mode order = renderer::order_mode::forced_prologue;

    public:
      prologue(param_t p, hydra_context& _hctx, window& _win)
        : internal_component_t(p)
        , gpu_task_producer_provider_t(*this, _hctx)
        , win(_win)
      {}

    protected:
      struct setup_state_t
      {
        vk::semaphore image_ready;

        bool is_valid = false;
        bool need_reset = true;
        bool has_framebuffer_index = false;
        uint32_t framebuffer_index = 0;
        glm::uvec2 size;
      };

      setup_state_t setup(renderer::gpu_task_context& gtctx)
      {
        return { vk::semaphore{ hctx.device, "glfw::prologue::image_ready" } };
      }

      // Skip rendering if the window is not yet fully initialized
      bool should_skip() const { return !win.is_window_ready() || !win.get_swapchain()._get_vk_swapchain(); }

      void acquire_next_image(setup_state_t& setup_state)
      {
        hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.gqueue), std::move(setup_state.image_ready));
        setup_state.image_ready = vk::semaphore{ hctx.device, "glfw::prologue::image_ready" };
        setup_state.framebuffer_index = win.get_swapchain().get_next_image_index(setup_state.image_ready, std::numeric_limits<uint64_t>::max(), &setup_state.need_reset);
        setup_state.has_framebuffer_index = true;
      }

      void prepare(renderer::gpu_task_context& gtctx, setup_state_t& setup_state)
      {
        glm::uvec2 new_size = win.get_framebuffer_size();
        if (setup_state.need_reset || setup_state.size != new_size)
        {
          setup_state.has_framebuffer_index = false;
          refresh(setup_state);
        }
        setup_state.is_valid = true;
        if (!setup_state.has_framebuffer_index)
        {
          acquire_next_image(setup_state);
        }
        setup_state.has_framebuffer_index = false;

        set_viewport_context(renderer::viewport_context
        {
          win.get_swapchain().get_dimensions(),
          {0, 0},
          win.get_swapchain().get_full_rect2D(),
          win.get_swapchain().get_full_viewport(),
        });
        export_resource(renderer::k_context_final_output, renderer::exported_image
        {
          win.get_swapchain().get_image_vector()[setup_state.framebuffer_index],
          win.get_swapchain().get_image_view_vector()[setup_state.framebuffer_index],
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // swapchain images are in undefined layout
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        }, renderer::export_mode::constant);
      }

      void submit(renderer::gpu_task_context& gtctx, vk::submit_info& si, setup_state_t& setup_state)
      {
        si.on(hctx.gqueue);
        si.wait(setup_state.image_ready, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        hctx.dfe.defer_destruction(hctx.dfe.queue_mask(*si.get_current_queue()), std::move(setup_state.image_ready));

        hydra::vk::command_buffer cmd_buf = hctx.gcpm.get_pool().create_command_buffer();
        {
          hydra::vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

          renderer::exported_image backbuffer
          {
            win.get_swapchain().get_image_vector()[setup_state.framebuffer_index],
            win.get_swapchain().get_image_view_vector()[setup_state.framebuffer_index],
            VK_IMAGE_LAYOUT_UNDEFINED, // swapchain images are in undefined layout
            VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
          };
          pipeline_barrier(cbr, backbuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
          begin_rendering(cbr, backbuffer, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
          cbr.end_rendering();
        }
        cmd_buf.end_recording();

        si.on(hctx.gqueue).execute(cmd_buf);
        hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.gqueue), std::move(cmd_buf));
      }

      void refresh(setup_state_t& setup_state)
      {
        setup_state.need_reset = false;
        setup_state.size = win.get_framebuffer_size();
        // refresh without waiting:
        std::lock_guard _l(hctx.gqueue.queue_lock);
        hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.gqueue), win.get_swapchain().recreate_swapchain(setup_state.size));
      }
    private:
      window& win;

      friend gpu_task_producer_provider_t;
      friend class epilogue;
      friend glfw_module;
  };
}
