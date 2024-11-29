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

#include "transient_pool.hpp"
#include "block_allocator.hpp"

namespace neam::hydra::allocator
{
  static constexpr uint32_t align(uint32_t value, uint32_t alignment)
  {
    return (value - 1u + alignment) & -alignment;
  }

  transient_pool::~transient_pool()
  {
    std::lock_guard _lg(lock);
    if (current_block)
    {
      check::debug::n_assert(current_block->remaining_suballocations == 0, "transient_pool: destructing a pool which has still references to it/still has allocations");

      delete current_block;
    }
  }

  memory_allocation transient_pool::allocate(uint32_t size, uint32_t alignment)
  {
    std::lock_guard _lg(lock);
    check::debug::n_assert(size > 0, "allocate: cannot perform an allocation of size 0");

    if (!current_block)
      do_allocate_block(size);

    const uint32_t aligned_offset = align(offset, alignment);
    auto& allocation = current_block->allocation;
    if (aligned_offset + size <= allocation.size())
    {
      offset = aligned_offset + size;
      ++current_block->remaining_suballocations;
      return {allocator._get_memory_type_index(), allocation_type::short_lived, allocation.offset() + aligned_offset, size, allocation.mem(), this, current_block};
    }

    // not enough space, try to grow the allocation (which is faster than performing a new allocation):
    const uint32_t block_count = (size + block_allocator::k_block_size - 1u) / block_allocator::k_block_size;
    if (allocator.try_grow_allocation(allocation, block_count))
    {
      offset = aligned_offset + size;
      ++current_block->remaining_suballocations;
      return {allocator._get_memory_type_index(), allocation_type::short_lived, allocation.offset() + aligned_offset, size, allocation.mem(), this, current_block};
    }

    // perform a new allocation:
    do_allocate_block(size);
    auto& new_allocation = current_block->allocation;

    offset = size;
    ++current_block->remaining_suballocations;
    return {allocator._get_memory_type_index(), allocation_type::short_lived, new_allocation.offset(), size, new_allocation.mem(), this, current_block};
  }

  void transient_pool::free_allocation(const memory_allocation& mem)
  {
    std::lock_guard _lg(lock);
    check::debug::n_assert(mem.allocator() == this, "free_allocation: wrong allocator for memory allocation");
    check::debug::n_assert(mem._get_payload() != nullptr, "free_allocation: invalid allocation payload");

    block_allocation& block = *(block_allocation*)mem._get_payload();
    check::debug::n_assert(block.remaining_suballocations > 0, "free_allocation: invalid allocator state");
    --block.remaining_suballocations;
    if (block.has_allocation_ended && block.remaining_suballocations == 0)
    {
      delete &block;
      return;
    }
  }

  void transient_pool::do_allocate_block(size_t size)
  {
    const uint32_t block_count = std::max<uint32_t>(2u, (size + block_allocator::k_block_size - 1u) / block_allocator::k_block_size);

    if (current_block != nullptr)
    {
      current_block->has_allocation_ended = true;
      if (current_block->remaining_suballocations == 0)
        delete current_block;
    }

    current_block = new block_allocation
    {
      allocator.block_level_allocation(block_count),
    };
    offset = 0;
  }
}

