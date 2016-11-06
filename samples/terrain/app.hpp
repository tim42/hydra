//
// file : app.hpp
// in : file:///home/tim/projects/hydra/samples/terrain/app.hpp
//
// created by : Timothée Feuillet
// date: Sat Nov 05 2016 16:17:47 GMT-0400 (EDT)
//
//
// Copyright (c) 2016 Timothée Feuillet
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

#ifndef __N_1117511799121637201_1265716500_APP_HPP__
#define __N_1117511799121637201_1265716500_APP_HPP__

#include <vector>

#include <hydra/hydra.hpp>          // the main header of hydra
#include <hydra/hydra_glm_udl.hpp>  // we do want the easy glm udl (optional, not used by hydra)
#include <hydra/hydra_glfw_ext.hpp> // for the window creation / handling

#include <hydra/tools/logger/logger.hpp>
#include <hydra/tools/chrono.hpp>
#include <hydra/tools/uninitialized.hpp>

namespace neam
{
  /// \brief A simple application class that just do some work
  class application : public neam::hydra::glfw::events::window_listener
  {
    public:
      application(const glm::uvec2 &window_size, const std::string &window_name)
        : gfr(), glfw_ext(), hydra_init(),
          instance(create_instance()),
          window(glfw_ext.create_window(instance, window_size, window_name)),
          emgr(window),
          device(hydra_init.create_device(instance)),
          gqueue(device, window._get_win_queue()),
          tqueue(device, *temp_transfer_queue),
          mem_alloc(device),
          swapchain(window._create_swapchain(device)),
          cmd_pool(gqueue.create_command_pool()),
          transfer_cmd_pool(tqueue.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)),
          btransfers(device, tqueue, transfer_cmd_pool),
          image_ready(device),
          render_finished(device),
          shmgr(device),
          ppmgr(device),
          render_pass(device)
      {
        btransfers.allocate_memory(mem_alloc);

        emgr.register_window_listener(this);
      }

      virtual ~application()
      {
        device.wait_idle();
      }

      void init_and_run()
      {
        init();
        run();
      }

