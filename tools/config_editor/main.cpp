

#include <hydra/resources/resources.hpp>
#include <ntools/fmt_utils.hpp>
#include <ntools/cmdline/cmdline.hpp>
#include <hydra/hydra_debug.hpp>
#include <hydra/engine/core_context.hpp>
#include <hydra/engine/engine.hpp>
#include <hydra/glfw/glfw.hpp>

#include <hydra/embedded_index.hpp>

#include "options.hpp"
#include "config_editor_module.hpp"


using namespace neam;


int main(int argc, char **argv)
{
  cr:: get_global_logger().min_severity = cr::logger::severity::message;
  cr:: get_global_logger().register_callback(cr::print_log_to_console, nullptr);

  // parse the commandline options:
  cmdline::parse cmd(argc, argv);
  bool success;
  global_options gbl_opt = cmd.process<global_options>(success, 2 /*index_key, data_folder*/);
  if (!success || gbl_opt.help)
  {
    // output the different options and exit:
    cr::out().warn("usage: {} [options] [index_key] [data_folder]", argv[0]);
    cr::out().log("possible options:");
    cmdline::arg_struct<global_options>::print_options();
    return 1;
  }

  // sanity checks:
  if (gbl_opt.thread_count < 1)
  {
    gbl_opt.thread_count = std::thread::hardware_concurrency();
    cr::out().log("Using {} threads", gbl_opt.thread_count);
  }
  if (gbl_opt.thread_count > 2 * std::thread::hardware_concurrency())
  {
    cr::out().warn("the requested thread-count is quite high ({}) compared to the current hardware capabilities ({} threads)",
                   gbl_opt.thread_count, std::thread::hardware_concurrency());
    cr::out().warn("This may lead to lower perfs.");
  }

  // thread_count now contains the number of thread to dispatch
//   gbl_opt.thread_count -= 1;

  // handle some of the options
  if (gbl_opt.verbose)
    cr:: get_global_logger().min_severity = cr::logger::severity::debug;
  if (gbl_opt.silent)
    cr:: get_global_logger().min_severity = cr::logger::severity::warning;

  {
    neam::hydra::engine_t engine;

    neam::hydra::engine_settings_t settings = engine.get_engine_settings();
    settings.vulkan_device_preferences = neam::hydra::hydra_device_creator::prefer_integrated_gpu;
    settings.thread_count = 4;
    engine.set_engine_settings(settings);

    const neam::hydra::runtime_mode engine_mode = neam::hydra::runtime_mode::hydra_context | neam::hydra::runtime_mode::release;

    engine.init(engine_mode);

    {
      auto* ed = engine.get_module<neam::hydra::config_editor_module>("config_editor_module"_rid);
      ed->options = gbl_opt;
    }
    engine.boot(neam::hydra::index_boot_parameters_t::from((neam::id_t)neam::autogen::index::index_key, neam::autogen::index::index_data));

    hydra::core_context& cctx = engine.get_core_context();
    cctx.hconf.register_watch_for_changes();

    // make the main thread participate in the task manager
    cctx.enroll_main_thread();
  }

  return 0;
}

