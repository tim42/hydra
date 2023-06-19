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

namespace neam::resources
{
  /// \brief Compress the data, result may be asynchronous / delegated to another thread
  /// \note the result is not a valid XZ stream, but instead can only be decoded with uncompress
  async::chain<raw_data> compress(raw_data&& in, threading::task_manager* tm = nullptr, threading::group_t group = threading::k_invalid_task_group);

  /// \brief uncompress the data, result may be asynchronous / delegated to another thread
  /// \note the input is not a valid XZ stream, but instead something that compress produced
  async::chain<raw_data> uncompress(raw_data&& in, threading::task_manager* tm = nullptr, threading::group_t group = threading::k_invalid_task_group);

  /// \brief uncompress the data, result may be asynchronous / delegated to another thread
  /// \note this version only takes valid XZ data stream
  async::chain<raw_data> uncompress_raw_xz(raw_data&& in, threading::task_manager* tm = nullptr, threading::group_t group = threading::k_invalid_task_group);
}

