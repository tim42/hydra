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

#include "engine.hpp"

#include <hydra/init/bootstrap.hpp>
#include <hydra/init/feature_requesters/gen_feature_requester.hpp>

namespace neam::hydra
{
  resources::context::status_chain engine_t::boot(runtime_mode _mode, id_t index_key, const std::string& index_file)
  {
    cr::out().debug("engine: starting the boot process for mode {:X} and index {}", std::to_underlying(_mode), index_file);

    if (mode != runtime_mode::none)
    {
      check::debug::n_check(false, "trying to boot an already init engine");
      return resources::context::status_chain::create_and_complete(resources::status::failure);
    }

    if ((_mode & runtime_mode::context_flags) == runtime_mode::none)
    {
      check::debug::n_check(false, "trying to boot an engine with no context");
      return resources::context::status_chain::create_and_complete(resources::status::failure);
    }

    // very first operation
    mode = _mode;

    // start by filtering and creating engine modules (they are necessary for the vk instance creation)
    // We don't set the engine yet, as it's only done at a later stage
    {
      cr::out().debug("creating engine modules...");
      check::debug::n_check(modules.empty(), "engine_t::boot(): invalid state");
      modules.clear();

      std::vector filtered_modules = module_manager::filter_modules(mode);
      for (auto& it : filtered_modules)
      {
        const id_t name = string_id::_runtime_build_from_string(it.name.data(), it.name.size());
        if (!check::debug::n_check(!modules.contains(name), "duplicate engine module name found: {}", it.name))
          continue;
        cr::out().debug("  adding engine module: {}", it.name);
        modules.emplace(name, it.create());
      }
      cr::out().debug("creating engine modules: {} modules created", modules.size());
    }

    for (auto& mod : modules)
      mod.second->set_engine(this);

    // create the vulkan instance then create the actual context (vk or hydra)
    if ((mode & runtime_mode::vulkan_context) != runtime_mode::none)
    {
      cr::out().debug("creating vulkan instance...");
      neam::hydra::gen_feature_requester gfr;
      neam::hydra::bootstrap hydra_init;

      neam::hydra::temp_queue_familly_id_t* temp_graphic_queue = nullptr;
      neam::hydra::temp_queue_familly_id_t* temp_transfer_queue = nullptr;
      neam::hydra::temp_queue_familly_id_t* temp_compute_queue = nullptr;

      // request the queues:
      temp_transfer_queue = gfr.require_queue_capacity(VK_QUEUE_TRANSFER_BIT,
                            [=,this](vk::instance& instance, size_t qindex, const neam::hydra::vk::physical_device & gpu)
      {
        for (auto& mod : modules)
        {
          if (!mod.second->filter_queue(instance, VK_QUEUE_TRANSFER_BIT, qindex, gpu))
            return false;
        }
        return true;
      }, false);
      temp_compute_queue = gfr.require_queue_capacity(VK_QUEUE_COMPUTE_BIT,
                           [=,this](vk::instance& instance, size_t qindex, const neam::hydra::vk::physical_device & gpu)
      {
        for (auto& mod : modules)
        {
          if (!mod.second->filter_queue(instance, VK_QUEUE_COMPUTE_BIT, qindex, gpu))
            return false;
        }
        return true;
      }, false);
      temp_graphic_queue = gfr.require_queue_capacity(VK_QUEUE_GRAPHICS_BIT,
                           [=,this](vk::instance& instance, size_t qindex, const neam::hydra::vk::physical_device & gpu)
      {
        for (auto& mod : modules)
        {
          if (!mod.second->filter_queue(instance, VK_QUEUE_GRAPHICS_BIT, qindex, gpu))
            return false;
        }
        return true;
      }, false);

      // request validation stuff if not in release:
      // NOTE: Might be better off in a module
      if ((mode & runtime_mode::release) == runtime_mode::none)
      {
        gfr.require_instance_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        gfr.require_instance_layer("VK_LAYER_KHRONOS_validation");
      }

      // loop over the modules for them to request stuff:
      for (auto& mod : modules)
        mod.second->init_vulkan_interface(gfr, hydra_init);

      hydra_init.register_feature_requester(gfr);

      // create the actual vulkan instance:
      vk::instance vk_instance = hydra_init.create_instance(fmt::format("hydra-engine[{:X}]", std::to_underlying(mode)));
      cr::out().debug("created vulkan instance");

      if ((mode & runtime_mode::release) == runtime_mode::none)
      {
        vk_instance.install_default_debug_callback(VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT);
      }

      // instance is created, we can get the actual queues:
      neam::hydra::temp_queue_familly_id_t graphic_queue = *temp_graphic_queue;
      neam::hydra::temp_queue_familly_id_t transfer_queue = *temp_transfer_queue;
      neam::hydra::temp_queue_familly_id_t compute_queue = *temp_compute_queue;

      // create the actual context:
      if ((mode & runtime_mode::hydra_context) != runtime_mode::none)
        context.emplace<hydra_context>(std::move(vk_instance), hydra_init, graphic_queue, transfer_queue, compute_queue);
      else // vk_context:
        context.emplace<internal_vk_context>(std::move(vk_instance), hydra_init, graphic_queue, transfer_queue, compute_queue);
    }
    else // core context
    {
      context.emplace<core_context>();
    }

    // the core context is guranteed to exist:
    core_context& cctx = get_core_context();

    // set the contextes in modules:
    {
      for (auto& mod : modules)
        mod.second->set_core_context(&cctx);
    }
    if ((mode & runtime_mode::vulkan_context) != runtime_mode::none)
    {
      vk_context& vctx = get_vulkan_context();
      for (auto& mod : modules)
        mod.second->set_vk_context(&vctx);
    }
    if ((mode & runtime_mode::hydra_context) != runtime_mode::none)
    {
      hydra_context& hctx = get_hydra_context();
      for (auto& mod : modules)
        mod.second->set_hydra_context(&hctx);
    }

    // setup the task groups:
    threading::task_group_dependency_tree tgd;

    // add the pre-made task groups and their dependencies:
    tgd.add_task_group("init"_rid, "init");
    tgd.add_task_group("io"_rid, "io");

    tgd.add_dependency("io"_rid, "init"_rid);

    // add the module task groups / their dependencies:
    for (auto& mod : modules)
      mod.second->add_task_groups(tgd);
    for (auto& mod : modules)
      mod.second->add_task_groups_dependencies(tgd);

    // boot the core context:
    std::lock_guard _lg (init_lock);
    auto ret = cctx.boot(tgd.compile_tree(), index_key, index_file).then(&cctx.tm, threading::k_non_transient_task_group, [this, &cctx](resources::status st)
    {
      cr::out().debug("engine boot: index loaded");
      if (st == resources::status::failure)
      {
        cr::out().debug("engine boot: failed to load index, tearing-down the engine");
        sync_teardown();
        return st;
      }

      // NOTE: partial success is ok

      std::lock_guard _lg(init_lock);

      for (auto& mod : modules)
        mod.second->on_resource_index_loaded();

      cr::out().debug("engine boot: engine has fully booted");
      return st;
    });

    for (auto& mod : modules)
      mod.second->on_context_initialized();
    cr::out().debug("engine boot: modules initialized, waiting for index load");

    return ret;
  }

  void engine_t::sync_teardown()
  {
    std::lock_guard _lg(init_lock);
    if (mode == runtime_mode::none)
      return;

    core_context& cctx = get_core_context();
    if (cctx.is_stopped() && modules.empty())
      return;
    if ((mode & runtime_mode::vulkan_context) != runtime_mode::none)
    {
      vk_context& vctx = get_vulkan_context();
      vctx.device.wait_idle();
    }
    cr::out().debug("engine tear-down: stopping the task manager...");
    cctx.stop_app();

    cr::out().debug("engine tear-down: clearing remaining tasks...");
    while (cctx.tm.has_pending_tasks())
      cctx.tm.run_a_task();

    cr::out().debug("engine tear-down: destructing modules...");
    // clear the modules
    modules.clear();

    // NOTE: we cannot destroy the context here as we might be in a task.

    // very last operation
    mode = runtime_mode::none;
  }
}

