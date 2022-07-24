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

#include <GLFW/glfw3.h>

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
        events::glfw::manager& emgr;

        pass_manager& pm;
        glfw_module& mod;

        ~state_ref_t() { mod._request_removal(*this); }
      };

      template<typename... Args>
      std::unique_ptr<state_ref_t> create_window(Args&&... args)
      {
        std::unique_ptr<state_t> st { new state_t(*hctx, std::forward<Args>(args)...) };
        std::unique_ptr<state_ref_t> r {new state_ref_t{st->window, st->emgr, st->pm, *this}};
        st->ref = r.get();

        std::lock_guard _l(lock);
        states_to_add.push_back(std::move(st));
        return r;
      }

      // automatically called on destruction of state_ref_t
      void _request_removal(state_ref_t& ref)
      {
        std::lock_guard _l(lock);
        states_to_remove.push_back(&ref);
      }

    private:
      

    private:
      static constexpr const char* module_name = "glfw";

      static bool is_compatible_with(runtime_mode m)
      {
        // we need vulkan and a screen for glfw to be active
        if ((m & runtime_mode::vulkan_context) != runtime_mode::vulkan_context)
          return false;
        if ((m & runtime_mode::offscreen) != runtime_mode::none)
          return false;
        return true;
      }

      void init_vulkan_interface(gen_feature_requester& gfr, bootstrap& /*hydra_init*/) override
      {
        gfr.require_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        uint32_t required_extension_count;
        const char** required_extensions;
        required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);

        // this is fatal (at least fatal enough to throw an exception) here 'cause if we're in this function
        // the user explicitly asked for GLFW support
        check::on_vulkan_error::n_assert(required_extensions != nullptr, "GLFW failed to find the platform surface extensions");

        // insert the needed instance extensions:
        for (size_t i = 0; i < required_extension_count; ++i)
          gfr.require_instance_extension(required_extensions[i]);
      }

      bool filter_queue(vk::instance& instance, int/*VkQueueFlagBits*/ queue_type, size_t qindex, const neam::hydra::vk::physical_device& gpu) override
      {
        const runtime_mode mode = engine->get_runtime_mode();
        const bool present_from_compute = (mode & runtime_mode::present_from_compute) != runtime_mode::none;

        if ((queue_type == VK_QUEUE_COMPUTE_BIT && present_from_compute) || (queue_type == VK_QUEUE_GRAPHICS_BIT && !present_from_compute))
        {
          if (!glfwGetPhysicalDevicePresentationSupport(instance._get_vk_instance(), gpu._get_vk_physical_device(), qindex))
            return false;
        }

        // Don't do anything for unconcerned queues:
        return true;
      }

      void add_task_groups(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_task_group("events"_rid, "events");
      }
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_dependency("render"_rid, "events"_rid);
        tgd.add_dependency("events"_rid, "init"_rid);
      }

      void on_context_initialized() override
      {
        // add an event polling task
        hctx->tm.set_start_task_group_callback("events"_rid, [this]
        {
          cctx->tm.get_task([this]
          {
            // FIXME: should be on the thread of main()
            glfwPollEvents();
          });
        });

        // setup the debug + render task
        engine->get_module<renderer_module>("renderer"_rid)->register_on_render_start("glfw::render"_rid, [this]
        {
          // render task
          cctx->tm.get_task([this]
          {
            std::lock_guard _l(lock);
            // add / remove states:
            for (auto& state : states_to_add)
            {
              states.push_back(std::move(state));
            }
            states_to_add.clear();

            for (auto* state_ref : states_to_remove)
            {
              auto it = std::find_if(states.begin(), states.end(), [state_ref](auto& a) { return a->ref == state_ref; });
              if (it != states.end())
              {
                (*it)->window.hide();
                hctx->vrd.postpone_destruction_to_next_fence(hctx->gqueue, std::move(*it));
                states.erase(it);
              }
            }

            states_to_remove.clear();

            // render all windows:
            // FIXME: Do parallel window rendering (but it seems my nvidia driver really doesn't like that)
            for (auto& state : states)
            {
              if (state->should_refresh())
                state->refresh();
              bool recreate = false;
              bool out_of_date = false;
              size_t index = state->swapchain.get_next_image_index(state->image_ready, std::numeric_limits<uint64_t>::max(), &recreate);
              if (index == ~size_t(0))
              {
                state->refresh();

                index = state->swapchain.get_next_image_index(state->image_ready, std::numeric_limits<uint64_t>::max(), &recreate);
                if (index == ~size_t(0) || recreate)
                  return;
              }

              render_pass_context rpctx
              {
                .output_size = state->swapchain.get_full_rect2D().get_size(),
                .viewport = state->swapchain.get_full_viewport(),
                .viewport_rect = state->swapchain.get_full_rect2D(),
                .output_format = state->swapchain.get_image_format(),
                .final_fb = state->framebuffers[index],
              };

              state->pm.setup(rpctx, state->need_setup);
              state->need_setup = false;

              state->pm.prepare(rpctx);

              vk::submit_info gsi;
              vk::submit_info csi;

              gsi << vk::cmd_sema_pair {state->image_ready, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
              neam::hydra::vk::command_buffer init_frame_command_buffer = state->graphic_transient_cmd_pool.create_command_buffer();
              {
                neam::hydra::vk::command_buffer_recorder cbr = init_frame_command_buffer.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
                cbr.begin_render_pass(state->init_render_pass, state->framebuffers[index], state->swapchain.get_full_rect2D(), VK_SUBPASS_CONTENTS_INLINE, {});
                cbr.end_render_pass();
              }
              init_frame_command_buffer.end_recording();
              gsi << init_frame_command_buffer;

              bool has_any_compute = false;
              state->pm.render(gsi, csi, rpctx, has_any_compute);

              neam::hydra::vk::command_buffer frame_command_buffer = state->graphic_transient_cmd_pool.create_command_buffer();
              {
                neam::hydra::vk::command_buffer_recorder cbr = frame_command_buffer.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
                cbr.begin_render_pass(state->render_pass, state->framebuffers[index], state->swapchain.get_full_rect2D(), VK_SUBPASS_CONTENTS_INLINE, {});
                cbr.end_render_pass();
              }
              frame_command_buffer.end_recording();

              gsi << frame_command_buffer >> state->render_finished;

              vk::fence gframe_done{hctx->device};
              gsi >> gframe_done;

              std::optional<vk::fence> cframe_done;
              if (has_any_compute)
              {
                cframe_done.emplace(hctx->device);
                csi >> *cframe_done;
              }

              hctx->gqueue.submit(gsi);
              if (has_any_compute)
                hctx->cqueue.submit(csi);

              // TODO: present from compute queue support
              hctx->gqueue.present(state->swapchain, index, { &state->render_finished }, &out_of_date);

              hctx->vrd.postpone_end_frame_cleanup(hctx->gqueue, hctx->allocator);

              hctx->vrd.postpone_destruction_to_next_fence(hctx->gqueue, std::move(init_frame_command_buffer), std::move(frame_command_buffer));

              state->pm.cleanup();

              hctx->vrd.postpone_destruction_inclusive(hctx->gqueue, std::move(gframe_done));
              if (has_any_compute)
                hctx->vrd.postpone_destruction_inclusive(hctx->cqueue, std::move(*cframe_done));

              if (out_of_date || recreate)
                state->refresh();
            }
          });
        });
      }

    private:
      struct state_t
      {
        template<typename... Args>
        state_t(hydra_context& _hctx, Args&&... args)
         : window(_hctx, std::forward<Args>(args)...)
         , hctx(_hctx)
         , pm(hctx)
         , swapchain(hctx.device, window._get_surface(), window.get_size())
         , image_ready(hctx.device)
         , render_finished(hctx.device)
         , init_render_pass(hctx.device)
         , render_pass(hctx.device)
         , graphic_transient_cmd_pool{hctx.gqueue.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)}
         , compute_transient_cmd_pool{hctx.cqueue.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)}
        {
          // resize window with the content size:
          window.set_size((glm::uvec2)((glm::vec2)window.get_size() * window.get_content_scale()));
          old_size = window.get_size();

          init_render_pass.create_subpass().add_attachment(neam::hydra::vk::subpass::attachment_type::color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
          init_render_pass.create_subpass_dependency(VK_SUBPASS_EXTERNAL, 0)
            .dest_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
            .source_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0);

          init_render_pass.create_attachment().set_format(swapchain.get_image_format()).set_samples(VK_SAMPLE_COUNT_1_BIT)
            .set_load_op(VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
            .set_store_op(VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
            .set_layouts(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
          init_render_pass.refresh();

          // create the present render pass:
          render_pass.create_subpass().add_attachment(neam::hydra::vk::subpass::attachment_type::color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
          render_pass.create_subpass_dependency(VK_SUBPASS_EXTERNAL, 0)
            .dest_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
            .source_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0);

          render_pass.create_attachment().set_format(swapchain.get_image_format()).set_samples(VK_SAMPLE_COUNT_1_BIT)
            .set_load_op(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
            .set_store_op(VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
            .set_layouts(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
          render_pass.refresh();

          // (re)create the framebuffers
          framebuffers.clear();
          for (size_t i = 0; i < swapchain.get_image_count(); ++i)
          {
            framebuffers.emplace_back(neam::hydra::vk::framebuffer(hctx.device, render_pass, { &swapchain.get_image_view_vector()[i] }, swapchain));
          }
        }
        ~state_t()
        {
          hctx.vrd.postpone_destruction_to_next_fence(hctx.gqueue, std::move(graphic_transient_cmd_pool));
          hctx.vrd.postpone_destruction_to_next_fence(hctx.cqueue, std::move(compute_transient_cmd_pool));
        }

        bool should_close() const { return window.should_close(); }

        bool should_refresh() const
        {
          return old_size != window.get_size();
        }

        void refresh()
        {
          old_size = window.get_size();
          // refresh without waiting:
          hctx.vrd.postpone_destruction_to_next_fence(hctx.gqueue, swapchain.recreate_swapchain(window.get_size()));
          hctx.vrd.postpone_destruction_to_next_fence(hctx.gqueue, std::move(framebuffers));

          // recreate the framebuffers:
          framebuffers.clear();
          for (size_t i = 0; i < swapchain.get_image_count(); ++i)
          {
            framebuffers.emplace_back(neam::hydra::vk::framebuffer(hctx.device, render_pass, { &swapchain.get_image_view_vector()[i] }, swapchain));
          }

          hctx.ppmgr.refresh(hctx.vrd, hctx.gqueue);
        }

        state_ref_t* ref = nullptr;

        // glfw stuff:
        glfw::window window;
        events::glfw::manager emgr { window };

        hydra_context& hctx;

        pass_manager pm;

        // vk stuff:
        vk::swapchain swapchain;

        // per-frame information
        std::vector<vk::framebuffer> framebuffers;

        vk::semaphore image_ready;
        vk::semaphore render_finished;
        vk::render_pass init_render_pass;
        vk::render_pass render_pass;

        vk::command_pool graphic_transient_cmd_pool;
        vk::command_pool compute_transient_cmd_pool;

        glm::uvec2 old_size;

        // state stuff:
        bool need_setup = true;
      };

    private:
      std::vector<std::unique_ptr<state_t>> states;

      spinlock lock;
      std::vector<std::unique_ptr<state_t>> states_to_add;
      std::vector<state_ref_t*> states_to_remove;

      friend engine_t;
      friend engine_module<glfw_module>;
  };
}

