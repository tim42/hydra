//
// created by : Timothée Feuillet
// date: 2023-7-2
//
//
// Copyright (c) 2023 Timothée Feuillet
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

#include "glfw_engine_module.hpp"
#include "ecs/glfw_prologue.hpp"
#include "ecs/glfw_epilogue.hpp"

namespace neam::hydra::glfw
{
  std::atomic<uint32_t> glfw_module::s_module_count = 0;

  glfw_module::glfw_module()
  {
    bool should_init = s_module_count.fetch_add(1, std::memory_order_acq_rel) == 0;
    if (should_init)
      glfwInit();
  }

  glfw_module::~glfw_module()
  {
    bool should_deinit = s_module_count.fetch_sub(1, std::memory_order_acq_rel) == 1;
    if (should_deinit)
      glfwTerminate();
  }

  bool glfw_module::is_compatible_with(runtime_mode m)
  {
    // we need vulkan and a screen for glfw to be active
    if ((m & runtime_mode::vulkan_context) != runtime_mode::vulkan_context)
      return false;
    if ((m & runtime_mode::offscreen) != runtime_mode::none)
      return false;
    return true;
  }

  void glfw_module::init_vulkan_interface(gen_feature_requester& gfr, bootstrap& /*hydra_init*/)
  {
    gfr.require_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    uint32_t required_extension_count;
    const char** required_extensions;
    required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);

    // this is fatal (at least fatal enough to throw an exception) here 'cause if we're in this function
    // the user explicitly asked for GLFW support
    check::on_vulkan_error::n_assert(required_extensions != nullptr, "GLFW failed to find the platform surface extensions");

