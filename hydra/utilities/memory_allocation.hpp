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

#include <hydra/vulkan/device_memory.hpp>
#include <hydra/utilities/allocator/allocator.hpp>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      class device_memory;
    } // namespace vk
    class allocator_interface;

    enum class allocation_type : unsigned
    {
      none = 0,

      scoped = 2, // The resource has a very short lifetime.
                  // Aggressive memory re-usage, super fast allocation / no deallocation cost

      short_lived = 3, // The resource will be put in a short-lived pool.
                       // Things that should be scoped but are also mapped_memory should use this
                       // super fast allocations / deallocations, fragmentation is ignored.

      persistent = 4, // The resource will be allocated in the long-lived pools or will have its own allocation if necessary.
                      // De/allocations might be slower as the system tries to minimize fragmentation.

      block_level = 5, // The allocation is directly mapped to blocks, without going through a sub-allocator
                       // This is normally automatically done for resources with the correct size range. It's faster, but may waste some memory.
      raw = 6, // The allocation maps 1:1 with a device allocation, usually for very big allocations.
               // WARNING: Can be VERY slow if used repeatedly (upward of 100ms) and waste memory.

      optimal_image = 1 << 6, // images on some gpus may need to have separate allocations, this flag indicates it's for an optimaly tiled image
      mapped_memory = 1 << 7, // request an allocation in the pool of pre-mapped memory, flushing the memory might be necessary.

      _flags = optimal_image|mapped_memory, // do not use.

      scoped_optimal_image = scoped | optimal_image,
      short_lived_optimal_image = short_lived | optimal_image,
      persistent_optimal_image = persistent | optimal_image,
    };
    N_ENUM_FLAG(allocation_type)


    /// \brief Represent a memory allocation
    /// \note Allocations are RAII.
    class memory_allocation
    {
      public:
        const vk::device_memory* mem() const { return alloc_type == allocation_type::raw ? &owned_memory : _mem; }
        size_t offset() const { return alloc_type == allocation_type::raw ? 0 : _offset; }
        size_t size() const { return alloc_type == allocation_type::raw ? owned_memory.get_size() : _size; }
        allocator_interface *allocator() const { return _allocator; }

        bool is_valid() const { return alloc_type == allocation_type::raw ? owned_memory.is_allocated() : (_mem != nullptr); }

        /// \brief Free the memory
        void free()
        {
          if (alloc_type == allocation_type::none)
            return;

          if (_allocator != nullptr)
          {
            _allocator->free_allocation(*this);
          }

          if (alloc_type == allocation_type::raw)
          {
            owned_memory.free();
            owned_memory.~device_memory();
          }

          _allocator = nullptr;
          alloc_type = allocation_type::none;
        }

        uint32_t _type_index() const { return type_index; }
        allocation_type _get_allocation_type() const { return alloc_type; }

        void* _get_payload() const { return alloc_type == allocation_type::raw ? nullptr : _payload; }

      public: // advanced
        memory_allocation(uint32_t _type_index, allocation_type _alloc_type, size_t offset, size_t size, const vk::device_memory *mem, allocator_interface *allocator, void* payload)
         : type_index(_type_index),
           alloc_type(_alloc_type),
           _allocator(allocator),
           _offset(offset),
           _size(size),
           _mem(mem),
           _payload(payload)
        {}

        memory_allocation(uint32_t _type_index, allocator_interface *allocator, vk::device_memory&& _owned_memory)
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

        void _set_new_size(size_t new_size)
        {
          check::debug::n_assert(alloc_type != allocation_type::raw, "_set_new_size: cannot resize raw_allocations");
          check::debug::n_assert(_offset + new_size <= _mem->get_size(), "_set_new_size: cannot grow above the underlying raw-allocation");
          _size = new_size;
        }

      private:
        uint32_t type_index;
        allocation_type alloc_type;
        allocator_interface *_allocator = nullptr;
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


