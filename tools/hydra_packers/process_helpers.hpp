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
  async::chain<std::string&&> read_pipe(hydra::core_context& ctx, id_t pipe_id, std::string&& content = {});

  using spawn_function_t = std::move_only_function<pid_t()>;

  /// \brief Queue a process, avoiding spinning / deadlocking on io
  /// \warning spawn_fnc must be non-blocking
  async::chain<int> queue_process(hydra::core_context& ctx, spawn_function_t&& spawn_fnc);
}

