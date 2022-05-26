//
// created by : Timothée Feuillet
// date: 2022-5-6
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

#include <hydra/engine/core_context.hpp>
#include <ntools/io/io.hpp>

#include <thread>
#include <sys/wait.h>

namespace neam
{
  // read + close the pipe
  inline async::chain<std::string&&> read_pipe(hydra::core_context& ctx, id_t pipe_id, std::string&& content = {})
  {
    constexpr uint32_t k_read_size = 1024;
    return ctx.io.queue_read(pipe_id, 0, k_read_size)
    .then([&ctx, pipe_id, content = std::move(content)] (raw_data&& rd, bool) mutable
    {
      if (rd.size == 0)
      {
        ctx.io.close(pipe_id);
        return async::chain<std::string&&>::create_and_complete(std::move(content));
      }

      const std::string_view view((const char*)rd.data.get(), rd.size);
      content.append(view);

      // there might still be some data in the pipe, continue to read
      return read_pipe(ctx, pipe_id, std::move(content));
    });
  }

  using spawn_function_t = fu2::unique_function<pid_t()>;

  /// \brief Queue a process, avoiding spinning / deadlocking on io
  /// \warning spawn_fnc must be non-blocking
  inline async::chain<int> queue_process(hydra::core_context& ctx, spawn_function_t&& spawn_fnc)
  {
    struct process_t
    {
      async::chain<int>::state state;
      spawn_function_t spawn_fnc;

      pid_t pid = 0;
    };

    static spinlock lock;
    static uint32_t queued_process_count = 0;
    static bool has_task = false;

    std::lock_guard _l(lock);
    async::chain<int> ret;

    queued_process_count += 1;
    if (!has_task)
    {
      has_task = true;

      // register the task lambda
      ctx.tm.get_long_duration_task([&ctx]
      {
        while (true)
        {
          std::this_thread::yield();

          // process some io:
          ctx.io.process();

          std::lock_guard _l(lock);
          if (queued_process_count == 0)
          {
            has_task = false;
            return;
          }
        }
      });
    }
    ctx.tm.get_long_duration_task([&ctx, spawn_fnc = std::move(spawn_fnc), state = ret.create_state()] mutable
    {
      const pid_t pid = spawn_fnc();
      int ret = -1;
      int status;
      if (waitpid(pid, &status, 0) == pid)
      {
        if (WIFEXITED(status))
          ret = WEXITSTATUS(status);
      }

      {
        std::lock_guard _l(lock);
        --queued_process_count;
      }

      state.complete(ret);
    });
    return ret;
  }
}

