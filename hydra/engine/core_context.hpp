//
// created by : Timothée Feuillet
// date: 2022-4-26
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

#pragma once

#include <ntools/io/context.hpp>
#include <ntools/threading/threading.hpp>
#include <ntools/sys_utils.hpp>

#include <resources/context.hpp>
#include <engine/conf/context.hpp>

namespace neam::hydra
{
  /// \brief Hold the core context. (all which is related to resources, threading, io, memory, ...)
  class core_context
  {
    public:
      threading::task_manager tm;
      io::context io;
      resources::context res = { io, *this };
      conf::context hconf = { *this };


    public:
      resources::context::status_chain boot(threading::resolved_graph&& task_graph, id_t index_key, const std::string& index_file = "root.index", bool auto_unlock_tm = true, uint32_t thread_count = std::thread::hardware_concurrency() - 2)
      {
        booted = false;
        never_started = false;
        should_stop = false;
        can_return = false;

        tm.get_frame_lock().lock();
        tm.add_compiled_frame_operations(std::move(task_graph));
        auto chn = res.boot(index_key, index_file);

        halted = false;
        // do the resource boot process asynchronously
        tm.get_long_duration_task([this]
        {
          while (!halted && !booted)
          {
            // perform IO stuff:
            io._wait_for_submit_queries();
          }
          cr::out().debug("core-context: boot: exiting initial IO loop");
        });

        // start the threads: (we have a minimum requirement of 4 threads)
        thread_count = std::min(std::thread::hardware_concurrency(), std::max(4u, thread_count));
        threads.reserve(thread_count);
        for (uint32_t i = 0; i < thread_count; ++i)
        {
          threads.emplace_back([this, i] { thread_main(*this, i); });
        }


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

      ~core_context()
      {
        if (never_started)
          return;
        if (!halted)
          stop_app();
        for (auto& it : threads)
        {
          if (it.joinable())
            it.join();
        }
        std::lock_guard _l(destruction_lock);
        cr::out().debug("core context is being destructed");
        while (!can_return)
        {
          tm.run_a_task();
        }
        tm.get_frame_lock()._unlock();
      }

      static void thread_main(core_context& ctx, uint32_t index = 2048)
      {
        if (index != ~0u)
          sys::set_cpu_affinity((index + 2) % std::thread::hardware_concurrency());
        else
          sys::set_cpu_affinity(0);

        while (!ctx.should_stop)
        {
          ctx.tm.wait_for_a_task();
          ctx.tm.run_a_task(index < 2 && ctx.threads_to_not_stall > 2);

          while (index > ctx.threads_to_not_stall && !ctx.should_stop)
          {
            std::this_thread::sleep_for(std::chrono::milliseconds{ctx.ms_to_stall});
          }
        }
      }

      async::continuation_chain stop_app()
      {
        if (is_stopped()) return async::continuation_chain::create_and_complete();
        destruction_lock.lock();
        async::continuation_chain ret;
        can_return = false;
        tm.request_stop([this, state = ret.create_state()] mutable
        {
          should_stop = true;
          can_return = true;
          state.complete();
          destruction_lock._unlock();
        }, true);
        halted = true;
        return ret;
      }

      /// \brief Return the number of threads manager by the core context
      uint32_t get_thread_count() const { return (uint32_t)threads.size(); }

      bool is_stopped() const { return halted && !never_started; }
      bool is_booted() const { return !never_started; }

      /// \brief Stall every threads except the \e count first threads. (2 is probably the safest min number of threads to not stall)
      void stall_all_threads_except(uint32_t count) { threads_to_not_stall = count; }

      /// \brief Undo the effects of stall_all_threads_except. Might take some time to take effect
      void unstall_all_threads() { threads_to_not_stall = ~0u; }

    private:
      std::vector<std::thread> threads;
      spinlock destruction_lock;
      bool should_stop = false;
      bool can_return = false;
      bool halted = false;
      bool never_started = true;
      bool booted = false;


      uint32_t threads_to_not_stall = ~0u;
      uint32_t ms_to_stall = 500;
    };
}

