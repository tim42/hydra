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

#include "glfw_prologue.hpp"
#include "../../ecs/ecs.hpp"
#include "../../renderer/ecs/gpu_task_producer.hpp"

namespace neam::hydra::glfw::components
{
  class epilogue : public ecs::sync_component<epilogue>, renderer::concepts::gpu_task_producer::concept_provider<epilogue>
  {
    public:
      static constexpr renderer::order_mode order = renderer::order_mode::forced_epilogue;

    public:
      epilogue(param_t p, hydra_context& _hctx, window& _window)
        : sync_component_t(p)
        , gpu_task_producer_provider_t(*this, _hctx)

        , prologue_comp(require<prologue>(_hctx, _window))
      {}

      void present()
      {
        if (!prologue_comp.should_skip() && has_setup_state() && prologue_comp.has_setup_state())
        {
          auto* st = get_setup_state();
          auto* pst = prologue_comp.get_setup_state();
          if (pst->is_valid && st->can_present)
          {
            st->can_present = false;
            bool recreate = false;
            hctx.gqueue.present(hctx.dqe, prologue_comp.win.get_swapchain(), pst->framebuffer_index, { &st->render_finished }, &recreate);
            pst->need_reset = pst->need_reset || recreate;
          }
        }
      }

      void acquire_next_image()
      {
        if (!prologue_comp.should_skip() && prologue_comp.has_setup_state())
        {
          auto* pst = prologue_comp.get_setup_state();

          if (!pst->has_framebuffer_index)
          {
            // avoid double acquiring images
            prologue_comp.acquire_next_image(*pst);
          }
        }
      }

    protected:
      struct setup_state_t
      {
        vk::semaphore render_finished;
        bool can_present = false;
      };

      struct prepare_state_t
      {
        renderer::exported_image image;
      };

      setup_state_t setup(renderer::gpu_task_context& gtctx)
      {
        return { vk::semaphore{ hctx.device, "glfw::epilogue::render_finished" } };
      }

      prepare_state_t prepare(renderer::gpu_task_context& gtctx, setup_state_t&)
      {
        renderer::exported_image image = import_image(renderer::k_context_final_output, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

        return { image };
      }

      void submit(renderer::gpu_task_context& gtctx, vk::submit_info& si, setup_state_t& setup_state, prepare_state_t& prepare_state)
      {
        check::debug::n_assert(setup_state.can_present == false, "glfw::epilogue::submit: missing call to present for last frame");
        setup_state.can_present = true;

        // transition to present:
        neam::hydra::vk::command_buffer frame_command_buffer = hctx.gcpm.get_pool().create_command_buffer();
        frame_command_buffer._set_debug_name("glfw::epilogue::framebuffer-transition");
        {
          neam::hydra::vk::command_buffer_recorder cbr = frame_command_buffer.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
          neam::hydra::vk::cbr_debug_marker _dm(cbr, "glfw::epilogue::framebuffer-transition");
          pipeline_barrier(cbr, prepare_state.image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        }
        frame_command_buffer.end_recording();

        si.on(hctx.gqueue).execute(frame_command_buffer);
        hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.gqueue), std::move(frame_command_buffer));

        // setup the semaphore:
        hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.gqueue), std::move(setup_state.render_finished));
        setup_state.render_finished = vk::semaphore{ hctx.device, "glfw::epilogue::render_finished" };
        si.signal(setup_state.render_finished);
      }

    private:
      prologue& prologue_comp;

      friend gpu_task_producer_provider_t;
      friend glfw_module;
  };
}