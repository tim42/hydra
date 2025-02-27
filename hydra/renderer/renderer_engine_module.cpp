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

#include "renderer_engine_module.hpp"

#include <hydra/init/feature_requesters/gen_feature_requester.hpp>
#include <hydra/engine/engine.hpp>
#include <hydra/engine/core_modules/core_module.hpp>
#include <hydra/utilities/transfer_context.hpp>
#include <ntools/hash/hash.hpp>
#include <ntools/tracy.hpp>

#include "ecs/gpu_tasks_order.hpp"
#include "ecs/hierarchy.hpp"
#include "ecs/universe.hpp"

namespace neam::hydra
{
  void renderer_module::init_vulkan_interface(gen_feature_requester& gfr, bootstrap& /*hydra_init*/)
  {
    gfr.gpu_features.get_device_features().shaderStorageImageReadWithoutFormat = true;
    gfr.gpu_features.get_device_features().shaderStorageImageWriteWithoutFormat = true;
    gfr.gpu_features.get_device_features().shaderFloat64 = true;
    gfr.gpu_features.get_device_features().shaderInt64 = true;
    gfr.gpu_features.get_device_features().shaderInt16 = true;
    gfr.gpu_features.get_device_features().multiDrawIndirect = true;
    gfr.gpu_features.get_device_features().sparseBinding = true;

    gfr.gpu_features.get_device_features().sparseResidencyImage2D = true;
    gfr.gpu_features.get_device_features().sparseResidencyImage3D = true;
    gfr.gpu_features.get_device_features().sparseResidencyBuffer = true;
    // Not needed for now
    //gfr.gpu_features.get_device_features().sparseResidencyAliased = true;

    // request validation stuff if not in release:
    if ((engine->get_runtime_mode() & runtime_mode::release) == runtime_mode::none)
    {
      gfr.require_instance_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
      gfr.require_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      gfr.require_instance_layer("VK_LAYER_KHRONOS_validation");
      // gfr.require_instance_layer("VK_LAYER_LUNARG_api_dump");
    }

    gfr.require_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
    gfr.require_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
    gfr.require_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
    gfr.require_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    gfr.require_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    // gfr.require_device_extension(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME); // renderdoc dislikes this extension
    gfr.require_device_extension(VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME);
    gfr.require_device_extension(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);

    // we require dynamic rendering
    gfr.gpu_features.get<VkPhysicalDeviceVulkan13Features>().dynamicRendering = true;
    VkPhysicalDeviceFeatures& vkdevfeatures = gfr.gpu_features.get_device_features();
    vkdevfeatures.imageCubeArray = true;

    VkPhysicalDeviceVulkan11Features& vk11features = gfr.gpu_features.get<VkPhysicalDeviceVulkan11Features>();
    vk11features.storageBuffer16BitAccess = true;
    vk11features.uniformAndStorageBuffer16BitAccess = true;
    vk11features.storagePushConstant16 = true;
    vk11features.storageInputOutput16 = true;

    VkPhysicalDeviceVulkan12Features& vk12features = gfr.gpu_features.get<VkPhysicalDeviceVulkan12Features>();
    vk12features.storageBuffer8BitAccess = true;
    vk12features.uniformAndStorageBuffer8BitAccess = true;
    vk12features.storagePushConstant8 = true;
    vk12features.shaderBufferInt64Atomics = true;
    vk12features.shaderSharedInt64Atomics = true;
    vk12features.shaderFloat16 = true;
    vk12features.shaderInt8 = true;
    vk12features.drawIndirectCount = true;
    vk12features.bufferDeviceAddress = true;
    vk12features.descriptorIndexing = true;

    // vk12features.shaderInputAttachmentArrayDynamicIndexing;
    vk12features.shaderUniformTexelBufferArrayDynamicIndexing = true;
    vk12features.shaderStorageTexelBufferArrayDynamicIndexing = true;
    vk12features.shaderUniformBufferArrayNonUniformIndexing = true;
    vk12features.shaderSampledImageArrayNonUniformIndexing = true;
    vk12features.shaderStorageBufferArrayNonUniformIndexing = true;
    vk12features.shaderStorageImageArrayNonUniformIndexing = true;
    vk12features.shaderInputAttachmentArrayNonUniformIndexing = true;
    vk12features.shaderUniformTexelBufferArrayNonUniformIndexing = true;
    vk12features.shaderStorageTexelBufferArrayNonUniformIndexing = true;
    // vk12features.descriptorBindingUniformBufferUpdateAfterBind;
    // vk12features.descriptorBindingSampledImageUpdateAfterBind;
    // vk12features.descriptorBindingStorageImageUpdateAfterBind;
    // vk12features.descriptorBindingStorageBufferUpdateAfterBind;
    // vk12features.descriptorBindingUniformTexelBufferUpdateAfterBind;
    // vk12features.descriptorBindingStorageTexelBufferUpdateAfterBind;
    // vk12features.descriptorBindingUpdateUnusedWhilePending;
    vk12features.descriptorBindingPartiallyBound = true;
    vk12features.descriptorBindingVariableDescriptorCount = true;
    vk12features.runtimeDescriptorArray = true;

    // we require mesh shaders
    VkPhysicalDeviceMeshShaderFeaturesEXT& vkmeshfeatures = gfr.gpu_features.get<VkPhysicalDeviceMeshShaderFeaturesEXT>();
    vkmeshfeatures.meshShader = true;
    vkmeshfeatures.taskShader = true;
    // vkmeshfeatures.primitiveFragmentShadingRateMeshShader = true;

    // descriptor buffers
    // VkPhysicalDeviceDescriptorBufferFeaturesEXT& physdescrbuf_features = gfr.gpu_features.get<VkPhysicalDeviceDescriptorBufferFeaturesEXT>();
    // physdescrbuf_features.descriptorBuffer = true;

    // mutable descriptor sets
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT& mutds_features = gfr.gpu_features.get<VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT>();
    mutds_features.mutableDescriptorType = true;
  }