    protected:
      void init()
      {
        pre_init_hook();

        // (re)create the framebuffer / command buffer things
        framebuffers.clear();
        frame_command_buffers.clear();
        frame_submit_info.clear();
        for (size_t i = 0; i < swapchain.get_image_count(); ++i)
        {
          framebuffers.emplace_back(neam::hydra::vk::framebuffer(device, render_pass, { &swapchain.get_image_view_vector()[i] }, swapchain));
          frame_command_buffers.emplace_back(cmd_pool.create_command_buffer());

          {
            neam::hydra::vk::command_buffer_recorder cbr = frame_command_buffers.back().begin_recording(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

            init_command_buffer(cbr, framebuffers.back(), i);

            frame_command_buffers.back().end_recording();
          }

          frame_submit_info.emplace_back();
          frame_submit_info.back() << neam::hydra::vk::cmd_sema_pair {image_ready, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT} << frame_command_buffers.back() >> render_finished;
        }
      }

      void refresh()
      {
        device.wait_idle();

        // refresh the swapchain + renderpass
        swapchain.recreate_swapchain(window.get_size());
        render_pass.refresh();

        ppmgr.refresh();

        refresh_hook();

        init();
      }

      void run()
      {
        neam::cr::chrono cr;
        float frame_cnt = 0;

        neam::cr::out.log() << LOGGER_INFO << "btransfer: remaining " << btransfers.get_total_size_to_transfer() << " bytes..." << std::endl;
        btransfers.wait_end_transfer(); // can't really do much more here

        bool recreate = false;

        pre_run_hook();

        cr.reset();
        while (!window.should_close())
        {
          glfwPollEvents();

          {
            size_t index = swapchain.get_next_image_index(image_ready, std::numeric_limits<uint64_t>::max(), &recreate);
            if (index == ~size_t(0))
            {
              refresh();
              continue;
            }
            if (should_recreate_command_framebuffer())
            {
              neam::hydra::vk::command_buffer_recorder cbr = frame_command_buffers[index].begin_recording(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

              init_command_buffer(cbr, framebuffers[index], index);

              frame_command_buffers[index].end_recording();
            }

            gqueue.submit(frame_submit_info[index]);
            gqueue.present(swapchain, index, { &render_finished });

            if (recreate)
            {
              refresh();
            }
          }

          render_loop_hook();

          ++frame_cnt;

          if (cr.get_accumulated_time() > 2.)
          {
            neam::cr::out.log() << (cr.get_accumulated_time() / frame_cnt) * 1000.f << "ms/frame\t(" << (int)(frame_cnt / cr.get_accumulated_time()) << "fps)" << std::endl;
            cr.reset();
            frame_cnt = 0;
          }
        }

        post_run_hook();
        device.wait_idle();
        post_run_idle_hook();
      }

    protected:
      neam::hydra::vk::instance create_instance()
      {
        // some requirements of the app
        glfw_ext.request_graphic_queue(true); // the queue
        gfr.require_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // debug extensions / layers:
        gfr.require_instance_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        gfr.require_instance_layer("VK_LAYER_LUNARG_standard_validation");

        temp_transfer_queue = gfr.require_queue_capacity(VK_QUEUE_TRANSFER_BIT, false);

        create_instance_hook(gfr);

        hydra_init.register_init_extension(glfw_ext);
        hydra_init.register_feature_requester(gfr);

        create_instance_hook(hydra_init);

        neam::hydra::vk::instance temp_instance = hydra_init.create_instance("hydra-test-dev");
        temp_instance.install_default_debug_callback(VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT);
        // temp_instance.install_default_debug_callback(VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT); // We want all the reports (maximum debug)
        return temp_instance;
      }

      // Watch for resize events
      virtual void framebuffer_resized(const glm::vec2 &) { refresh(); }

    protected: // hooks
      virtual void create_instance_hook(neam::hydra::gen_feature_requester &/*gfr*/) {}
      virtual void create_instance_hook(neam::hydra::bootstrap &/*hydra_init*/) {}

      virtual void pre_init_hook() {}
      virtual void init_command_buffer(neam::hydra::vk::command_buffer_recorder &/*cbr*/, neam::hydra::vk::framebuffer &/*fb*/, size_t /*index*/) {}

      virtual void refresh_hook() {} // called when both the renderpass and the swapchain has been recreated

      virtual void pre_run_hook() {}
      virtual void render_loop_hook() {}
      virtual void post_run_hook() {}
      virtual void post_run_idle_hook() {} // called when the device is idle

      // you should not override those two hooks: (but use instead render_loop_hook())
      virtual bool should_recreate_command_framebuffer() { return false; }
      virtual void recreate_command_buffer(neam::hydra::vk::command_buffer_recorder &cbr, neam::hydra::vk::framebuffer &fb, size_t index)
      {
        init_command_buffer(cbr, fb, index);
      }

    private:
      neam::hydra::gen_feature_requester gfr;
      neam::hydra::glfw::init_extension glfw_ext;
      neam::hydra::bootstrap hydra_init;
      neam::hydra::temp_queue_familly_id_t *temp_transfer_queue = nullptr;

    protected:
      neam::hydra::vk::instance instance;
      neam::hydra::glfw::window window;
      neam::hydra::glfw::events::manager emgr;
      neam::hydra::vk::device device;
      neam::hydra::vk::queue gqueue;
      neam::hydra::vk::queue tqueue;

      // the memory allocator
      neam::hydra::memory_allocator mem_alloc;

      // the swapchain
      neam::hydra::vk::swapchain swapchain;

      // command pools (graphic / transfer)
      neam::hydra::vk::command_pool cmd_pool;
      neam::hydra::vk::command_pool transfer_cmd_pool;
      neam::hydra::batch_transfers btransfers;

      // per-frame information
      std::vector<neam::hydra::vk::framebuffer> framebuffers;
      std::vector<neam::hydra::vk::command_buffer> frame_command_buffers;
      std::vector<neam::hydra::vk::submit_info> frame_submit_info;

      neam::hydra::vk::semaphore image_ready;
      neam::hydra::vk::semaphore render_finished;

      neam::hydra::shader_manager shmgr;
      neam::hydra::pipeline_manager ppmgr;

      neam::hydra::vk::render_pass render_pass; // NOTE: automatically refreshed by refresh()
  };
} // namespace neam

#endif // __N_1117511799121637201_1265716500_APP_HPP__

