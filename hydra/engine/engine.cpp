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

#include <ntools/struct_metadata/fmt_support.hpp>

namespace neam::hydra
{
  resources::status engine_t::init(runtime_mode _mode)
  {
    cr::out().debug("engine: initializing engine for mode {:X}", std::to_underlying(_mode));

    if (mode != runtime_mode::none)
    {
      check::debug::n_check(false, "engine: trying to boot an already init engine");
      return resources::status::failure;
    }

    if ((_mode & runtime_mode::context_flags) == runtime_mode::none)
    {
      check::debug::n_check(false, "engine: trying to boot an engine with no context");
      return resources::status::failure;
    }

    // very first operation
    mode = _mode;

    // start by filtering and creating engine modules (they are necessary for the vk instance creation)
    // We don't set the engine yet, as it's only done at a later stage
    {
      cr::out().debug("engine: creating engine modules...");
      check::debug::n_check(modules.empty(), "engine_t::init(): invalid state");
      modules.clear();

      std::vector filtered_modules = module_manager::filter_modules(mode);
      for (auto& it : filtered_modules)
      {
        const id_t name = string_id::_runtime_build_from_string(it.name.data(), it.name.size());
        if (!check::debug::n_check(!modules.contains(name), "engine: duplicate engine module name found: {}", it.name))
          continue;
        cr::out().debug("  adding engine module: {}", it.name);
        modules.emplace(name, it.create());
      }
      cr::out().debug("engine: creating engine modules: {} modules created", modules.size());
    }

    for (auto& mod : modules)
      mod.second->set_engine(this);

    // create the vulkan instance then create the actual context (vk or hydra)
    if ((mode & runtime_mode::vulkan_context) == runtime_mode::vulkan_context)
    {
      cr::out().debug("engine: creating vulkan instance...");
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

      // loop over the modules for them to request stuff:
      for (auto& mod : modules)
        mod.second->init_vulkan_interface(gfr, hydra_init);

      hydra_init.register_feature_requester(gfr);

      // create the actual vulkan instance:
      constexpr VkValidationFeatureEnableEXT vk_validation_to_enable[] =
      {
//         VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
      };
      vk::instance vk_instance = hydra_init.create_instance(fmt::format("hydra-engine[{:X}]", std::to_underlying(mode)), 1,
                                                            VkValidationFeaturesEXT
                                                            {
                                                              .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
                                                              .pNext = nullptr,
                                                              .enabledValidationFeatureCount = sizeof(vk_validation_to_enable) / sizeof(vk_validation_to_enable[0]),
                                                              .pEnabledValidationFeatures = vk_validation_to_enable,
                                                              .disabledValidationFeatureCount = 0,
                                                              .pDisabledValidationFeatures = nullptr,
                                                            });
      cr::out().debug("engine: created vulkan instance");

      if ((mode & runtime_mode::release) == runtime_mode::none)
      {
        vk_instance.install_default_debug_callback(VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT);
      }

      // create the actual context:
      if ((mode & runtime_mode::hydra_context) == runtime_mode::hydra_context)
      {
        context.emplace<hydra_context>(std::move(vk_instance), hydra_init,
                                       temp_graphic_queue, temp_transfer_queue, temp_compute_queue,
                                       engine_settings.vulkan_device_preferences);
      }
      else // vk_context:
      {
        context.emplace<internal_vk_context>(std::move(vk_instance), hydra_init,
                                             temp_graphic_queue, temp_transfer_queue, temp_compute_queue,
                                             engine_settings.vulkan_device_preferences);
      }
    }
    else // core context
    {
      context.emplace<core_context>();
    }

    cr::out().debug("engine: engine context created");

    // the core context is guranteed to exist:
    core_context& cctx = get_core_context();

    // set the contextes in modules:
    {
      for (auto& mod : modules)
        mod.second->set_core_context(&cctx);
    }
    if ((mode & runtime_mode::vulkan_context) == runtime_mode::vulkan_context)
    {
      vk_context& vctx = get_vulkan_context();
      for (auto& mod : modules)
        mod.second->set_vk_context(&vctx);
    }
    if ((mode & runtime_mode::hydra_context) == runtime_mode::hydra_context)
    {
      hydra_context& hctx = get_hydra_context();
      for (auto& mod : modules)
        mod.second->set_hydra_context(&hctx);
    }

    cr::out().debug("engine: engine successfully initialized");
    return resources::status::success;
  }

