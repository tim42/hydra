//
// created by : Timothée Feuillet
// date: 2021-12-18
//
//
// Copyright (c) 2021 Timothée Feuillet
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

#include <ntools/raw_data.hpp>
#include <ntools/async/chain.hpp>
#include <ntools/threading/task_manager.hpp>
#include <ntools/threading/utilities/rate_limit.hpp>

namespace neam::resources
{
  /// \brief Compress a raw_data into a something that uncompress can inflate
  /// \note the result is not a valid XZ stream, but instead can only be decoded with uncompress
  raw_data compress(raw_data&& in);

  /// \brief uncompress data that were produced by compress
  /// \note the input must not be a valid XZ stream, but instead something that compress produced
  raw_data uncompress(raw_data&& in);

  /// \brief uncompress data
  /// \note this version only takes valid XZ data stream
  raw_data uncompress_raw_xz(raw_data&& in);


  // versions using a task manager:

  inline async::chain<raw_data> compress(raw_data&& in, threading::task_manager& tm, threading::group_t group)
  {
    async::chain<raw_data> ret;
    tm.get_task(group, [in = std::move(in), state = ret.create_state()] () mutable
    {
      state.complete(compress(std::move(in)));
    });
    return ret;
  }
  inline async::chain<raw_data> uncompress(raw_data&& in, threading::task_manager& tm, threading::group_t group)
  {
    async::chain<raw_data> ret;
    tm.get_task(group, [in = std::move(in), state = ret.create_state()] () mutable
    {
      state.complete(uncompress(std::move(in)));
    });
    return ret;
  }
  inline async::chain<raw_data> uncompress_raw_xz(raw_data&& in, threading::task_manager& tm, threading::group_t group)
  {
    async::chain<raw_data> ret;
    tm.get_task(group, [in = std::move(in), state = ret.create_state()] () mutable
    {
      state.complete(uncompress_raw_xz(std::move(in)));
    });
    return ret;
  }

  // versions using a rate limiter:

  inline async::chain<raw_data> compress(raw_data&& in, threading::rate_limiter& rl, threading::group_t group, bool high_priority = false)
  {
    async::chain<raw_data> ret;
    rl.dispatch(group, [in = std::move(in), state = ret.create_state()] () mutable
    {
      state.complete(compress(std::move(in)));
    }, high_priority);
    return ret;
  }
  inline async::chain<raw_data> uncompress(raw_data&& in, threading::rate_limiter& rl, threading::group_t group, bool high_priority = false)
  {
    async::chain<raw_data> ret;
    rl.dispatch(group, [in = std::move(in), state = ret.create_state()] () mutable
    {
      state.complete(uncompress(std::move(in)));
    }, high_priority);
    return ret;
  }
  inline async::chain<raw_data> uncompress_raw_xz(raw_data&& in, threading::rate_limiter& rl, threading::group_t group, bool high_priority = false)
  {
    async::chain<raw_data> ret;
    rl.dispatch(group, [in = std::move(in), state = ret.create_state()] () mutable
    {
      state.complete(uncompress_raw_xz(std::move(in)));
    }, high_priority);
    return ret;
  }
}

