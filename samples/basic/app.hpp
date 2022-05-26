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

#include <ntools/logger/logger.hpp>
#include <ntools/chrono.hpp>
// #include <ntools/uninitialized.hpp>

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
          context(instance, hydra_init.create_device(instance),
                  window._get_win_queue(), *temp_transfer_queue, *temp_compute_queue),
          swapchain(window._create_swapchain(context.device)),
          image_ready(context.device),
          render_finished(context.device),
          transfer_finished(context.device),
          imgui_ctx(context, window, emgr),
          render_pass(context.device)
      {
        {
          threading::task_group_dependency_tree tgd;

          // pre-made groups:
          tgd.add_task_group("init"_rid, "init");
          tgd.add_task_group("io"_rid, "io");
          tgd.add_task_group("render"_rid, "render");

          tgd.add_dependency("io"_rid, "init"_rid);
          tgd.add_dependency("render"_rid, "io"_rid);

          // 

          context.boot(tgd.compile_tree(), "caca"_rid);
        }
        {
          // setup the task manager:
          context.tm.set_start_task_group_callback("io"_rid, [this]
          {
            context.tm.get_task([this] { context.io.process(); });
          });
          context.tm.set_end_task_group_callback("render"_rid, [this]
          {
            // FIXME
          });
        }
        // FIXME: remove
        context.io._wait_for_submit_queries();
        imgui_ctx.load_default_fonts();
        emgr.register_window_listener(this);

        window.set_size((glm::uvec2)((glm::vec2)window_size * window.get_content_scale()));

        render_pass.create_subpass().add_attachment(neam::hydra::vk::subpass::attachment_type::color,                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
        render_pass.create_subpass_dependency(VK_SUBPASS_EXTERNAL, 0)
          .dest_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
          .source_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0);

        render_pass.create_attachment().set_swapchain(&swapchain).set_samples(VK_SAMPLE_COUNT_1_BIT)
          .set_load_op(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
          .set_store_op(VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
          .set_layouts(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        render_pass.refresh();
      }

      virtual ~application()
      {
        context.device.wait_idle();
      }

      void init_and_run()
      {
        setup_hook();
        init();
        run();
      }

    protected:
      void init()
      {
        // (re)create the framebuffers
        framebuffers.clear();
        for (size_t i = 0; i < swapchain.get_image_count(); ++i)
        {
          framebuffers.emplace_back(neam::hydra::vk::framebuffer(context.device, render_pass, { &swapchain.get_image_view_vector()[i] }, swapchain));
        }
      }

      void refresh()
      {
        context.device.wait_idle();

        // refresh the swapchain + renderpass
        swapchain.recreate_swapchain(window.get_size());

        render_pass.refresh();
        refresh_hook();

        context.ppmgr.refresh();

        init();
      }

      void run()
      {
        neam::cr::chrono cr;
        float frame_cnt = 0.f;
        float wasted = 0.f;

        if (context.transfers.get_total_size_to_transfer() > 0)
          neam::cr::out().log("btransfer: remaining {} bytes...", context.transfers.get_total_size_to_transfer());

        cr.reset();
        size_t count_since_mem_stats = 0;

        imgui_ctx.new_frame();

        while (!window.should_close())
        {
          bool recreate = false;
          bool out_of_date = false;
          neam::cr::chrono frame_cr;

          size_t to_transfer = 0;

          {
            size_t index = swapchain.get_next_image_index(image_ready, std::numeric_limits<uint64_t>::max(), &recreate);
            if (index == ~size_t(0))
            {
              refresh();
              continue;
            }

            render_loop_hook();
            log_window.show_log_window();

            prepare_hook();
            to_transfer = context.transfers.get_total_size_to_transfer();
            const bool has_transfers = context.transfers.transfer(context.allocator, { &transfer_finished });

            neam::hydra::vk::command_buffer frame_command_buffer = context.graphic_transient_cmd_pool.create_command_buffer();
            {
              neam::hydra::vk::command_buffer_recorder cbr = frame_command_buffer.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
              submit_hook(cbr, framebuffers[index]);

              cbr.begin_render_pass(render_pass, framebuffers[index], swapchain.get_full_rect2D(), VK_SUBPASS_CONTENTS_INLINE, {});
              cbr.end_render_pass();
              frame_command_buffer.end_recording();
            }

            neam::hydra::vk::fence frame_done{context.device};
            neam::hydra::vk::submit_info si;

            if (has_transfers)
              si << neam::hydra::vk::cmd_sema_pair {transfer_finished, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

            si << neam::hydra::vk::cmd_sema_pair {image_ready, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}
               << frame_command_buffer >> render_finished >> frame_done;

            context.gqueue.submit(si);

            cleanup_hook();

            // delete the transient resources at the right time:
            context.vrd.postpone_end_frame_cleanup(context.gqueue, context.allocator);
            context.vrd.postpone_destruction(context.gqueue, std::move(frame_done),  std::move(frame_command_buffer));

            context.gqueue.present(swapchain, index, { &render_finished }, &out_of_date);

            context.io.process(); // process queued IO operations
            if (recreate || out_of_date)
              refresh();
          }

          glfwPollEvents();
          imgui_ctx.new_frame();

          show_imgui_basic_stats(frame_cr.get_accumulated_time() * 1000., to_transfer);

          ++frame_cnt;

          if (cr.get_accumulated_time() > 4.)
          {
            const double accumulated_time = cr.get_accumulated_time();
            const double used_time = ((accumulated_time - wasted) / frame_cnt) * 1000.f;
            const double wasted_time = (wasted / frame_cnt) * 1000.f;
            neam::cr::out().log("{:6.3} ms/frame [used: {:6.3} ms/frame, wasted: {:6.3} ms/frame]\t({} fps)",
                                (accumulated_time / frame_cnt) * 1000.f,
                                used_time,
                                wasted_time,
                                (int)(frame_cnt / accumulated_time));
            ++count_since_mem_stats;
            if (count_since_mem_stats == 8)
            {
              count_since_mem_stats = 0;
              context.allocator.print_stats();
            }
            cr.reset();
            frame_cnt = 0.f;
            wasted = 0.f;
          }

          context.vrd.update();

          if (rate_limit > 0.0001 && frame_cr.get_accumulated_time() < rate_limit)
          {
            const double original_accumulated_time = frame_cr.get_accumulated_time();
            double cr = rate_limit - frame_cr.get_accumulated_time();
            const uint64_t slack_us = 1000; // avoid overshoothing

            // hard sleep:
            std::this_thread::sleep_for(std::chrono::microseconds((uint64_t)(cr * 1e6) - slack_us));

            // spin yield the remaining time:
            while (cr > 0.00001) // avoid overshoothing
            {
              std::this_thread::yield();
              cr = rate_limit - frame_cr.get_accumulated_time();
            }
            wasted += frame_cr.get_accumulated_time() - original_accumulated_time;
          }
        }

        context.device.wait_idle();
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
        temp_compute_queue = gfr.require_queue_capacity(VK_QUEUE_COMPUTE_BIT, false);

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
        const float font_size = ImGui::GetFontSize();
        const float PAD = font_size;
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

          ImGui::Text("Loaded pipelines: %zu | Loaded shaders: %zu", context.ppmgr.get_pipeline_count(), context.shmgr.get_shader_count());

          ImGui::Text("Allocated GPU Memory: %.3f Mb [ in %zu allocations ]", context.allocator.get_used_memory() / (1.0e6f), context.allocator.get_allocation_count());
          ImGui::Text("Reserved GPU Memory:  %.3f Mb", context.allocator.get_reserved_memory() / (1.0e6f));

          ImGui::Separator();

          {
            constexpr unsigned history_size = 500;
            static float frame_ms_history[history_size] = { 0.0f };
            static unsigned write_index = 0;
            frame_ms_history[write_index] = frame_ms;
            write_index = (write_index + 1) % history_size;
            const auto plot_fnc = +[](void*, int idx) -> float
            {
              return frame_ms_history[(idx + write_index) % history_size];
            };

            ImGui::PlotHistogram("", plot_fnc, frame_ms_history, history_size, 0, nullptr, 0.0f, FLT_MAX, ImVec2(ImGui::GetContentRegionAvail().x, font_size * 2));
          }
        }
        ImGui::End();
      }

      // Watch for resize events
      virtual void framebuffer_resized(const glm::vec2 &) { refresh(); }

    protected: // hooks
      virtual void create_instance_hook(neam::hydra::gen_feature_requester &/*gfr*/) {}
      virtual void create_instance_hook(neam::hydra::bootstrap &/*hydra_init*/) {}


      virtual void render_loop_hook() {}

      virtual void refresh_hook() {}
      virtual void setup_hook() {}
      virtual void prepare_hook() {}
      virtual void submit_hook(hydra::vk::command_buffer_recorder& /*cbr*/, hydra::vk::framebuffer& /*fb*/) {}
      virtual void cleanup_hook() {}

    private:
      neam::hydra::gen_feature_requester gfr;
      neam::hydra::glfw::init_extension glfw_ext;
      neam::hydra::bootstrap hydra_init;
      neam::hydra::temp_queue_familly_id_t *temp_transfer_queue = nullptr;
      neam::hydra::temp_queue_familly_id_t *temp_compute_queue = nullptr;

    protected:
      neam::hydra::vk::instance instance;
      neam::hydra::glfw::window window;
      neam::hydra::glfw::events::manager emgr;

      neam::hydra::hydra_context context;

      // the swapchain
      neam::hydra::vk::swapchain swapchain;

      // per-frame information
      std::vector<neam::hydra::vk::framebuffer> framebuffers;
      std::vector<neam::hydra::vk::command_buffer> frame_command_buffers;

      neam::hydra::vk::semaphore image_ready;
      neam::hydra::vk::semaphore render_finished;
      neam::hydra::vk::semaphore transfer_finished;

      neam::hydra::imgui::imgui_context imgui_ctx;
//       neam::hydra::imgui::render_pass imgui_rp;

      neam::hydra::vk::render_pass render_pass; // NOTE: automatically refreshed by refresh()

      imgui_log_window log_window;

      double rate_limit = 0.f; // no limit
  };
} // namespace neam

#endif // __N_1117511799121637201_1265716500_APP_HPP__