    // insert the needed instance extensions:
    for (size_t i = 0; i < required_extension_count; ++i)
      gfr.require_instance_extension(required_extensions[i]);
  }

  bool glfw_module::filter_queue(vk::instance& instance, int/*VkQueueFlagBits*/ queue_type, size_t qindex, const neam::hydra::vk::physical_device& gpu)
  {
//         const runtime_mode mode = engine->get_runtime_mode();
    //const bool present_from_compute = (mode & runtime_mode::present_from_compute) != runtime_mode::none;
    const bool present_from_compute = false;

    if ((queue_type == VK_QUEUE_COMPUTE_BIT && present_from_compute) || (queue_type == VK_QUEUE_GRAPHICS_BIT && !present_from_compute))
    {
      if (!glfwGetPhysicalDevicePresentationSupport(instance._get_vk_instance(), gpu._get_vk_physical_device(), qindex))
        return false;
    }

    // Don't do anything for unconcerned queues:
    return true;
  }

  void glfw_module::add_task_groups(threading::task_group_dependency_tree& tgd)
  {
    tgd.add_task_group("glfw/events"_rid, { .restrict_to_named_thread = "main"_rid });
    tgd.add_task_group("glfw/present"_rid);
    tgd.add_task_group("glfw/framebuffer_acquire"_rid);
    tgd.add_task_group("glfw/update"_rid);
  }

  void glfw_module::add_task_groups_dependencies(threading::task_group_dependency_tree& tgd)
  {
    tgd.add_dependency("glfw/present"_rid, "render"_rid);
    //tgd.add_dependency("glfw/framebuffer_acquire"_rid, "render"_rid);
    tgd.add_dependency("render"_rid, "glfw/framebuffer_acquire"_rid);
    tgd.add_dependency("glfw/framebuffer_acquire"_rid, "glfw/update"_rid);
  }

  void glfw_module::on_context_initialized()
  {
    // Initialize the cursors (on the main thread)
    execute_on_main_thread([this]()
    {
      cr::out().debug("glfw: creating cursors");
      init_cursors();
    });

    // add an event polling task
    hctx->tm.set_start_task_group_callback("glfw/events"_rid, [this]
    {
      cctx->tm.get_task([this]
      {
        if (hctx->db.get_attached_object_count<components::epilogue>() == 0)
          return;
        if (should_wait_for_events && frames_since_any_event > k_max_frame_to_render_after_event && frames_since_any_event < 100)
          glfwWaitEventsTimeout(10);
        else
          glfwPollEvents();
      });
    });

    hctx->tm.set_start_task_group_callback("glfw/present"_rid, [this]
    {
      hctx->tm.get_task([this]
      {
        TRACY_SCOPED_ZONE;
        hctx->db.for_each([this](components::epilogue& epi)
        {
          hctx->tm.get_task([this, &epi]
          {
            TRACY_SCOPED_ZONE;
            epi.present();
          });
        });
      });
    });
    hctx->tm.set_start_task_group_callback("glfw/framebuffer_acquire"_rid, [this]
    {
      hctx->tm.get_task([this]
      {
        TRACY_SCOPED_ZONE;
        hctx->db.for_each([this](components::epilogue& epi)
        {
          // we cannot spawn a task as it's inherently single-threaded :(
          epi.acquire_next_image();
        });
      });
    });
    hctx->tm.set_start_task_group_callback("glfw/update"_rid, [this]
    {
      hctx->tm.get_task([this]
      {
        TRACY_SCOPED_ZONE;
        bool had_any_events = false;

        bool is_focused = false;
        bool has_any_windows = false;

        hctx->db.for_each([&, this](components::epilogue& epi)
        {
          auto& win = epi.prologue_comp.win;
          if (!win.is_window_ready())
            return;
          has_any_windows = true;
          if (win.is_focused())
            is_focused = true;
          if (win.get_event_manager().get_event_count() > 0)
          {
            win.get_event_manager().clear_event_count();
            had_any_events = true;
          }
        });

        if (had_any_events)
          frames_since_any_event = 0;
        else
          ++frames_since_any_event;

        was_focused = is_focused;
        has_any_window_ready = has_any_windows;
      });
    });
  }

  void glfw_module::on_shutdown_post_idle_gpu()
  {
    // Destroy the cursors (on the main thread)
    execute_on_main_thread([this]()
    {
      cr::out().debug("glfw: destroying cursors");
      destroy_cursors();
    });
  }

  void glfw_module::on_shutdown()
  {
  }

  void glfw_module::init_cursors()
  {
    assert_is_main_thread();

    cursors[(uint32_t)cursor::arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    cursors[(uint32_t)cursor::ibeam] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    cursors[(uint32_t)cursor::crosshair] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    cursors[(uint32_t)cursor::pointing_hand] = glfwCreateStandardCursor(GLFW_POINTING_HAND_CURSOR);
    cursors[(uint32_t)cursor::resize_ew] = glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
    cursors[(uint32_t)cursor::resize_ns] = glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
    cursors[(uint32_t)cursor::resize_nwse] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    cursors[(uint32_t)cursor::resize_nesw] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    cursors[(uint32_t)cursor::resize_all] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    cursors[(uint32_t)cursor::not_allowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
  }

  void glfw_module::destroy_cursors()
  {
    assert_is_main_thread();

    for (uint32_t i = 0; i < (uint32_t)cursor::_count; ++i)
    {
      glfwDestroyCursor(cursors[i]);
      cursors[i] = nullptr;
    }
  }

  void glfw_module::execute_on_main_thread(threading::function_t&& fnc) const
  {
    if (cctx->tm.get_current_thread() == cctx->tm.get_named_thread("main"_rid))
    {
      fnc();
    }
    else
    {
      cctx->tm.get_long_duration_task("main"_rid, std::move(fnc));
    }
  }

  void glfw_module::assert_is_main_thread() const
  {
    check::debug::/*n_assert*/n_check((cctx->tm.get_current_thread() == cctx->tm.get_named_thread("main"_rid)), "Current thread is not the main thread (glfw functions require to be called on the main thread)");
  }

  ecs::entity glfw_module::create_render_entity(window& win)
  {
    auto& renderer = *engine->get_module<renderer_module>();

    ecs::entity ret = renderer.create_render_entity();

    std::lock_guard _el(spinlock_exclusive_adapter::adapt(ret.get_lock()));
    ret.add<components::prologue>(*hctx, win);
    ret.add<components::epilogue>(*hctx, win);
    return ret;
  }
}

