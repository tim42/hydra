
// #include <neam/reflective/reflective.hpp>

// #define N_ALLOW_DEBUG true // we want full debug information
// #define N_DISABLE_CHECKS 1 // we don't want anything
#define HYDRA_AUTO_BUFFER_NO_SMART_SYNC
// #include "sample.hpp"

#include <math.h>
#include <ntools/rpc/rpc.hpp>
#include <ntools/chrono.hpp>
#include <ntools/struct_metadata/fmt_support.hpp>
#include <ntools/rolling_average.hpp>
#include <ntools/cmdline/cmdline.hpp>

#include <hydra/engine/engine.hpp>
#include <hydra/glfw/glfw_window.hpp>
#include <hydra/glfw/glfw_events.hpp>
#include <hydra/hydra_glm_udl.hpp>  // we do want the easy glm udl (optional, not used by hydra)
#include <hydra/glfw/glfw_engine_module.hpp>  // Add the glfw module
#include <hydra/imgui/imgui_engine_module.hpp>  // Add the imgui module
#include <hydra/imgui/generic_ui.hpp>
#include <hydra/engine/core_modules/core_module.hpp>

#include <hydra/imgui/utilities/imgui_log_window.hpp>
#include "fs_quad_pass.hpp"
#include <hydra/ecs/universe.hpp>
//#include "mesh-render-pass.hpp"

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
        tgd.add_task_group("during_render"_rid);
        tgd.add_task_group("after_render"_rid);
      }
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_dependency("during_render"_rid, "glfw/events"_rid);
        tgd.add_dependency("after_render"_rid, "render"_rid);
      }

      void on_context_initialized() override
      {
        auto* core = engine->get_module<core_module>("core"_rid);
        auto* renderer = engine->get_module<renderer_module>("renderer"_rid);
        renderer->min_frame_time = 0.0f;

        ser_data = rle::serialize(data_sample_t{});
        ser_metadata = rle::generate_metadata<data_sample_t>();

        // we actualy create the window while the index is loading.
        // This may lead to cases where the window gets created before the index fail to load and needs to be closed.
        cr::out().debug("creating application main window/render-context...");
        auto* glfw_mod = engine->get_module<glfw::glfw_module>("glfw"_rid);
        window_state = glfw_mod->create_window(800_uvec2_xy);
        cr::out().debug("created application main window and render-context");
        // FIXME: Fix fs-quad pass
        {
          std::lock_guard _el(spinlock_exclusive_adapter::adapt(window_state.render_entity.get_lock()));
          window_state.render_entity.add<hydra::ecs::name_component>("main-window-state");
        }
//        window_state->_ctx_ref.debug_context = "window-state";
//        window_state->_ctx_ref.clear_framebuffer = true;

 //       fb_test = renderer->create_render_context<offscreen_render_context_t>(std::vector<VkFormat>{VK_FORMAT_R8G8B8A8_UNORM}, 100_uvec2_xy);

      //  /*fs_quad_pass& pass = */fb_test->pm.add_pass<fs_quad_pass>(*hctx);
 //       fb_test->debug_context = "fb_test";
        // fb_test->pm.add_pass<hydra::mesh_pass>(*hctx, db);
#if 0
        using transform = hydra::ecs::components::transform;
        transform* root_tr;
        {
          // generate some entities:
          using hierarchy = hydra::ecs::components::hierarchy;
          hierarchy& uni_root = universe.get_universe_root();
          hydra::ecs::entity root = uni_root.create_child().generate_strong_reference();
          transform& root_transform = root.add<transform>();
          root_tr = &root_transform;
          root_transform.update_local_transform().translation.z = -10;
          hierarchy& root_hc = *root.get<hierarchy>();

          for (int32_t x = -100; x <= 100; ++x)
          {
            for (int32_t y = -100; y <= 100; ++y)
            {
              hydra::ecs::entity chld = root_hc.create_child().generate_strong_reference();
              transform& chld_transform = chld.add<transform>();
              chld_transform.update_local_transform().translation = glm::dvec3(x/10.0f, y/10.0f, 1.25f);
              chld_transform.update_local_transform().scale = 0.08f;
              // chld_transform.update_local_transform().translation = glm::dvec3(x/2.0f, y/2.0f, 1.25f);
              // chld_transform.update_local_transform().scale = 0.2f;
            }
          }

          // update the db
          db.apply_component_db_changes();
          // update the hierarchy (which includes the transform components)
          universe.hierarchical_update_single_thread();
        }
#endif
        auto* imgui = engine->get_module<imgui::imgui_module>("imgui"_rid);
        imgui->create_context(window_state);
        imgui->register_function("dockspace"_rid, [this]()
        {
          ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        });
        imgui->register_function("main"_rid, [this, core, glfw_mod/*, &pass*/]()
        {
          ImGui::Begin("Conf", nullptr, 0);
          float width = ImGui::GetContentRegionAvail().x;

          // We can reference textures directly with a resource-id
          // Hydra will stream it as needed (when it's visible in imgui)
          ImGui::Image("images/hydra-logo.png.xz/hydra-logo.png:image"_rid, ImVec2(width, width / 6));

          ImGui::Text("Avg Frame Time: %.3f ms [etr: %.3f ms | %u ent.]", last_average_frametime * 1000, last_update_dt * 1000, entity_cnt/*(uint32_t)db.get_entity_count()*/);
          ImGui::Text("Avg FPS: %.3f f/s", 1.0f / last_average_frametime);
          ImGui::Text("Allocated pages: %u (total alloc: %u)", memory::statistics::get_current_allocated_page_count(), memory::statistics::get_total_allocated_page_count());
          ImGui::Text("Allocated gpu memory: %.3f", (hctx->allocator.get_reserved_memory() / 1024 / 1024) / 1024.0f);
          ImGui::Text("texture manager gpu memory: %.3f", (hctx->textures.get_total_gpu_memory() / 1024 / 1024) / 1024.0f);
          {
            ImGui::Text("Limit framerate:"); ImGui::SameLine(); ImGui::Checkbox("##limit-fps", &frametime_limiter);
            ImGui::SameLine();
            const uint32_t min = 0;
            const uint32_t max = 100;
            ImGui::SliderScalar("##frametime-ms", ImGuiDataType_U32, &framerate_limit_ms, &min, &max);
            ImGui::SameLine();
            ImGui::Text("ms");
          }
         // FIXME ImGui::Text("Only render on event:"); ImGui::SameLine(); ImGui::Checkbox("##render-on-evt", &window_state->only_render_on_event);
          bool wait_for_events = glfw_mod->get_wait_for_events();
          ImGui::Text("Wait for events:"); ImGui::SameLine(); ImGui::Checkbox("##wait-for-evt", &wait_for_events);
          glfw_mod->wait_for_events(wait_for_events);
          const bool bt_reload_index = ImGui::Button("force full index reload"); ImGui::SameLine(); ImGui::Checkbox("##reload-index-ck", &force_relad_index);
          if (bt_reload_index || force_relad_index)
          {
            core->ask_for_index_reload();
            //[[maybe_unused]] auto c = hctx->res.reload_index();
          }
          ImGui::Separator();
          {
            if (ImGui::Button("open new window"))
            {
              window_states.emplace_back(glfw_mod->create_window(80_uvec2_xy, fmt::format("[HYDRA: WIN {}]", window_states.size() + 1)));
              {
                auto& re = window_states.back().render_entity;
                std::lock_guard _lg{spinlock_exclusive_adapter::adapt(re.get_lock())};
                re.add<components::fs_quad_pass>(*hctx);
              }
            }
          }
          ImGui::Separator();
          const uint32_t cnt = total_frame_cnt / std::max(1u, frames_between_image_changes);
          const uint32_t min = 0;
          const uint32_t max = 10000;
          ImGui::Text("frames between image changes:"); ImGui::SameLine(); ImGui::SliderScalar("##frame-change-img", ImGuiDataType_U32, &frames_between_image_changes, &min, &max);
          uint32_t img = cnt%64 + 2;
#if 0
          ImGui::Image(string_id::_runtime_build_from_string(fmt::format("images/_dns_images/{}/{}.png:image", (cnt/64) % 12 + 1, img == 33||img>64 ? 32 : img)), ImVec2(width, width));
#endif
          ImGui::Separator();
          ser_data = hydra::imgui::generate_ui(ser_data, ser_metadata);
          ImGui::End();
        });
        imgui->register_function("demo"_rid, [this]()
        {
          ImGui::ShowDemoWindow();
          ImPlot::ShowDemoWindow();
        });
        imgui->register_function("log_window"_rid, [this]()
        {
          log_window.show_log_window();
        });
        imgui->register_function("stats"_rid, [this]()
        {
          ImGui::Begin("Stats", nullptr, 0);
          const auto& stats = hctx->tm.get_last_frame_stats();

          if (stats.frame_duration > 0)
          {
            frame_times.add_value(stats.frame_duration);

            static constexpr uint32_t k_rolling_average_size = 100;
            task_group_rolling_averages_start.resize(stats.task_groups.size(), { k_rolling_average_size });
            task_group_rolling_averages_end.resize(stats.task_groups.size(), { k_rolling_average_size });
            task_group_rolling_averages_duration.resize(stats.task_groups.size(), { k_rolling_average_size });
            for (uint32_t grp = 0; grp < (uint32_t)stats.task_groups.size(); ++grp)
            {
              if (grp == threading::k_non_transient_task_group) continue;
              const auto& it = stats.task_groups[grp];

              task_group_rolling_averages_start[grp].add_value(it.start);
              task_group_rolling_averages_end[grp].add_value(it.end);
              task_group_rolling_averages_duration[grp].add_value(it.end - it.start);
            }
            if ((total_frame_cnt % frame_times.total_size()) == 0 && total_frame_cnt > 0 && frame_times.size() == frame_times.total_size())
            {
              frame_times_avgs.add_value({frame_times.get_min() * 1e3f, frame_times.get_average() * 1e3f, frame_times.get_max() * 1e3f});
            }

            if (frame_times_avgs.size() > 0)
            {
              ImGui::TextUnformatted(fmt::format("frame time: [{:.3f} / {:.3f} / {:.3f}] | {:.3f} ms",
                                                frame_times.get_min() * 1e3f, frame_times.get_average() * 1e3f, frame_times.get_max() * 1e3f,
                                                stats.frame_duration * 1e3f).c_str());
              ImGui::TextUnformatted(fmt::format("  graph range: {:.3f} ms to {:.3f} ms",
                                                (frame_times_avgs.get_min().x), (frame_times_avgs.get_max().z)).c_str());
              ImGui::TextUnformatted(fmt::format("  graph avg min/avg/max: {:.3f} ms / {:.3f} ms / {:.3f} ms",
                                                (frame_times_avgs.get_average().x), (frame_times_avgs.get_average().y), (frame_times_avgs.get_average().z)).c_str());
            }
            ImGui::SliderFloat("Graph Vertical Scale", &ui_graph_scale, 0, 1);
            if (ImPlot::BeginPlot("##FrameTime", ImVec2(-1, 150), ImPlotFlags_NoLegend|ImPlotFlags_NoInputs))
            {
              if (frame_times_avgs.size() > 0)
              {
                constexpr ImPlotAxisFlags flags = ImPlotAxisFlags_AutoFit;
                ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels | flags, flags);
                ImPlot::SetupAxisLimits(ImAxis_X1, 0, frame_times_avgs.size(), ImGuiCond_Always);
                // ImPlot::SetupAxisLimits(ImAxis_Y1, 0, (frame_times_avgs.get_average().z) * 1.5f, ImGuiCond_Always);
                // ImPlot::SetupAxisLimits(ImAxis_Y1, 0, glm::mix(frame_times_avgs.get_average().z, frame_times_avgs.get_max().z, ui_graph_scale), ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, glm::mix(frame_times_avgs.get_average().x, frame_times_avgs.get_min().x, ui_graph_scale),
                                                   glm::mix(frame_times_avgs.get_average().z, frame_times_avgs.get_max().z, ui_graph_scale), ImGuiCond_Always);

                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

                ImPlotGetter get_min = [](int idx, void* d) { return ImPlotPoint{(double)idx, (*(decltype(frame_times_avgs)*)(d))[idx].x}; };
                ImPlotGetter get_max = [](int idx, void* d) { return ImPlotPoint{(double)idx, (*(decltype(frame_times_avgs)*)(d))[idx].z}; };

                ImPlot::PlotShadedG("time", get_min, &frame_times_avgs, get_max, &frame_times_avgs,
                                  frame_times_avgs.size());
                ImPlot::PlotLine("time", &frame_times_avgs.inner_data()[0].y, frame_times_avgs.size(), 1, 0, 0, frame_times_avgs.first_element_offset(), sizeof(*frame_times_avgs.inner_data()));
                ImPlot::PopStyleVar();
              }
              ImPlot::EndPlot();
            }

            // ImPlot::
            {
              if (ImGui::BeginTable("##stats-table", 3, ImGuiTableFlags_BordersInner | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
              {
                const float rcp_graph_scale = 1.0f / (frame_times_avgs.size() == 0 ? 1.0f : glm::mix(frame_times_avgs.get_average().z, frame_times_avgs.get_max().z, ui_graph_scale) / 1e3f);
                ImDrawList& draw_list = *ImGui::GetWindowDrawList();
                for (uint32_t grp = 0; grp < (uint32_t)stats.task_groups.size(); ++grp)
                {
                  if (grp == threading::k_non_transient_task_group) continue;
                  const auto& it = stats.task_groups[grp];

                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  {
                    ImGui::TextUnformatted(hctx->tm.get_task_group_name(grp).data());
                  }
                  ImGui::TableNextColumn();
                  {
                    ImGui::TextUnformatted(fmt::format("{:.3f} us", task_group_rolling_averages_duration[grp].get_average() * 1e6).c_str());
                  }
                  ImGui::TableNextColumn();
                  {
                    const float width = ImGui::GetContentRegionAvail().x * rcp_graph_scale;
                    ImVec2 p = ImGui::GetCursorScreenPos();
                    // worst
                    {
                      p.y += 4;
                      const float start = task_group_rolling_averages_start[grp].get_min() * width;
                      const float end = start + task_group_rolling_averages_duration[grp].get_max() * width;
                      //const float end = task_group_rolling_averages_end[grp].get_max() * width;
                      draw_list.AddLine(ImVec2(p.x + start, p.y), ImVec2(p.x + end, p.y), 0x33000000, 20);
                    }
                    // average
                    {
                      const float start = task_group_rolling_averages_start[grp].get_average() * width;
                      const float end = start + task_group_rolling_averages_duration[grp].get_average() * width;
                      //const float end = task_group_rolling_averages_end[grp].get_average() * width;
                      draw_list.AddLine(ImVec2(p.x + start, p.y), ImVec2(p.x + end, p.y), 0xAA999999, 8);
                    }
                    // immediate
                    {
                      const float start = it.start  * width;
                      const float end = it.end  * width;
                      draw_list.AddLine(ImVec2(p.x + start, p.y), ImVec2(p.x + end, p.y), 0xFFFFFFFF, 2);
                    }
                  }
                }

                ImGui::EndTable();
              }

            }
          }
          ImGui::End();
        });


        imgui->register_function("framebuffer"_rid, [this]()
        {
          ImGui::Begin("Framebuffer", nullptr, 0);
          ImVec2 size = ImGui::GetContentRegionAvail();
          // if (!fb_test->images_views.empty())
          // {
          //   ImGui::Image(&(fb_test->images_views[0]), size);
          //   const glm::uvec2 new_size { std::max(1, (int)size.x), std::max(1, (int)size.y) };
          //   if (fb_test->size != new_size)
          //   {
          //     fb_test->size = new_size;
          //     fb_test->recreate = true;
          //   }
          // }
          ImGui::End();
        });
        // setup the debug + render task
        hctx->tm.set_start_task_group_callback("during_render"_rid, [this, glfw_mod, core]
        {
          cctx->tm.get_task([this, glfw_mod, core]
          {
            // if (!glfw_mod->has_any_window() || glfw_mod->need_render())
            //   return;

/*            if (!glfw_mod->is_app_focused() && !glfw_mod->get_wait_for_events())
              core->min_frame_length = std::chrono::milliseconds(500);
            else */if (frametime_limiter)
              core->min_frame_length = std::chrono::milliseconds(framerate_limit_ms);
            else
              core->min_frame_length = std::chrono::milliseconds(0);

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
          for (auto it = window_states.begin(); it < window_states.end();)
          {
            if (it->win->should_close())
            {
              it = window_states.erase(it);
            }
            else
            {
              ++it;
            }
          }
          // check to see if we need to close the window:
          if (window_state.win->should_close())
          {
            cr::out().debug("main window should close, requesting an engine tear-down");
            engine->sync_teardown();
            return;
          }
        });

        on_render_start_tk = renderer->on_render_start.add([this, renderer/*, root_tr*/]
        {
          cctx->tm.get_task([this, renderer/*, root_tr*/]
          {
            // root_tr->update_local_transform().rotation = glm::angleAxis(glm::radians(1.0f), glm::vec3(0, 0.8f, 1)) * root_tr->update_local_transform().rotation;

            // update the db
            // db.apply_component_db_changes();

            {
              // const std::chrono::time_point start = std::chrono::high_resolution_clock::now();

              // update the hierarchy (which includes the transform components)
              // universe.hierarchical_update_single_thread();

              // entity_cnt = universe.hierarchical_update_tasked(*cctx, cctx->get_thread_count() - 0, 1);

              // const std::chrono::time_point now = std::chrono::high_resolution_clock::now();
              // const std::chrono::nanoseconds elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start);
              // last_update_dt = (float)((double)elapsed.count() * 1e-9);
            }

         //   fb_test->debug_context = fmt::format("fb_test[{}]", total_frame_cnt);

          //  renderer->do_render(fb_test);
          });
        });

        on_index_reloaded_tk = hctx->res.on_index_loaded.add([this]()
        {
          // reload shaders / fonts:
          auto* imgui = engine->get_module<imgui::imgui_module>("imgui"_rid);
          imgui->reload_fonts();
        });
      }

      void on_shutdown_post_idle_gpu() override
      {
        window_state = {};
        window_states.clear();
   //     fb_test.reset();
        on_render_start_tk.release();
      }


    private:
      glfw::window_state_t window_state;
      //render_context_ptr<offscreen_render_context_t> fb_test;

      //hydra::ecs::database db;
      //hydra::ecs::universe universe { db };

      cr::event_token_t on_render_start_tk;
      cr::event_token_t on_index_reloaded_tk;

      cr::rolling_average<float> frame_times { 50 };
      cr::rolling_average<glm::vec3> frame_times_avgs { 50 };
      std::vector<cr::rolling_average<float>> task_group_rolling_averages_start;
      std::vector<cr::rolling_average<float>> task_group_rolling_averages_end;
      std::vector<cr::rolling_average<float>> task_group_rolling_averages_duration;

      std::vector<glfw::window_state_t> window_states;

      uint32_t total_frame_cnt = 0;
      uint32_t frame_cnt = 0;
      uint32_t entity_cnt = 0;
      uint32_t framerate_limit_ms = 12;
      float last_average_frametime = 0;
      float last_update_dt = 0;
      cr::chrono chrono;
      bool frametime_limiter = false;
      bool force_relad_index = false;
      uint32_t frames_between_image_changes = 10;
      float ui_graph_scale = 1;

      imgui_log_window log_window;

      friend class engine_t;
      friend engine_module<app_module>;
  };
}

