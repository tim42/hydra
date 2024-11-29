//
// created by : Timothée Feuillet
// date: 2023-9-2
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

#pragma once

#include "vulkan/command_pool.hpp"
#include "vulkan/queue.hpp"

#include <ntools/mt_check/deque.hpp>
#include <ntools/mt_check/vector.hpp>
#include <ntools/spinlock.hpp>

namespace neam::hydra
{
  struct hydra_context;

  /// \brief Manage allocations and reset/reuse of command pools for transient command buffers
  /// \note Could have been named command_pool_pool
  class command_pool_manager
  {
    public:
      command_pool_manager(hydra_context& _hctx, vk::queue& _queue) : hctx(_hctx), queue(_queue) {}


      /// \brief Return a pool for the current thread / given queue.
      /// That pool should not be stored outside local variables or used on another thread (or non-immediatly used).
      /// \warning The returned pool should never live past call to flip(), as it will cause memory corruptions
      [[nodiscard]] vk::command_pool& get_pool();

      /// \brief Flip the pools, clearing the in-use pools and putting them in the VRD for reset
      void flip();

      void _set_debug_name(const std::string&& str) { pool_name = std::move(str); }
    private:
      static std::pair<vk::command_pool*, uint64_t>& get_thread_local_pool(vk::queue& q);

    private:
      hydra_context& hctx;
      vk::queue& queue;

      mutable spinlock free_pools_lock;
      std::mtc_vector<vk::command_pool> free_pools_list;

      mutable shared_spinlock lock;
      std::mtc_deque<vk::command_pool> pools_in_use;

      uint64_t flip_id = 1;

      std::string pool_name;
  };
}

