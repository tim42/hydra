
// #include <neam/reflective/reflective.hpp>

// #define N_ALLOW_DEBUG true // we want full debug information
// #define N_DISABLE_CHECKS 1 // we don't want anything
#define HYDRA_AUTO_BUFFER_NO_SMART_SYNC
// #include "sample.hpp"

#include <ntools/chrono.hpp>
#include <ntools/struct_metadata/fmt_support.hpp>

#include <hydra/engine/engine.hpp>
#include <hydra/glfw/glfw_window.hpp>
#include <hydra/glfw/glfw_events.hpp>
#include <hydra/hydra_glm_udl.hpp>  // we do want the easy glm udl (optional, not used by hydra)
#include <hydra/glfw/glfw_engine_module.hpp>  // Add the glfw module
#include <hydra/imgui/imgui_engine_module.hpp>  // Add the imgui module
#include <hydra/imgui/generic_ui.hpp>

#include "imgui_log_window.hpp"
#include "fs_quad_pass.hpp"

using namespace neam;

struct data_sample_ar_t
{
  glm::i8vec2 truc;
  glm::vec3 truc2;
  glm::bvec4 btruc;
  std::string stuff = "hello";
};
struct data_sample_t
{
  // options
  unsigned force = 0;
  float length = 1;
  uint16_t other = 32;

  std::vector<std::variant<std::optional<bool>, uint32_t, data_sample_ar_t>> parameters
  {
    1u,
    2u,
    {},
    data_sample_ar_t{{1, 2}, {3, 4, 5}, {true, true, false, false}, "yo"},
    5u,
    data_sample_ar_t{},
    0u
  };

  std::map<std::string, int> map;
  std::vector<int> aré;
  neam::id_t key = neam::id_t::invalid;
  std::string command = "hello";
};
N_METADATA_STRUCT(data_sample_ar_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(truc, metadata::range<int8_t>{.min = -100, .max = 100, .step=3}),
    N_MEMBER_DEF(truc2, metadata::range<float>{.step = 0.5f}),
    N_MEMBER_DEF(btruc, metadata::info{.description = c_string_t<"Checkboxes.\nYou want 'em? We got 'em">}),
    N_MEMBER_DEF(stuff)
  >;
};

N_METADATA_STRUCT(data_sample_t)
{
  using member_list = ct::type_list
  <
    N_MEMBER_DEF(force, metadata::range<uint32_t>{.min = 10, .max = 150, .step = 8},
                        metadata::info{.description = c_string_t<"Does forcy stuff.\nDescriptions can be\nsplit on\nmultiple lines!!">, .doc_url = c_string_t<"https://en.wikipedia.org/wiki/Force">}),
    N_MEMBER_DEF(length, metadata::range<float>{.step = 0.1f}),
    N_MEMBER_DEF(other, metadata::range<uint16_t>{.step = 3}),
    N_MEMBER_DEF(parameters),
    N_MEMBER_DEF(map),
    N_MEMBER_DEF(aré),
    N_MEMBER_DEF(key),
    N_MEMBER_DEF(command)
  >;
};

namespace neam::hydra
{
  /// \brief Temp renderer
  class app_module : private engine_module<app_module>
  {
    public:
      raw_data ser_data;
      rle::serialization_metadata ser_metadata;

    private: // module interface
      static bool is_compatible_with(runtime_mode m)
      {
//         cr::out().log("{}", data_sample_t{});

        // we need full hydra and a screen for the module to be active
        if ((m & runtime_mode::hydra_context) == runtime_mode::none)
          return false;
        if ((m & runtime_mode::offscreen) != runtime_mode::none)
          return false;
        return true;
      }

      void add_task_groups(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_task_group("during_render"_rid, "during_render");
        tgd.add_task_group("after_render"_rid, "after_render");
      }
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_dependency("during_render"_rid, "events"_rid);
        tgd.add_dependency("after_render"_rid, "render"_rid);
      }

