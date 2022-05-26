
// #include <neam/reflective/reflective.hpp>

// #define N_ALLOW_DEBUG true // we want full debug information
// #define N_DISABLE_CHECKS 1 // we don't want anything
#define HYDRA_AUTO_BUFFER_NO_SMART_SYNC
// #include "sample.hpp"

#include <ntools/chrono.hpp>

#include <hydra/engine/engine.hpp>
#include <hydra/glfw/glfw_window.hpp>
#include <hydra/glfw/glfw_events.hpp>
#include <hydra/hydra_glm_udl.hpp>  // we do want the easy glm udl (optional, not used by hydra)

#include "fs_quad_pass.hpp"

namespace neam::hydra
{
  /// \brief Temp renderer
  class app_module : private engine_module<app_module>
  {
    public:

    private: // module interface
      static bool is_compatible_with(runtime_mode m)
      {
        // we need full hydra and a screen for the module to be active
        if ((m & runtime_mode::hydra_context) == runtime_mode::none)
          return false;
        if ((m & runtime_mode::offscreen) != runtime_mode::none)
          return false;
        return true;
      }

      void add_task_groups(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_task_group("render"_rid, "render");
        tgd.add_task_group("events"_rid, "events");
      }
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_dependency("render"_rid, "io"_rid);
        tgd.add_dependency("render"_rid, "events"_rid);
      }

      void on_context_initialized() override
      {
        // we actualy create the window while the index is loading.
        // This may lead to cases where the window gets created before the index fail to load and needs to be closed.
        state = std::make_unique<state_t>(*hctx);
        cr::out().debug("renderer initialized");

        state->pm.add_pass<fs_quad_pass>(*hctx);

        // add an event polling task
        hctx->tm.set_start_task_group_callback("events"_rid, [this]
        {
          cctx->tm.get_task([this]
          {
            glfwPollEvents();
          });
        });

        // setup the debug + render task
        hctx->tm.set_start_task_group_callback("render"_rid, [this]
        {
          cctx->tm.get_task([this]
          {
            // check to see if we need to close the window:
            if (state->window.should_close())
            {
              engine->sync_teardown();
              return;
            }

            // hard sleep (FIXME: remove)
            // this only locks a single thread, making the frame at least that long (to not burn my cpu/gpu)
            std::this_thread::sleep_for(std::chrono::microseconds(4000));

            // print some debug:
            ++state->frame_cnt;
            const double max_dt_s = 4;
            if (state->chrono.get_accumulated_time() >= max_dt_s)
            {
              const double dt = state->chrono.delta();
              cr::out().debug("{}s avg: ms/frame: {:6.3}ms [{:.2f} fps]", max_dt_s, (dt * 1000 / state->frame_cnt), (state->frame_cnt / dt));
              state->frame_cnt = 0;
            }
          });

          // render task
          cctx->tm.get_task([this]
          {
            hctx->vrd.update();

            bool recreate = false;
            bool out_of_date = false;
            size_t index = state->swapchain.get_next_image_index(state->image_ready, std::numeric_limits<uint64_t>::max(), &recreate);
            if (index == ~size_t(0) || recreate)
            {
              state->refresh(*hctx);

              index = state->swapchain.get_next_image_index(state->image_ready, std::numeric_limits<uint64_t>::max(), &recreate);
              if (index == ~size_t(0) || recreate)
                return;
            }

            vk::submit_info gsi;
            vk::submit_info csi;

            render_pass_context rpctx
            {
              .output_size = state->swapchain.get_full_rect2D().get_size(),
              .viewport = state->swapchain.get_full_viewport(),
              .viewport_rect = state->swapchain.get_full_rect2D(),
              .output_format = state->swapchain.get_image_format(),
              .final_fb = state->framebuffers[index],
            };

            if (state->need_setup)
            {
              state->pm.setup(rpctx);
              state->need_setup = false;
            }

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

            hctx->vrd.postpone_destruction(hctx->gqueue, std::move(gframe_done), std::move(init_frame_command_buffer), std::move(frame_command_buffer));
            if (has_any_compute)
              hctx->vrd.postpone_destruction(hctx->cqueue, std::move(*cframe_done));

            state->pm.cleanup();
            if (out_of_date)
              state->refresh(*hctx);
          });
        });
      }


    private:
      struct state_t
      {
        state_t(hydra_context& hctx)
         : window(hctx, 400_uvec2_xy)
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

        void refresh(hydra_context& hctx)
        {
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

        // glfw stuff:
        glfw::window window;
        glfw::events::manager emgr { window };

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

        // debug stuff:
        cr::chrono chrono;
        uint64_t frame_cnt = 0.f;
        float wasted = 0.f;

        // state stuff:
        bool need_setup = true;
      };
      std::unique_ptr<state_t> state;
      friend class engine_t;
      friend engine_module<app_module>;
  };
}

int main(int, char **)
{
  neam::cr::out.min_severity = neam::cr::logger::severity::debug/*message*/;
  neam::cr::out.register_callback(neam::cr::print_log_to_console, nullptr);
  neam::cr::out().log("app start");

  neam::hydra::engine_t engine;
  engine.boot(neam::hydra::runtime_mode::hydra_context, "caca"_rid);

  engine.get_core_context().thread_main(engine.get_core_context());

  return 0;
}

