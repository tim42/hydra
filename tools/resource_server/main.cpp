

#include <hydra/resources/resources.hpp>
#include <ntools/cmdline/cmdline.hpp>
#include <hydra/hydra_debug.hpp>
#include <hydra/engine/core_context.hpp>
#include <hydra/engine/engine.hpp>
#include <hydra/glfw/glfw.hpp>

#include "fs_watcher.hpp"
#include "packer_engine_module.hpp"
#include "packer_ui.hpp"

#include "options.hpp"


using namespace neam;

void make_index(const global_options& gbl_opt)
{
  hydra::core_context ctx;
  ctx.res.source_folder = gbl_opt.source_folder;
  ctx.io.set_prefix_directory(gbl_opt.build_folder);

  if (gbl_opt.force || !std::filesystem::exists(gbl_opt.index))
  {
    cr::out().log("Creating a new index...");

    std::filesystem::create_directory(gbl_opt.build_folder / "pack");
    ctx.res.make_self_boot(gbl_opt.index_key, gbl_opt.index.lexically_relative(gbl_opt.build_folder), { .prefix_path = "pack" });
    ctx.io._wait_for_submit_queries();
  }
}

// BENCHMARK: 4420 png images (w/ LZMA), 60k total files, hot fs cache
// ryzen 5600x: (filesystem access is negligeable as it's mostly a compression benchmark)
//           ~695s, single threaded
//           ~257s, 4 threads
//           ~150s, 8 threads

int main(int argc, char **argv)
{
  cr:: get_global_logger().min_severity = cr::logger::severity::message;
  cr:: get_global_logger().register_callback(cr::print_log_to_console, nullptr);

  // parse the commandline options:
  cmdline::parse cmd(argc, argv);
  bool success;
  global_options gbl_opt = cmd.process<global_options>(success, 2 /*index_key, data_folder*/);
  if (!success || gbl_opt.parameters.size() > 2 || gbl_opt.help)
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

  // parse/setup parameters:
  if (gbl_opt.parameters.size() > 0)
    gbl_opt.index_key = string_id::_runtime_build_from_string(gbl_opt.parameters[0].data(), gbl_opt.parameters[0].size());
  if (gbl_opt.parameters.size() > 1)
    gbl_opt.data_folder = gbl_opt.parameters[1].data();

  if (!std::filesystem::exists(gbl_opt.data_folder))
  {
    cr::out().error("Specified data folder {} does not exist", gbl_opt.data_folder.c_str());
    return 2;
  }
  if (!std::filesystem::is_directory(gbl_opt.data_folder))
  {
    cr::out().error("Specified data folder {} is not a directory", gbl_opt.data_folder.c_str());
    return 2;
  }

  // filesystem setup:
  gbl_opt.source_folder = gbl_opt.data_folder / "source";
  gbl_opt.build_folder = gbl_opt.data_folder / "build";
  gbl_opt.index = gbl_opt.build_folder / "root.index";
  gbl_opt.ts_file = gbl_opt.build_folder / "last_build_ts";

  cr::out().debug("Data directory: {}", gbl_opt.data_folder.c_str());
  cr::out().debug("Source directory: {}", gbl_opt.source_folder.c_str());
  cr::out().debug("Build directory: {}", gbl_opt.build_folder.c_str());
  cr::out().debug("Index: {}", gbl_opt.index.c_str());

  if (!std::filesystem::exists(gbl_opt.source_folder) || !std::filesystem::is_directory(gbl_opt.source_folder))
  {
    cr::out().error("Source folder ({}) is not valid. Refusing to operate.", gbl_opt.source_folder.c_str());
    return 2;
  }
  if (!gbl_opt.force && !std::filesystem::exists(gbl_opt.index))
  {
    cr::out().log("Forcing a full rebuild: index does not exist");
    gbl_opt.force = true;
  }
  if (!gbl_opt.force && !std::filesystem::exists(gbl_opt.ts_file))
  {
    cr::out().log("Forcing a full rebuild: timestamp file does not exist");
    gbl_opt.force = true;
  }

  // just in case:
  std::filesystem::create_directory(gbl_opt.build_folder);

  make_index(gbl_opt);

  {
    neam::hydra::engine_t engine;

    neam::hydra::engine_settings_t settings = engine.get_engine_settings();
    settings.vulkan_device_preferences = neam::hydra::hydra_device_creator::prefer_integrated_gpu;
    settings.thread_count = gbl_opt.thread_count;
    engine.set_engine_settings(settings);

    neam::hydra::runtime_mode engine_mode = neam::hydra::runtime_mode::core;
    if (gbl_opt.ui)
      engine_mode = neam::hydra::runtime_mode::hydra_context;
    if (!gbl_opt.debug)
      engine_mode |= neam::hydra::runtime_mode::release;

    engine.init(engine_mode);
    hydra::core_context& cctx = engine.get_core_context();
    {
      cctx.res.source_folder = gbl_opt.source_folder;

      cctx.hconf.register_watch_for_changes();

      auto* pck = engine.get_module<neam::hydra::packer_engine_module>("packer"_rid);
      pck->packer_options = gbl_opt;
    }

    engine.boot({.index_key = gbl_opt.index_key, .index_file = gbl_opt.index, .argv0 = argv[0]});

    // make the main thread participate in the task manager
    cctx.enroll_main_thread();
  }

  return 0;
}

