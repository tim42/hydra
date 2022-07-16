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

#include <resources/context.hpp>

namespace neam::hydra
{
  /// \brief Hold the core context. (all which is related to resources, threading, io, memory, ...)
  class core_context
  {
    public:
      threading::task_manager tm;
      io::context io;
      resources::context res = { io, *this };

      resources::context::status_chain boot(threading::resolved_graph&& task_graph, id_t index_key, const std::string& index_file = "root.index", uint32_t thread_count = std::thread::hardware_concurrency() - 2)
      {
        should_stop = false;
        can_return = false;

        tm.get_frame_lock().lock();
        tm.add_compiled_frame_operations(std::move(task_graph));
        auto chn = res.boot(index_key, index_file);

        // do the resource boot process asynchronously
        tm.get_long_duration_task([this]
        {
          // perform IO stuff:
          io._wait_for_submit_queries();

          // there should not be any outstanding io tasks, so start the task-graph
          tm.get_frame_lock()._unlock();
        });

        // start the threads: (we have a minimum requirement of 4 threads)
        thread_count = std::min(std::thread::hardware_concurrency(), std::max(4u, thread_count));
        threads.reserve(thread_count);
        for (uint32_t i = 0; i < thread_count; ++i)
        {
          threads.emplace_back([this] { thread_main(*this); });
        }

        halted = false;

        return chn;
      }

      ~core_context()
      {
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
        tm.get_frame_lock().unlock();
      }

      static void thread_main(core_context& ctx)
      {
        while (!ctx.should_stop)
        {
          ctx.tm.wait_for_a_task();
          ctx.tm.run_a_task();
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
          destruction_lock.unlock();
        });
        halted = true;
        return ret;
      }

      bool is_stopped() const { return halted; }

    private:
      std::vector<std::thread> threads;
      spinlock destruction_lock;
      bool should_stop = false;
      bool can_return = false;
      bool halted = false;
    };
}

