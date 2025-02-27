

#include <hydra/resources/resources.hpp>
#include <ntools/fmt_utils.hpp>
#include <ntools/cmdline/cmdline.hpp>
#include <hydra/hydra_debug.hpp>
#include <hydra/engine/core_context.hpp>
#include <hydra/engine/engine.hpp>
#include <hydra/glfw/glfw.hpp>

#include "options.hpp"
#include "packer_engine_module.hpp"


using namespace neam;


int main(int argc, char **argv)
{
  cr:: get_global_logger().min_severity = cr::logger::severity::message;
  cr:: get_global_logger().register_callback(cr::print_log_to_console, nullptr);

  // parse the commandline options:
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

  // setup parameters:
  gbl_opt.index_key = string_id::_runtime_build_from_string(gbl_opt.key.data(), gbl_opt.key.size());

  // filesystem setup:
  cr::out().debug("Source directory: {}", gbl_opt.source_folder.c_str());
  cr::out().debug("Output: {}", gbl_opt.output);

  if (!std::filesystem::exists(gbl_opt.source_folder) || !std::filesystem::is_directory(gbl_opt.source_folder))
  {
    cr::out().error("Source folder ({}) is not valid. Refusing to operate.", gbl_opt.source_folder.c_str());
    return 2;
  }

  {
    neam::hydra::engine_t engine;

    neam::hydra::engine_settings_t settings = engine.get_engine_settings();
    settings.vulkan_device_preferences = neam::hydra::hydra_device_creator::prefer_integrated_gpu;
    engine.set_engine_settings(settings);

    neam::hydra::runtime_mode engine_mode = neam::hydra::runtime_mode::core;

    engine.init(engine_mode);
    hydra::core_context& cctx = engine.get_core_context();
    {
      cctx.res.source_folder = gbl_opt.source_folder;

      auto* pck = engine.get_module<neam::hydra::packer_engine_module>("packer"_rid);
      pck->files_to_pack = gbl_opt.parameters;
      pck->packer_options = gbl_opt;
    }
    engine.boot({.mode = neam::hydra::index_boot_parameters_t::init_empty_index, .index_key = gbl_opt.index_key, .argv0 = argv[0]});

    // make the main thread participate in the task manager
    cctx.enroll_main_thread();
  }

  return 0;
}

