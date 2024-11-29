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

#include <ntools/tracy.hpp>

#include "block_allocator.hpp"

namespace neam::hydra::allocator
{
  memory_allocation block_allocator::block_level_allocation(uint32_t block_count)
  {
    TRACY_SCOPED_ZONE;

    check::debug::n_assert(block_count > 0, "block_level_allocation: cannot allocate 0 blocks (block count must be > 0)");
    const uint64_t mask = (~uint64_t(0)) >> (64 - block_count);

    // go over allocations:
    for (auto& alloc : allocations)
    {
      const uint32_t free_blocks = __builtin_popcountl(alloc.free_mask);
      if (free_blocks >= block_count)
      {
        uint64_t itmask = alloc.free_mask;
        uint32_t shift = 0;
        while (itmask >= mask && itmask != 0)
        {
          const uint32_t ctz = __builtin_ctzl(itmask);
          itmask >>= ctz;
          shift += ctz;
          if ((itmask & mask) == mask)
          {
            // found a space big enough !
            alloc.free_mask &= ~(mask << shift); // update the mask
            return memory_allocation(memory_type_index, allocation_type::block_level, shift * k_block_size, block_count * k_block_size, &alloc.mem, this, nullptr);
          }
          // advance by the number of set bit:
          const uint32_t cts = __builtin_ctzl(~itmask);
          itmask >>= cts;
          shift += cts;
        }
      }
    }


    // allocate a new entry
    {
      raw_allocation& alloc = add_new_allocation(block_count);

      alloc.free_mask &= ~mask;
      return memory_allocation(memory_type_index, allocation_type::block_level, 0, block_count * k_block_size, &alloc.mem, this, nullptr);
    }
  }

  block_allocator::raw_allocation& block_allocator::add_new_allocation(uint32_t block_count)
  {
    TRACY_SCOPED_ZONE;

    const uint32_t current_alloc_count = std::min((uint32_t)allocations.size(), k_allocations_for_max_size);


    // get the number of block to allocate:
    const uint32_t target_block_count = std::max
    (
      blocks_to_allocate,
      k_start_block_count_to_allocate + (current_alloc_count * (k_max_block_count - k_start_block_count_to_allocate) / k_allocations_for_max_size)
    );


    block_count = std::max(block_count, target_block_count);

    check::debug::n_assert(block_count < k_max_block_count,
                           "block_allocator::add_new_allocation: trying to allocate {} blocks in a single allocation (which is more than the maximum of {} blocks in an allocation)",
                           block_count, k_max_block_count);

    const uint64_t mask = (~uint64_t(0)) >> (64 - block_count);

    allocated_memory += block_count * k_block_size;

    check::debug::n_check(allocated_memory < 8ull*1024*1024*1024,
                          "block_allocator::add_new_allocation: allocator for pool {} got above 8Gib (current size: {:.3}Gib)",
                          memory_type_index, (allocated_memory / 1024 / 1024) / 1024.0f);

    allocations.push_back(
    {
      vk::device_memory::allocate(device, block_count * k_block_size, memory_type_index),
      mask,
    });

    if (map_memory)
      allocations.back().mem.map_memory();

    return allocations.back();
  }

  bool block_allocator::try_grow_allocation(memory_allocation& mem, uint32_t additional_block_count)
  {
    if (additional_block_count == 0)
      return true;

    check::debug::n_assert(mem.allocator() == this, "block_allocator::try_grow_allocation: wrong allocator for memory allocation");

    const uint32_t block_count = mem.size() / k_block_size;
    const uint32_t shift = mem.offset() / k_block_size;
    const uint32_t end_shift = shift + block_count;
    const uint64_t additional_mask = (~uint64_t(0)) >> (64 - additional_block_count);
    const uint64_t shifted_additional_mask = additional_mask << end_shift;

    raw_allocation& alloc = *(raw_allocation*)(mem.mem());
    if ((alloc.free_mask & shifted_additional_mask) == shifted_additional_mask)
    {
      alloc.free_mask &= ~shifted_additional_mask;
      mem._set_new_size((block_count + additional_block_count) * k_block_size);
      return true;
    }

    return false;
  }

  void block_allocator::free_allocation(const memory_allocation& mem)
  {
    check::debug::n_assert(mem.allocator() == this, "block_allocator::free_allocation: wrong allocator for memory allocation");

    const uint32_t block_count = mem.size() / k_block_size;
    const uint32_t shift = mem.offset() / k_block_size;
    const uint64_t mask = (~uint64_t(0)) >> (64 - block_count);

    raw_allocation& alloc = *(raw_allocation*)(mem.mem());
    alloc.free_mask |= mask << shift;
  }
}

