//
// created by : Timothée Feuillet
// date: 2023-1-21
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

#include <hydra/engine/engine.hpp>
#include <hydra/engine/engine_module.hpp>
#include <hydra/engine/core_context.hpp>
#include <hydra/engine/hydra_context.hpp>

#include <ntools/chrono.hpp>
#include "core_module.hpp"

namespace neam::hydra
{
  void core_module::add_task_groups(threading::task_group_dependency_tree& tgd)
  {
    tgd.add_task_group("init"_rid, "init");
    tgd.add_task_group("last"_rid, "last");
  }
  void core_module::add_task_groups_dependencies(threading::task_group_dependency_tree& tgd)
  {
    tgd.add_dependency("io"_rid, "init"_rid);

    // Make the `last` group dependent on all the other groups
    const threading::group_t id = tgd.get_group("last"_rid);
    const threading::group_t count = tgd.get_group_count();
    for (threading::group_t i = 1; i < count; ++i)
    {
      if (id != i)
        tgd.add_dependency(id, i);
    }
  }

  void core_module::on_context_initialized()
  {
    // do some hctx init:
    if (hctx)
    {
      hctx->ppmgr.register_shader_reload_event(*hctx);
    }
  }

  void core_module::on_start_shutdown()
  {
    cctx->res._prepare_engine_shutdown();
    cctx->hconf._stop_watching_for_file_changes();
  }

  void core_module::on_engine_boot_complete()
  {
    last_index_timestamp = cctx->res.get_index_modified_time();
    index_watcher_chrono.reset();
    last_frame_timepoint = std::chrono::high_resolution_clock::now();


    const bool is_release_engine = (engine->get_runtime_mode() & runtime_mode::release) != runtime_mode::none;

    cctx->tm.set_start_task_group_callback("init"_rid, [this, is_release_engine]()
    {
      if (!is_release_engine)
      {
        // spawn the index watcher task
        if (cctx->res.is_index_mapped())
          cctx->tm.get_task([this]() { watch_for_index_change(); });
      }
    });

    cctx->tm.set_start_task_group_callback("last"_rid, [this]
    {
      // if we have anything, we dispatch a task
      if (min_frame_length > std::chrono::microseconds{0})
      {
        cctx->tm.get_task([this]() { throttle_frame(); });
      }
    });
  }

  void core_module::watch_for_index_change()
  {
    TRACY_SCOPED_ZONE;
    // rate-limit the function:
    if (index_watcher_chrono.get_accumulated_time() < 0.5)
      return;
    index_watcher_chrono.reset();

    const std::filesystem::file_time_type index_mtime = cctx->res.get_index_modified_time();
    if (index_mtime > last_index_timestamp)
    {
      neam::cr::out().debug("core_module: index change detected, reloading index");
      last_index_timestamp = index_mtime;
      cctx->res.reload_index();
    }
  }

  void core_module::throttle_frame()
  {
    TRACY_SCOPED_ZONE;
    const auto current_timepoint = std::chrono::high_resolution_clock::now();
    const auto delta = current_timepoint - last_frame_timepoint;

    if (delta + min_delta_time_to_sleep < min_frame_length)
    {
      // If there are any pending tasks, we simply sleep, avoid locking long-durations tasks
      // we still fully lock a thread tho
      if (cctx->tm.has_pending_tasks() || cctx->tm.is_stop_requested())
      {
        std::this_thread::sleep_for(min_frame_length - delta - min_delta_time_to_sleep);
        last_frame_timepoint = std::chrono::high_resolution_clock::now();
      }
      else
      {
        // fully stall the task manager, further limiting cpu usage
        // we should be the very last task to run, so requesting a stop is fine (and we only stop if no one requested it)
        // if we fail to request a stop, we simply sleep
        const bool will_stop = cctx->tm.try_request_stop([this]
        {
          const auto current_timepoint = std::chrono::high_resolution_clock::now();
          const auto delta = current_timepoint - last_frame_timepoint;
          if (delta + min_delta_time_to_sleep < min_frame_length)
            std::this_thread::sleep_for(min_frame_length - delta);
          last_frame_timepoint = std::chrono::high_resolution_clock::now();

          cctx->tm.get_frame_lock().unlock();
        });
        if (!will_stop)
        {
          std::this_thread::sleep_for(min_frame_length - delta - min_delta_time_to_sleep);
          last_frame_timepoint = std::chrono::high_resolution_clock::now();
        }
      }
    }
    else
    {
      last_frame_timepoint = std::chrono::high_resolution_clock::now();
    }
  }
}

