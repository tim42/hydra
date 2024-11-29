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

#pragma once

#include <ntools/mt_check/deque.hpp>
#include "vulkan/descriptor_pool.hpp"

namespace neam::hydra
{
  struct hydra_context;

  /// \brief Allocate a descriptor-set using managed pools
  /// \note _might_ not be as efficient as having a pool per layout, but _might_ be more memory efficient
  class descriptor_allocator
  {
    public:
      descriptor_allocator(hydra_context& _hctx) : hctx(_hctx) {}

      /// \brief Allocate a descriptor set from a layout
      /// \note Cannot fails, unless there's no gpu memory left
      [[nodiscard]] vk::descriptor_set allocate_set(const vk::descriptor_set_layout& ds_layout, uint32_t variable_descriptor_count = ~0u);

    private:
      [[nodiscard]] vk::descriptor_pool* create_pool();

    private:
      hydra_context& hctx;

      spinlock pools_lock;
      // set of pools that failed to allocate a set at some point.
      // threads will grab them when a new pool is needed
      std::mtc_deque<vk::descriptor_pool*> waiting_pools;
      shared_spinlock thread_pools_lock;
      std::mtc_deque<vk::descriptor_pool*> thread_specific_pools;

      spinlock pools_storage_lock;
      std::mtc_deque<vk::descriptor_pool> pools_storage;
  };
}

