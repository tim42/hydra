//
// created by : Timothée Feuillet
// date: 2022-7-23
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

#include "process_helpers.hpp"

namespace neam
{
  struct process_t
  {
    async::chain<int>::state state;
    spawn_function_t spawn_fnc;
  };

  struct process_queue_t
  {
    spinlock lock;
    std::deque<process_t> to_spawn;
    uint32_t running_process_count = 0;

  };

  static process_queue_t& get_process_queue()
  {
    static process_queue_t pq;
    return pq;
  }

  static void spawn_and_wait_for_process(process_t& process)
  {
    const pid_t pid = process.spawn_fnc();
    int ret = -1;
    int status;
    if (waitpid(pid, &status, 0) == pid)
    {
      if (WIFEXITED(status))
        ret = WEXITSTATUS(status);
    }
    process.state.complete(ret);
  }

  static void process_task(hydra::core_context& ctx, process_t process)
  {
    spawn_and_wait_for_process(process);

    process_queue_t& pq = get_process_queue();
    {
      std::lock_guard _l(pq.lock);
      if (pq.to_spawn.size() > 0)
      {
        process = std::move(pq.to_spawn.front());
        pq.to_spawn.pop_front();
      }
      else
      {
        --pq.running_process_count;
        return;
      }
    }

    // launch another task that will spawn the process (let other tasks have a chance to run)
    ctx.tm.get_long_duration_task([&ctx, process = std::move(process)] mutable
    {
      process_task(ctx, std::move(process));
    });
  }

  async::chain<int> queue_process(hydra::core_context& ctx, spawn_function_t&& spawn_fnc)
  {
    process_queue_t& pq = get_process_queue();


    std::lock_guard _l(pq.lock);
    async::chain<int> ret;
    process_t process
    {
      .state = ret.create_state(),
      .spawn_fnc = std::move(spawn_fnc),
    };

    // we leave 2 thread for the other tasks
    if (pq.running_process_count < ctx.get_thread_count() - 2)
    {
      ++pq.running_process_count;
      ctx.tm.get_long_duration_task([&ctx, process = std::move(process)] mutable
      {
        process_task(ctx, std::move(process));
      });
    }
    else
    {
      pq.to_spawn.push_back(std::move(process));
    }

    return ret;
  }
}

