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

#pragma once

#include "block_allocator.hpp"
#include "scoped_pool.hpp"
#include "transient_pool.hpp"

namespace neam::hydra::allocator
{
  class pool_set
  {
    public:
      pool_set(vk::device& dev, uint32_t memory_type_index, bool map_memory = false)
        : device(dev)
        , block(device, memory_type_index)
      {
        block._should_map_memory(map_memory);
      }

      memory_allocation allocate(size_t size, uint32_t alignment, allocation_type at);

      scoped_pool::scope push_scope() { return scoped.push_scope(); }

      uint64_t get_allocated_memory() const { return block.get_allocated_memory(); }

    private:
      vk::device& device;

      block_allocator block;
      scoped_pool scoped { block };
      transient_pool transient { block };
  };
}

