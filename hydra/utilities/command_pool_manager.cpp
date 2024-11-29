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

#include "command_pool_manager.hpp"
#include "engine/hydra_context.hpp"

namespace neam::hydra
{
  vk::command_pool& command_pool_manager::get_pool()
  {
    auto& storage = get_thread_local_pool(queue);
    auto*& stored_ptr = storage.first;

    std::optional<vk::command_pool> pool;
    {
      std::lock_guard _lg(spinlock_shared_adapter::adapt(lock));

      // clear storage if we are on a different flip:
      if (storage.second != flip_id)
      {
        storage.second = flip_id;
        stored_ptr = nullptr;
      }
      if (stored_ptr != nullptr)
      {
        check::debug::n_assert(stored_ptr->_get_vulkan_command_pool() != nullptr, "get_pool: storage contains an invalid command pool");
        return *stored_ptr;
      }

      // not found, try to see there's and entry in the free-list:
      {
        std::lock_guard _lg(free_pools_lock);
        if (!free_pools_list.empty())
        {
          // found an entry, use it:
          check::debug::n_assert(free_pools_list.back()._get_vulkan_command_pool() != nullptr, "get_pool: free_pools_list contains an invalid pool");
          check::debug::n_assert(free_pools_list.back().get_allocated_buffer_count() == 0, "get_pool: free_pools_list contains a pool with buffers still created");
          pool.emplace(std::move(free_pools_list.back()));
          free_pools_list.pop_back();
        }
      }
      if (!pool)
      {
        // no free entry for the current queue, we have to create one:
        pool.emplace(queue._create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT));
        pool->_set_debug_name(pool_name);
      }
    }

    check::debug::n_assert(pool->_get_vulkan_command_pool() != nullptr, "get_pool: will return a null command pool");
    vk::command_pool* ret;
    {
      std::lock_guard _lg(spinlock_exclusive_adapter::adapt(lock));
      // clear storage if we are on a different flip:
      // NOTE: If this happens, it's a very bad sign
      if (storage.second != flip_id)
      {
        check::debug::n_check(false, "get_pool: flip() called during a get_pool call, which should never happen");
        storage.second = flip_id;
      }

      ret = &pools_in_use.emplace_back(std::move(*pool));
    }

    stored_ptr = ret;
    return *ret;
  }

  void command_pool_manager::flip()
  {
    // We only lock for incrementing the flip_id and clearing the pools_in_use
    // (both of those operations should be done atomically
    {
      std::lock_guard _lg(spinlock_exclusive_adapter::adapt(lock));
      ++flip_id;
      if (flip_id == 0)
        flip_id = 1;

      while (!pools_in_use.empty())
      {
        auto& it = pools_in_use.front();
        if (it.get_allocated_buffer_count() == 0)
        {
          it.reset_and_free_memory();
          {
            std::lock_guard _lg(free_pools_lock);
            free_pools_list.emplace_back(std::move(it));
          }
          pools_in_use.pop_front();
          continue;
        }
        break;
      }
    }
  }

  std::pair<vk::command_pool*, uint64_t>& command_pool_manager::get_thread_local_pool(vk::queue& q)
  {
    thread_local std::map<vk::queue*, std::pair<vk::command_pool*, uint64_t>> storage;
    if (auto it = storage.find(&q); it != storage.end())
      return it->second;
    return storage.insert({&q, {nullptr, 0}}).first->second;
    // thread_local std::pair<vk::command_pool*, uint64_t> storage { nullptr, 0 };
    // return storage;
  }
}