struct global_options
{
  // options
  bool silent = false;
  bool debug = false;
  bool help = false;
  uint32_t thread_count = std::thread::hardware_concurrency() - 4;
};
N_METADATA_STRUCT(global_options)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(silent, neam::metadata::info{.description = c_string_t<"Only show log (and above) messages.">}),
    N_MEMBER_DEF(help, neam::metadata::info{.description = c_string_t<"Print this message and exit.">}),
    N_MEMBER_DEF(debug, neam::metadata::info{.description = c_string_t<"Enable debug mode (vulkan validation layer and other debug features).">}),
    N_MEMBER_DEF(thread_count, neam::metadata::info{.description = c_string_t<"Number of thread the task manager will launch.">})
  >;
};

int main(int argc, char** argv)
{
  neam::cr::get_global_logger().min_severity = neam::cr::logger::severity::debug/*message*/;
  neam::cr::get_global_logger().register_callback(neam::cr::print_log_to_console, nullptr);

  cmdline::parse cmd(argc, argv);
  bool success;
  global_options gbl_opt = cmd.process<global_options>(success);
  if (!success || gbl_opt.help)
  {
    // output the different options and exit:
    cr::out().warn("usage: {} [options] [index_key] [data_folder]", argv[0]);
    cr::out().log("possible options:");
    cmdline::arg_struct<global_options>::print_options();
    return 1;
  }
  if (gbl_opt.silent)
    neam::cr::get_global_logger().min_severity = neam::cr::logger::severity::message;

  neam::cr::out().log("app start");
  {
    neam::hydra::engine_t engine;
    neam::hydra::engine_settings_t settings = engine.get_engine_settings();
    settings.vulkan_device_preferences = neam::hydra::hydra_device_creator::prefer_discrete_gpu;
    settings.thread_count = gbl_opt.thread_count;
    engine.set_engine_settings(settings);

    neam::hydra::runtime_mode rm = neam::hydra::runtime_mode::hydra_context;
    if (!gbl_opt.debug)
      rm |= neam::hydra::runtime_mode::release;

    engine.boot(rm, {.index_key = "caca"_rid, .index_file = "root.index", .argv0 = argv[0]});
    // engine.boot(neam::hydra::runtime_mode::hydra_context | neam::hydra::runtime_mode::release, {.index_key = "caca"_rid, .index_file = "root.index"});

    hydra::core_context& cctx = engine.get_core_context();
    cctx.hconf.register_watch_for_changes();

    // make the main thread participate in the task manager
    cctx.enroll_main_thread();
  }

  return 0;
}