  void renderer_module::add_named_threads(threading::threads_configuration& tc)
  {
    // tc.add_named_thread("gqueue"_rid, { .can_run_general_tasks = false, .can_run_general_long_duration_tasks = false });
    // tc.add_named_thread("tqueue"_rid, { .can_run_general_tasks = false, .can_run_general_long_duration_tasks = false });
    // tc.add_named_thread("cqueue"_rid, { .can_run_general_tasks = false, .can_run_general_long_duration_tasks = false });
  }
  void renderer_module::add_task_groups(threading::task_group_dependency_tree& tgd)
  {
    tgd.add_task_group("render"_rid);
    tgd.add_task_group("render_cleanup"_rid); // FIXME: Should not be present
    tgd.add_task_group("prepare_queue_submission"_rid);
    tgd.add_task_group("queue_submission"_rid);
  }
  void renderer_module::add_task_groups_dependencies(threading::task_group_dependency_tree& tgd)
  {
    tgd.add_dependency("render"_rid, "queue_submission"_rid); // FIXME
    tgd.add_dependency("render"_rid, "prepare_queue_submission"_rid);
    tgd.add_dependency("render"_rid, "ecs/db-optimize"_rid);
    tgd.add_dependency("queue_submission"_rid, "prepare_queue_submission"_rid);
  }

  ecs::entity renderer_module::create_render_entity()
  {
    ecs::entity ret = universe->get_universe_root().create_weakly_tracked_child();
    return ret;
  }

  void renderer_module::prepare_renderer()
  {
    vk::submit_info si { *hctx };

    hctx->textures.process_start_of_frame(si);

    si.deferred_submit();

    if (hctx->ppmgr.should_refresh())
      hctx->ppmgr.refresh();
  }

  void renderer_module::on_context_set()
  {
    // init the names (only needed for debug purpose)
    {
      auto g = string_id::_runtime_build_from_string("gqueue");
      auto c = string_id::_runtime_build_from_string("cqueue");
      auto t = string_id::_runtime_build_from_string("tqueue");
      auto st = string_id::_runtime_build_from_string("slow_tqueue");
    }
    // create the universe / task-order:
    universe = std::make_unique<ecs::universe>(hctx->db);
    {
      auto& root = universe->get_universe_root_entity();
      std::lock_guard _el(spinlock_exclusive_adapter::adapt(root.get_lock()));
      task_order = &root.add<renderer::internals::gpu_task_order>();
    }
    // FIXME:
    // hctx->gqueue.queue_id = "gqueue"_rid;
    // hctx->tqueue.queue_id = "gqueue"_rid;
    // hctx->slow_tqueue.queue_id = "gqueue"_rid;
    // hctx->cqueue.queue_id = "gqueue"_rid;

    // init the pass order:
#if 0
    {
      render_pass_dependencies.clear_setup_data();

      for (uint32_t i = 0; i < internal::get_pass_count(); ++i)
      {
        const auto& pass_setup = internal::get_pass_setup(i);
        if (pass_setup.size == 0) continue;

        pass_setup.setup_pass_dependencies(render_pass_dependencies);
      }

      render_pass_dependencies.build_pass_order();
      render_pass_dependencies.print_order();
      render_pass_dependencies.clear_setup_data();
    }
#endif
  }

