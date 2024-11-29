//
// created by : Timothée Feuillet
// date: 2022-8-27
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

#include "allocator_set.hpp"

namespace neam::hydra::allocator
{
  memory_allocation pool_set::allocate(size_t size, uint32_t alignment, allocation_type at)
  {
    check::debug::n_assert(size > 0, "allocate: cannot perform an allocation of size 0");
    check::debug::n_assert(alignment < block_allocator::k_block_size,
                           "allocate: cannot perform an allocation with an alignment of {} (max alignment is {})",
                           alignment, block_allocator::k_block_size);
    check::debug::n_assert(block_allocator::k_block_size % alignment == 0,
                           "allocate: cannot perform an allocation with an alignment of {} ({} is not a multiple of it)",
                           alignment, block_allocator::k_block_size);

    switch (at)
    {
      case allocation_type::raw:
      {
        return memory_allocation(block._get_memory_type_index(), nullptr, vk::device_memory::allocate(device, size, block._get_memory_type_index()));
      }
      case allocation_type::persistent:
        [[fallthrough]]; // FIXME!
      case allocation_type::block_level:
      {
        return block.block_level_allocation((size + block_allocator::k_block_size - 1) / block_allocator::k_block_size);
      }

      case allocation_type::short_lived:
        return transient.allocate(size, alignment);
      case allocation_type::scoped:
        return scoped.allocate(size, alignment);
        // check::debug::n_assert(false, "allocate: scoped allocations should not go through here, but use through the scopes instead");
        break;
      default:
        check::debug::n_assert(false, "allocate: invalid allocation type");
        break;
    }
    __builtin_unreachable();
  }
}