  resources::context::status_chain engine_t::boot(id_t index_key, const std::string& index_file)
  {
    if (mode == runtime_mode::none)
    {
      check::debug::n_check(false, "engine: trying to boot a non-initialized engine. Please call init() before calling boot.");
      return resources::context::status_chain::create_and_complete(resources::status::failure);
    }

    // the core context is guranteed to exist:
    core_context& cctx = get_core_context();

    if (cctx.is_booted())
    {
      check::debug::n_check(false, "engine: trying to boot an already booted engine.");
      return resources::context::status_chain::create_and_complete(resources::status::failure);
    }

    cr::out().debug("engine: booting engine (index: {})", index_file);

    // run the pre-boot step:
    for (auto& mod : modules)
      mod.second->on_pre_boot_step();

    // setup the task groups:
    threading::task_group_dependency_tree tgd;

    // add the module task groups / their dependencies:
    for (auto& mod : modules)
      mod.second->add_task_groups(tgd);
    for (auto& mod : modules)
      mod.second->add_task_groups_dependencies(tgd);

    // boot the core context:
    cr::out().debug("engine: booting engine...");
    std::lock_guard _lg (init_lock);
    auto tree = tgd.compile_tree();
    tree.print_debug();
    auto ret = cctx.boot(std::move(tree), index_key, index_file, false /*unlock tm*/).then(&cctx.tm, threading::k_non_transient_task_group, [this, &cctx](resources::status st)
    {
      cr::out().debug("engine: index loaded");
      if (st == resources::status::failure)
      {
        cr::out().debug("engine: failed to load index, tearing-down the engine");
        sync_teardown();
        return st;
      }

      // NOTE: partial success is ok

      {
        std::lock_guard _lg(init_lock);

        for (auto& mod : modules)
          mod.second->on_resource_index_loaded();

        cr::out().debug("engine: index has been loaded");
      }
      return st;
    });

    // done after the call to boot() so as to maximize what it done at the same time:
    for (auto& mod : modules)
      mod.second->on_context_initialized();
    cr::out().debug("engine: modules initialized, waiting for index load");

    return ret.then([this, &cctx](resources::status st)
    {
      cctx.tm.get_frame_lock()._unlock();
      cr::out().debug("engine: task manager unlocked");

      for (auto& mod : modules)
        mod.second->on_engine_boot_complete();
      cr::out().log("engine: engine has fully booted");
      return st;
    });
  }

  void engine_t::sync_teardown()
  {
    init_lock.lock();
    if (mode == runtime_mode::none)
    {
      init_lock.unlock();
      return;
    }

    core_context& cctx = get_core_context();
    if (cctx.is_stopped())
    {
      init_lock.unlock();
      return;
    }
    cr::out().debug("engine tear-down: stopping the task manager...");
    cctx.stop_app()
    .then([this, &cctx]
    {
      cr::out().debug("engine tear-down: clearing remaining tasks...");
      while (cctx.tm.has_pending_tasks())
        cctx.tm.run_a_task();

      if ((mode & runtime_mode::vulkan_context) == runtime_mode::vulkan_context)
      {
        vk_context& vctx = get_vulkan_context();

        cr::out().debug("engine tear-down: syncing vulkan device...");
        vctx.device.wait_idle();

        if ((mode & runtime_mode::hydra_context) == runtime_mode::hydra_context)
        {
          hydra_context& hctx = get_hydra_context();

          cr::out().debug("engine tear-down: destructing vulkan objects which are pending deletion...");
          hctx.vrd._force_full_cleanup();
        }
      }

      cr::out().debug("engine tear-down: module shutdown...");
      for (auto& mod : modules)
        mod.second->on_start_shutdown();

      if ((mode & runtime_mode::vulkan_context) == runtime_mode::vulkan_context)
      {
        vk_context& vctx = get_vulkan_context();
        vctx.device.wait_idle();
        if ((mode & runtime_mode::hydra_context) == runtime_mode::hydra_context)
        {
          hydra_context& hctx = get_hydra_context();
          hctx.vrd._force_full_cleanup();
        }
      }

      cr::out().debug("engine tear-down: destructing modules...");
      modules.clear();

      // NOTE: we cannot destroy the context as we are still in the task manager

      cr::out().debug("engine tear-down: clearing the runtime-mode...");

      // very last operation
      mode = runtime_mode::none;

      // release the lock: (it's not the same thread that locked the lock, so we use _unlock instead)
      init_lock._unlock();
    });

  }
}

