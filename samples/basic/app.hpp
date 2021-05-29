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
#include <thread>

#include <hydra/hydra.hpp>          // the main header of hydra
#include <hydra/hydra_glm_udl.hpp>  // we do want the easy glm udl (optional, not used by hydra)
#include <hydra/hydra_glfw_ext.hpp> // for the window creation / handling
#include <hydra/hydra_imgui_ext.hpp> // for the window creation / handling

#include <hydra/tools/logger/logger.hpp>
#include <hydra/tools/chrono.hpp>
#include <hydra/tools/uninitialized.hpp>

#include "imgui_log_window.hpp"

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
          transient_cmd_pool(gqueue.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)),
          reset_cmd_pool(gqueue.create_command_pool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)),
          transfer_cmd_pool(tqueue.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)),
          btransfers(device, tqueue, transfer_cmd_pool, vrd),
          image_ready(device),
          render_finished(device),
          shmgr(device),
          ppmgr(device),
          imgui_ctx(window, instance, device, swapchain, emgr),
          render_pass(device)
      {
        btransfers.allocate_memory(mem_alloc);

        emgr.register_window_listener(this);

        imgui_ctx.init(gqueue, transient_cmd_pool, vrd);
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

        // (re)create the framebuffers
        framebuffers.clear();
        for (size_t i = 0; i < swapchain.get_image_count(); ++i)
        {
          framebuffers.emplace_back(neam::hydra::vk::framebuffer(device, render_pass, { &swapchain.get_image_view_vector()[i] }, swapchain));
        }
      }

      void refresh()
      {
        device.wait_idle();

        // refresh the swapchain + renderpass
        swapchain.recreate_swapchain(window.get_size());
        render_pass.refresh();

        ppmgr.refresh();

        imgui_ctx.refresh(swapchain);

        refresh_hook();

        init();
      }

      void run()
      {
        neam::cr::chrono cr;
        float frame_cnt = 0.f;
        float wasted = 0.f;

        if (btransfers.get_total_size_to_transfer() > 0)
          neam::cr::out.log() << "btransfer: remaining " << btransfers.get_total_size_to_transfer() << " bytes..." << std::endl;
        btransfers.wait_end_transfer(); // can't really do much more here

        pre_run_hook();

        cr.reset();
        size_t count_since_mem_stats = 0;

        imgui_ctx.new_frame();

        while (!window.should_close())
        {
          bool recreate = false;
          bool out_of_date = false;
          neam::cr::chrono frame_cr;

          {
            size_t index = swapchain.get_next_image_index(image_ready, std::numeric_limits<uint64_t>::max(), &recreate);
            if (index == ~size_t(0))
            {
              refresh();
              continue;
            }

            render_loop_hook();
            log_window.show_log_window();

            neam::hydra::vk::command_buffer frame_command_buffer = transient_cmd_pool.create_command_buffer();
            {
              neam::hydra::vk::command_buffer_recorder cbr = frame_command_buffer.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
              init_command_buffer(cbr, framebuffers[index], index);
              frame_command_buffer.end_recording();
            }

            neam::hydra::vk::command_buffer imgui_command_buffer = transient_cmd_pool.create_command_buffer();
            {
              neam::hydra::vk::command_buffer_recorder cbr = imgui_command_buffer.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
              imgui_ctx.draw(imgui_command_buffer, cbr, framebuffers[index]);
              imgui_command_buffer.end_recording();
            }

            neam::hydra::vk::fence frame_done{device};
            neam::hydra::vk::submit_info si;
            si << neam::hydra::vk::cmd_sema_pair {image_ready, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}
               << frame_command_buffer << imgui_command_buffer >> frame_done >> render_finished;

            // delete the transient resources at the right time:
            vrd.postpone_destruction(std::move(frame_done), std::move(imgui_command_buffer), std::move(frame_command_buffer));

            gqueue.submit(si);
            gqueue.present(swapchain, index, { &render_finished }, &out_of_date);

            if (recreate || out_of_date)
              refresh();
          }

          glfwPollEvents();
          imgui_ctx.new_frame();

          vrd.update();
          const size_t to_transfer = btransfers.get_total_size_to_transfer();
          btransfers.start();
          show_imgui_basic_stats(frame_cr.get_accumulated_time() * 1000., to_transfer);

          ++frame_cnt;

          if (cr.get_accumulated_time() > 4.)
          {
            const double accumulated_time = cr.get_accumulated_time();
            const double used_time = ((accumulated_time - wasted) / frame_cnt) * 1000.f;
            const double wasted_time = (wasted / frame_cnt) * 1000.f;
            neam::cr::out.log() << (accumulated_time / frame_cnt) * 1000.f
                                << "ms/frame [used: " << used_time << "ms/frame, wasted: " << wasted_time << "ms/frame]"
                                << "\t(" << (int)(frame_cnt / accumulated_time) << "fps)" << std::endl;
            ++count_since_mem_stats;
            if (count_since_mem_stats == 8)
            {
              count_since_mem_stats = 0;
              mem_alloc.print_stats();
            }
            cr.reset();
            frame_cnt = 0.f;
            wasted = 0.f;
          }

          if (rate_limit > 0.0001 && frame_cr.get_accumulated_time() < rate_limit)
          {
            const double original_accumulated_time = frame_cr.get_accumulated_time();
            double cr = rate_limit - frame_cr.get_accumulated_time();
            while (cr > 0.00001) // avoid overshoothing
            {
              std::this_thread::yield();
              cr = rate_limit - frame_cr.get_accumulated_time();
            }
            wasted += frame_cr.get_accumulated_time() - original_accumulated_time;
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
        gfr.require_instance_layer("VK_LAYER_KHRONOS_validation");

        temp_transfer_queue = gfr.require_queue_capacity(VK_QUEUE_TRANSFER_BIT, false);

        create_instance_hook(gfr);

        hydra_init.register_init_extension(glfw_ext);
        hydra_init.register_feature_requester(gfr);

        create_instance_hook(hydra_init);

        neam::hydra::vk::instance temp_instance = hydra_init.create_instance("hydra-test-dev");
        temp_instance.install_default_debug_callback(VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT);
//         temp_instance.install_default_debug_callback(VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT); // We want all the reports (maximum debug)
        return temp_instance;
      }

      void show_imgui_basic_stats(const float frame_ms, const size_t to_transfer)
      {
        const float PAD = 10.0f;
        const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (work_pos.x + work_size.x - PAD);
        window_pos.y = (work_pos.y + PAD);
        window_pos_pivot.x = 1.0f;
        window_pos_pivot.y = 0.0f;

        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

        if (ImGui::Begin("", nullptr, window_flags))
        {
          ImGui::Text("Raw timings: %7.3f ms/frame [ %8.2f fps ]", frame_ms, 1000.0f / frame_ms);
          if (rate_limit > 0)
            ImGui::Text("FPS Locked at: %.2f", 1.0f / rate_limit);

          ImGui::Text("Data to transfer: %.3f Kb", (to_transfer / 1000.0f));

          ImGui::Text("Loaded pipelines: %zu | Loaded shaders: %zu", ppmgr.get_pipeline_count(), shmgr.get_shader_count());

          ImGui::Text("Allocated GPU Memory: %.3f Mb [ in %zu allocations ]", mem_alloc.get_used_memory() / (1.0e6f), mem_alloc.get_allocation_count());

          ImGui::Separator();

          {
            constexpr unsigned history_size = 400;
            static float frame_ms_history[history_size] = { 0.0f };
            static unsigned write_index = 0;
            frame_ms_history[write_index] = frame_ms;
            write_index = (write_index + 1) % history_size;
            const auto plot_fnc = +[](void*, int idx) -> float
            {
              return frame_ms_history[(idx + write_index) % history_size];
            };
            ImGui::PlotHistogram("", plot_fnc, frame_ms_history, history_size, 0, nullptr, 0.0f, FLT_MAX, ImVec2(history_size, 30));
          }
        }
        ImGui::End();
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
      neam::hydra::vk::command_pool transient_cmd_pool;
      neam::hydra::vk::command_pool reset_cmd_pool;

      neam::hydra::vk::command_pool transfer_cmd_pool;

      neam::hydra::vk_resource_destructor vrd;
      neam::hydra::batch_transfers btransfers;

      // per-frame information
      std::vector<neam::hydra::vk::framebuffer> framebuffers;
      std::vector<neam::hydra::vk::command_buffer> frame_command_buffers;

      neam::hydra::vk::semaphore image_ready;
      neam::hydra::vk::semaphore render_finished;

      neam::hydra::shader_manager shmgr;
      neam::hydra::pipeline_manager ppmgr;

      neam::hydra::imgui::context imgui_ctx;

      neam::hydra::vk::render_pass render_pass; // NOTE: automatically refreshed by refresh()

      imgui_log_window log_window;

      double rate_limit = 0.f; // no limit
  };
} // namespace neam

#endif // __N_1117511799121637201_1265716500_APP_HPP__

