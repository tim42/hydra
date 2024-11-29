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

#include <cstddef>
#include <cstdint>
#include <vector>
#include "allocator.hpp"
#include "../memory_allocation.hpp"

namespace neam::hydra::allocator
{
  class block_allocator;

  /// \brief Handle \e short-lived allocations.
  /// The right place for anything temporary that has to deal with transfers,
  ///  stuff that has to wait for a fence to be freed,
  ///  or short-lived stuff that for a reason or another cannot go in the scopped allocator.
  /// Allocations are fast (as fast as the scoped allocator), deallocations are fast but contrary to the scoped allocator,
  /// memory is not re-used aggressively and fragmentation is not dealt with, which may lead to extreme wastes memory
  ///   (which is the reason why it's for transient resources, so fragmentation is kept at a minimum)
  ///
  /// \note ANYTHING that may end-up living more than 3 frames (or past a fence) should be in the persitent allocator
  /// \note allocations are very fast, and for resources that don't hold data more than one frame, it may be beneficial to move them to this allocator instead
  class transient_pool final : allocator_interface
  {
    public:
      transient_pool(block_allocator& _allocator) : allocator(_allocator) {}
      ~transient_pool();

      memory_allocation allocate(uint32_t size, uint32_t alignment);

      void free_allocation(const memory_allocation& mem) override;

    private:
      void do_allocate_block(size_t size);

    private:
      // NOTE we "leak" those to avoid paying the cost to track them.
      // The sub-allocations are still tracking those, so they aren't really leaked, but the pool itself does not keep track of them
      struct block_allocation
      {
        memory_allocation allocation;
        uint32_t remaining_suballocations = 0;
        bool has_allocation_ended = false;
        // spinlock ?
      };

      // FIXME: BAD
      spinlock lock;

      block_allocator& allocator;

      block_allocation* current_block = nullptr;
      uint32_t offset = 0;
  };
}

