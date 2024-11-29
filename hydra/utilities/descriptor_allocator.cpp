//
// created by : Timothée Feuillet
// date: 2024-3-2
//
//
// Copyright (c) 2024 Timothée Feuillet
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

#include "descriptor_allocator.hpp"
#include "engine/hydra_context.hpp"

namespace neam::hydra
{
  vk::descriptor_set descriptor_allocator::allocate_set(const vk::descriptor_set_layout& ds_layout, uint32_t variable_descriptor_count)
  {
    static std::atomic<uint32_t> global_count = 0;
    thread_local uint32_t index = ~0u;


    // check that the layout we received is valid
    // (it can very well happen that we get a request to allocate a set for a null ds-layout, when we're still
    //  loading shaders from disk)
    if (ds_layout._get_vk_descriptor_set_layout() == nullptr) [[unlikely]]
      return vk::descriptor_set{ hctx.device, nullptr };


    if (index == ~0u) [[unlikely]]
    {
      // init the thread
      std::lock_guard _l(spinlock_exclusive_adapter::adapt(thread_pools_lock));
      index = global_count.fetch_add(1, std::memory_order_relaxed);
      thread_specific_pools.resize(index + 1, nullptr);
      // please don't create too many threads...
    }
    else if (global_count.load(std::memory_order_acquire) > thread_specific_pools.size()) [[unlikely]]
    {
      // init this instance
      std::lock_guard _l(spinlock_exclusive_adapter::adapt(thread_pools_lock));
      if (global_count.load(std::memory_order_relaxed) > thread_specific_pools.size())
        thread_specific_pools.resize(global_count.load(std::memory_order_relaxed) + 1, nullptr);
    }

    // init is done, try to allocate the set using the pool we have in the thread
    uint32_t waiting_pools_count = waiting_pools.size();
    std::lock_guard _l(spinlock_shared_adapter::adapt(thread_pools_lock));
    do
    {
      if (thread_specific_pools[index] == nullptr) [[unlikely]]
      {
        std::lock_guard _l(pools_lock);
        if (waiting_pools.size() == 0)
          break; // force allocation of a new pool
        // try to allocate using any pools in the waiting_pools list
        thread_specific_pools[index] = waiting_pools.front();
        waiting_pools.pop_front();
        if (waiting_pools_count == 0)
          break; // force allocation of a new pool
        --waiting_pools_count;
      }

      // try to allocate the descriptor set:
      auto [ status, ret ] = thread_specific_pools[index]->try_allocate_descriptor_set(ds_layout, true, variable_descriptor_count);
      if (status == VK_SUCCESS) [[likely]]
        return std::move(ret);

      // push the pool back in the waiting pools
      {
        std::lock_guard _l(pools_lock);
        bool has_pools = (waiting_pools.size() != 0);
        waiting_pools.push_back(thread_specific_pools[index]);
        if (!has_pools)
          break; // force allocation of a new pool
        thread_specific_pools[index] = waiting_pools.front();
        waiting_pools.pop_front();
        if (waiting_pools_count == 0)
          break; // force allocation of a new pool
        --waiting_pools_count;
      }
    } while (true);

    // allocate a new pool and a descriptor_set from that new pool. Note that we MUST succeed in allocating that set.
    thread_specific_pools[index] = create_pool();
    return thread_specific_pools[index]->allocate_descriptor_set(ds_layout, true, variable_descriptor_count);
  }

  vk::descriptor_pool* descriptor_allocator::create_pool()
  {
    std::lock_guard _l(pools_storage_lock);

    VkDescriptorPoolSize pool_sizes[] =
    {
      { VK_DESCRIPTOR_TYPE_MUTABLE_EXT, 16384, },
      { VK_DESCRIPTOR_TYPE_SAMPLER, 16384, },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16384, },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 16384, },
    };
    VkDescriptorPoolCreateInfo create_info
    {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr,
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      16384, // max set
      sizeof(pool_sizes) / sizeof(pool_sizes[0]), pool_sizes,
    };
    cr::out().debug("descriptor_allocator::create_pool: total of {} allocated descriptor pool", pools_storage.size() + 1);
    vk::descriptor_pool& pool = pools_storage.emplace_back(hctx.device, create_info);
    pool._set_debug_name("descriptor_allocator::pool");
    return &pool;
  }
}

