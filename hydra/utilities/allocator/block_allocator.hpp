//
// created by : Timothée Feuillet
// date: 2022-8-26
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

#include <ntools/mt_check/deque.hpp>

#include "../memory_allocation.hpp"
#include "allocator.hpp"

namespace neam::hydra::allocator
{
  /// \brief Handle block-level stuff and raw allocations
  /// Block are k_block_size memory areas. Block are either used by allocator-pools or directly (block-level allocations)
  /// Raw allocations performed by the block-allocator are always a multiple of block-size (up to k_max_raw_allocation_size)
  /// Allocations bigger than k_max_raw_allocation_size cannot be handled by the block-allocator.
  ///
  /// Deallocation is extremely fast, allocations can be slow if fragmentation is high
  class block_allocator final : allocator_interface
  {
    public:
      static constexpr uint32_t k_block_size = 8 * 1024 * 1024;
      static constexpr uint32_t k_max_block_count = 64; // max number of block in an allocation
      static constexpr size_t k_max_raw_allocation_size = k_block_size * k_max_block_count;

      static constexpr uint32_t k_start_block_count_to_allocate = 4;

      // will progressively grow from k_start_block_count_to_allocate to k_max_block_count in the specified number of allocations
      static constexpr uint32_t k_allocations_for_max_size = 100;

      block_allocator(vk::device& dev, uint32_t _memory_type_index) : device(dev), memory_type_index(_memory_type_index) {}

      /// \brief Perform a block-level allocation.
      /// Unless there's a lot of fragmentation, this operation is fast
      memory_allocation block_level_allocation(uint32_t block_count);

      /// \brief Try to grow an allocation by the specified amount of blocks
      /// \note It's a extremely fast operation
      bool try_grow_allocation(memory_allocation& mem, uint32_t additional_block_count);

      /// \brief Free the allocation (extremely fast)
      void free_allocation(const memory_allocation& mem) override;

      uint32_t _get_memory_type_index() const { return memory_type_index; }

      void _should_map_memory(bool _map){ map_memory = _map; }

      uint64_t get_allocated_memory() const { return allocated_memory; }

    private:
      struct raw_allocation
      {
        vk::device_memory mem; // MUST be the first field !!
        uint64_t free_mask = ~uint64_t(0);
      };

    private:
      raw_allocation& add_new_allocation(uint32_t block_count);

      vk::device& device;

      std::mtc_deque<raw_allocation> allocations;

      // min number of block to allocate at once in a raw allocation. Can be changed, and will increase WRT the number of raw allocations currently alive
      uint32_t blocks_to_allocate = k_start_block_count_to_allocate;
      uint32_t memory_type_index;

      uint64_t allocated_memory = 0;

      bool map_memory = false;
  };
}