      void on_context_initialized() override
      {
        ser_data = rle::serialize(data_sample_t{});
        ser_metadata = rle::generate_metadata<data_sample_t>();

        // we actualy create the window while the index is loading.
        // This may lead to cases where the window gets created before the index fail to load and needs to be closed.
        cr::out().debug("creating application main window/render-context...");
        window_state = engine->get_module<glfw::glfw_module>("glfw"_rid)->create_window(800_uvec2_xy);
        cr::out().debug("created application main window and render-context");
        fs_quad_pass& pass = window_state->pm.add_pass<fs_quad_pass>(*hctx);

        auto* imgui = engine->get_module<imgui::imgui_module>("imgui"_rid);
        imgui->create_context(*window_state.get());
        imgui->register_function("dockspace"_rid, [this]()
        {
          ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        });
        imgui->register_function("main"_rid, [this, &pass]()
        {
          ImGui::Begin("Conf", nullptr, 0);
          auto* img = pass.get_hydra_logo_img_view();
          if (img)
          {
            float width = ImGui::GetContentRegionAvail().x;
            ImGui::Image(img, ImVec2(width, width/5), ImVec2(0, 0.45f), ImVec2(1, 0.65f));
          }
          ImGui::Text("Avg Frame Time: %.3f ms", last_average_frametime * 1000);
          ImGui::Text("Avg FPS: %.3f f/s", 1.0f / last_average_frametime);
          ImGui::Text("Limit framerate:"); ImGui::SameLine(); ImGui::Checkbox("##limit-fps", &frametime_limiter);
          if (ImGui::Button("force full shader refresh"))
          {
            hctx->shmgr.refresh();
            hctx->ppmgr.refresh(hctx->vrd, hctx->gqueue);
          }
          ImGui::Separator();
          ser_data = hydra::imgui::generate_ui(ser_data, ser_metadata);
          ImGui::End();
        });
        imgui->register_function("demo"_rid, [this]()
        {
          ImGui::ShowDemoWindow();
        });
        imgui->register_function("log_window"_rid, [this]()
        {
          log_window.show_log_window();
        });

        // setup the debug + render task
        hctx->tm.set_start_task_group_callback("during_render"_rid, [this]
        {
          cctx->tm.get_task([this]
          {
            // hard sleep (FIXME: remove)
            // this only locks a single thread, making the frame at least that long (to not burn my cpu/gpu)
            if (frametime_limiter)
              std::this_thread::sleep_for(std::chrono::microseconds(12000));

            // print some debug:
            ++frame_cnt;
            ++total_frame_cnt;
            const double max_dt_s = 1.5f;
            if (chrono.get_accumulated_time() >= max_dt_s)
            {
              const double dt = chrono.delta();
              last_average_frametime = (dt / frame_cnt);
//               cr::out().debug("{}s avg: ms/frame: {:6.3}ms [{:.2f} fps]", max_dt_s, (dt * 1000 / frame_cnt), (frame_cnt / dt));
              frame_cnt = 0;
            }
          });
        });
        hctx->tm.set_start_task_group_callback("after_render"_rid, [this]
        {
          // check to see if we need to close the window:
          if (window_state->window.should_close())
          {
            cr::out().debug("main window should close, requesting an engine tear-down");
            engine->sync_teardown();
            return;
          }
        });
      }

      void on_start_shutdown() override
      {
        window_state.reset();
      }


    private:
      std::unique_ptr<glfw::glfw_module::state_ref_t> window_state;

      uint32_t total_frame_cnt = 0;
      uint32_t frame_cnt = 0;
      float last_average_frametime = 0;
      cr::chrono chrono;
      bool frametime_limiter = true;

      imgui_log_window log_window;

      friend class engine_t;
      friend engine_module<app_module>;
  };
}

int main(int, char **)
{
  neam::cr::out.min_severity = neam::cr::logger::severity::debug/*message*/;
  neam::cr::out.register_callback(neam::cr::print_log_to_console, nullptr);
  neam::cr::out().log("app start");

  glfwInit();

  {
    neam::hydra::engine_t engine;
    engine.boot(neam::hydra::runtime_mode::hydra_context, "caca"_rid);

    // make the main thread participate in the task manager
    neam::hydra::core_context::thread_main(engine.get_core_context());
  }

  glfwTerminate();

  return 0;
}