  void renderer_module::on_context_initialized()
  {
    // auto* core = engine->get_module<core_module>("core"_rid);

    // dispatch the dfe poll at the very start of the frame
    // on_frame_start_tk = core->on_frame_start.add([this]
    // {
    //   // hctx->tm.get_long_duration_task([this]
    //   // {
    //   //   hctx->dfe.poll();
    //   // });
    // });

    hctx->tm.set_start_task_group_callback("render_cleanup"_rid, [this]
    {
      hctx->tm.get_task([this]
      {
        hctx->dfe.poll();
      });
    });
    hctx->tm.set_start_task_group_callback("prepare_queue_submission"_rid, [this]
    {
      hctx->tm.get_task([this]
      {
        hctx->dqe.execute_deferred_tasks(hctx->tm.get_group_id("queue_submission"_rid));
      });
    });

    hctx->tm.set_start_task_group_callback("render"_rid, [this]
    {
      skip_frame = true;
      if (chrono.get_accumulated_time() >= min_frame_time)
      {
        chrono.reset();
        skip_frame = false;
      }

      if (skip_frame)
        return;

      {
        std::lock_guard _l { hctx->dqe.lock };
        hctx->dqe.defer_sync_unlocked();
      }

      // start a task to avoid stalling the task manager:
      // (and so that tasks that are spawned by the functions are immediatly dispatched)
      cctx->tm.get_task([this]
      {
        TRACY_SCOPED_ZONE;
        prepare_renderer();

        // Call the different functions:
        {
          TRACY_SCOPED_ZONE;
          on_render_start();
        }

        // once the on-render-start even is done, trigger the actual rendering
        task_order->prepare_and_dispatch_gpu_tasks(*hctx);
      });
    });
    hctx->tm.set_end_task_group_callback("render"_rid, [this]
    {
      if (skip_frame)
        return;

      TRACY_SCOPED_ZONE;
      // may stall the task manager :/
      on_render_end();

      hctx->dfe.defer(hctx->dfe.queue_mask(hctx->gqueue, hctx->cqueue), [this]
      {
        hctx->allocator.flush_empty_allocations();
      });

      // force a sync
      {
        std::lock_guard _l { hctx->dqe.lock };
        hctx->dqe.defer_sync_unlocked();
      }

      // add the main queue fences:
      vk::fence gqf { hctx->device };
      vk::fence cqf { hctx->device };
      vk::fence tqf { hctx->device };
      vk::fence slow_tqf { hctx->device };

      vk::submit_info si { *vctx };
      si.on(hctx->gqueue).signal(gqf);
      si.on(hctx->cqueue).signal(cqf);
      si.on(hctx->tqueue).signal(tqf);
      si.on(hctx->slow_tqueue).signal(slow_tqf);

      si.deferred_submit();

      hctx->dfe.set_end_frame_fences(
      cr::construct<std::vector>(
        std::pair<uint32_t, vk::fence>{hctx->dfe.queue_mask(hctx->gqueue), std::move(gqf)},
        std::pair<uint32_t, vk::fence>{hctx->dfe.queue_mask(hctx->cqueue), std::move(cqf)},
        std::pair<uint32_t, vk::fence>{hctx->dfe.queue_mask(hctx->tqueue), std::move(tqf)},
        std::pair<uint32_t, vk::fence>{hctx->dfe.queue_mask(hctx->slow_tqueue), std::move(slow_tqf)}
      ));

      hctx->gcpm.flip();
      hctx->tcpm.flip();
      hctx->slow_tcpm.flip();
      hctx->ccpm.flip();
    });
  }

  void renderer_module::on_start_shutdown()
  {
    hctx->textures.begin_engine_shutdown();
  }

  void renderer_module::on_shutdown_post_idle_gpu()
  {
    on_frame_start_tk.release();

    task_order = nullptr;
    universe.reset();
  }
}

