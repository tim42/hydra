//
// created by : Timothée Feuillet
// date: 2021-11-19
//
//
// Copyright (c) 2021 Timothée Feuillet
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


#include <cstdint>
#include <cstddef>

#include <ntools/enum.hpp>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      class device_memory;
    } // namespace vk
    class memory_allocator;

    enum class allocation_type : unsigned
    {
      none = 0,

      pass_local = 2 /* FIXME: 1 */, // The ressource will/must be discarded at the end of the pass.
      // NOTE: for intra-frame ressource re-use there's utilities for that. single_frame will not reuse allocations but provide facilities for such uses.
      single_frame = 2, // The resource will be released once the gpu frame has ended. Fast allocation, free deallocation.
      short_lived = 3, // The resource will be put in a short lived pool.
      persistent = 4, // The resource will be allocated in the long-lived pools or will have its own allocation if necessary. Allocation is slower as the system tries to minimize fragmentation.

      block_level = 5, // The allocation is directly mapped to blocks, without going through a sub-allocator
      raw = 6, // The allocation maps 1:1 with a device allocation, usually for very big allocations

      optimal_image = 1 << 6, // images on some gpus may need to have separate allocations
      mapped_memory = 1 << 7, // request an allocation in the pool of pre-mapped memory, flushing the memory might be necessary.

      _flags = optimal_image|mapped_memory, // do not use.

      single_frame_optimal_image = single_frame | optimal_image,
      short_lived_optimal_image = short_lived | optimal_image,
      persistent_optimal_image = persistent | optimal_image,
    };
    N_ENUM_FLAG(allocation_type)


    /// \brief Represent a memory allocation
    /// \note Allocations are RAII.
    struct memory_allocation
    {
      public:
        const vk::device_memory* mem() const { return alloc_type == allocation_type::raw ? &owned_memory : _mem; }
        size_t offset() const { return alloc_type == allocation_type::raw ? 0 : _offset; }
        size_t size() const { return alloc_type == allocation_type::raw ? owned_memory.get_size() : _size; }
        memory_allocator *allocator() const { return _allocator; }

        bool is_valid() const { return alloc_type == allocation_type::raw ? owned_memory.is_allocated() : (_mem != nullptr); }

        /// \brief Free the memory
        /// Implemented in memory_allocator.hpp
        void free();

        uint32_t _type_index() const { return type_index; }
        allocation_type _get_allocation_type() const { return alloc_type; }

        void* _get_payload() const { return alloc_type == allocation_type::raw ? nullptr : _payload; }

      public: // advanced
        memory_allocation(uint32_t _type_index, allocation_type _alloc_type, size_t offset, size_t size, const vk::device_memory *mem, memory_allocator *allocator, void* payload)
         : type_index(_type_index),
           alloc_type(_alloc_type),
           _allocator(allocator),
           _offset(offset),
           _size(size),
           _mem(mem),
           _payload(payload)
        {}

        memory_allocation(uint32_t _type_index, memory_allocator *allocator, vk::device_memory&& _owned_memory)
         : type_index(_type_index),
           alloc_type(allocation_type::raw),
           _allocator(allocator),
           owned_memory(std::move(_owned_memory))
        {}

        memory_allocation() : memory_allocation(~0u, allocation_type::none, ~0ull, 0, nullptr, nullptr, nullptr) {}

        memory_allocation(memory_allocation &&o)
          : type_index(o.type_index),
            alloc_type(o.alloc_type),
            _allocator(o._allocator)
        {
          o._allocator = nullptr;

          if (alloc_type != allocation_type::raw)
          {
            _offset = (o._offset);
            _size = (o._size);
            _mem = (o._mem);
            _payload = (o._payload);

            o._mem = nullptr;
            o._size = 0;
            o._offset = ~(size_t)0;
          }
          else
          {
            new(&owned_memory) vk::device_memory(std::move(o.owned_memory));
          }
        }

        memory_allocation &operator = (memory_allocation &&o)
        {
          if (&o == this)
            return *this;

          free();

          type_index = (o.type_index);
          alloc_type = o.alloc_type;
          _allocator = (o._allocator);

          o._allocator = nullptr;

          if (alloc_type != allocation_type::raw)
          {
            _offset = (o._offset);
            _size = (o._size);
            _mem = (o._mem);
            _payload = (o._payload);

            o._mem = nullptr;
            o._size = 0;
            o._offset = ~(size_t)0;
          }
          else
          {
            new(&owned_memory) vk::device_memory(std::move(o.owned_memory));
          }

          return *this;
        }

        ~memory_allocation()
        {
          free();
        }

      private:
        uint32_t type_index;
        allocation_type alloc_type;
        memory_allocator *_allocator = nullptr;
        union
        {
          struct // allocation type != raw
          {
            size_t _offset;
            size_t _size;
            const vk::device_memory *_mem = nullptr;
            void* _payload = nullptr;
          };
          vk::device_memory owned_memory;
        };
    };
  } // namespace hydra
} // namespace neam


