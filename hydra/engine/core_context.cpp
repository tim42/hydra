//
// created by : Timothée Feuillet
// date: 2023-6-20
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

#include "core_context.hpp"

namespace neam::hydra
{
  resources::context::status_chain core_context::boot(threading::resolved_graph&& task_graph, index_boot_parameters_t&& ibp,
                                                      bool auto_unlock_tm, uint32_t thread_count)
  {
    booted = false;
    never_started = false;
    should_stop = false;
    can_return = false;

    tm.get_frame_lock().lock();
    tm.add_compiled_frame_operations(std::move(task_graph));

    resources::context::status_chain chn;
    switch (ibp.mode)
    {
      case index_boot_parameters_t::init_empty_index:
        res._init_with_clean_index(ibp.index_key);
        chn = chn.create_and_complete(resources::status::success);
        break;
      case index_boot_parameters_t::init_from_data:
        chn = res._init_with_index_data(ibp.index_key, ibp.index_data, ibp.index_size);
        break;
      case index_boot_parameters_t::load_index_file:
      default:
        chn = res.boot(ibp.index_key, ibp.index_file);
        break;
    }

    halted = false;
    // do the resource boot process asynchronously
    tm.get_long_duration_task([this]
    {
      while (!halted && !booted)
      {
        // perform IO stuff:
        io._wait_for_submit_queries();
        tm.run_a_task();
      }
      cr::out().debug("core-context: boot: exiting initial IO loop");
    });

    // start the threads: (we have a minimum requirement of 4 threads)
    thread_count = std::min(std::thread::hardware_concurrency() * 4, std::max(4u, thread_count));
    threads.reserve(thread_count);
    cr::out().debug("core-context: boot: lanching {} threads...", thread_count);
    for (uint32_t i = 0; i < thread_count; ++i)
    {
      threads.emplace_back([this, i] { thread_main(*this, i); });
    }

    cr::out().debug("core-context: boot: sync process done, waiting for async tasks...");

    return chn.then([this, auto_unlock_tm](resources::status st)
    {
      if (auto_unlock_tm)
      {
        // there should not be any outstanding io tasks, so start the task-graph
        tm.get_frame_lock()._unlock();
      }
      booted = true;
      return st;
    });
  }
  core_context::~core_context()
  {
    if (never_started)
      return;
    if (!halted)
      stop_app();
    cr::out().debug("core context: destructor: joining all threads...");
    tm.should_threads_exit_wait(true);
    for (auto& it : threads)
    {
      if (it.joinable())
        it.join();
    }
    tm.should_threads_exit_wait(false);
    cr::out().debug("core context: destructor: waiting tasks...");
    std::lock_guard _l(destruction_lock);
    while (!can_return)
    {
      tm.run_a_task();
    }
    tm.get_frame_lock()._unlock();
    cr::out().debug("core context: destructor: done");
  }

  void core_context::thread_main(core_context& ctx, uint32_t index)
  {
    if (index != ~0u)
      sys::set_cpu_affinity((index + 2) % std::thread::hardware_concurrency());
    else
      sys::set_cpu_affinity(0);

    while (!ctx.should_stop)
    {
      ctx.tm.wait_for_a_task();
      ctx.tm.run_a_task(index < 2 && ctx.threads_to_not_stall > 2);

      if (index > ctx.threads_to_not_stall && !ctx.should_stop)
      {
        TRACY_SCOPED_ZONE;
        while (index > ctx.threads_to_not_stall && !ctx.should_stop)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds{ctx.ms_to_stall});
        }
      }
    }
  }

  async::continuation_chain core_context::stop_app()
  {
    unstall_all_threads();
    if (is_stopped()) return async::continuation_chain::create_and_complete();
    cr::out().debug("core context: stop_app: stopping core context...");
    destruction_lock.lock();
    async::continuation_chain ret;
    can_return = false;
    tm.get_long_duration_task([this, state = ret.create_state()] mutable
    {
      cr::out().debug("core context: stop_app: stopping task-manager...");
      bool done = false;
      do
      {
        // Avoid over-writing an already existing stop request, but we do try every millisecond until we manage to request a stop
        done = tm.try_request_stop([this, state = std::move(state)] mutable
        {
          cr::out().debug("core context: stop_app: task-manager is stopped...");
          should_stop = true;
          can_return = true;
          cr::out().debug("core context: stop_app: flushing io...");
          io._wait_for_submit_queries();
          state.complete();
          destruction_lock._unlock();
        }, true);

        if (!done)
          std::this_thread::sleep_for(std::chrono::milliseconds{1});
      }
      while(!done);
    });
    halted = true;
    return ret;
  }
}

