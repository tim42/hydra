//
// file : memory_allocation.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/memory_allocation.hpp
//
// created by : Timothée Feuillet
// date: Thu Sep 01 2016 13:04:11 GMT+0200 (CEST)
//
//
// Copyright (c) 2016 Timothée Feuillet
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

#ifndef __N_106284930325156748_28512937_MEMORY_ALLOCATION_HPP__
#define __N_106284930325156748_28512937_MEMORY_ALLOCATION_HPP__

#include <cstdint>
#include <cstddef>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      class device_memory;
    } // namespace vk
    class memory_allocator;

    enum class allocation_type : int
    {
      normal        =      0,
      optimal_image = 1 << 1,
      short_lived   = 1 << 2,

      short_lived_optimal_image = short_lived | optimal_image,
    };


    /// \brief Represent a memory allocation
    struct memory_allocation
    {
      public:
        const vk::device_memory *mem() const { return _mem; }
        size_t offset() const { return _offset; }
        size_t size() const { return _size; }
        memory_allocator *allocator() const { return _allocator; }

        /// \brief Free the memory
        /// Implemented in memory_allocator.hpp
        void free() const;

        bool _is_non_shared() const { return non_shared; }
        uint32_t _type_index() const { return type_index; }
        allocation_type _get_allocation_type() const { return alltype; }

      public: // advanced
        memory_allocation(uint32_t _type_index, bool _non_shared, allocation_type _alltype, size_t offset, size_t size, const vk::device_memory *mem, memory_allocator *allocator)
         : type_index(_type_index), non_shared(_non_shared), alltype(_alltype), _offset(offset), _size(size), _mem(mem), _allocator(allocator)
        {}
        memory_allocation() : memory_allocation(-1, false, allocation_type::normal, -1, 0, nullptr, nullptr) {}

        memory_allocation(const memory_allocation &o)
          : type_index(o.type_index), non_shared(o.non_shared), alltype(o.alltype), _offset(o._offset), _size(o._size),
            _mem(o._mem), _allocator(o._allocator)
        {
//           o._allocator = nullptr;
//           o._mem = nullptr;
//           o._size = 0;
//           o._offset = (size_t)-1;
        }

        memory_allocation &operator = (const memory_allocation &o)
        {
          if (&o == this)
            return *this;
          type_index = (o.type_index);
          non_shared = (o.non_shared);
          alltype = o.alltype;
          _offset = (o._offset);
          _size = (o._size);
          _mem = (o._mem);
          _allocator = (o._allocator);

          return *this;
        }

      private:
        uint32_t type_index;
        bool non_shared;
        allocation_type alltype;
        size_t _offset;
        size_t _size;
        const vk::device_memory *_mem;
        memory_allocator *_allocator;
    };
  } // namespace hydra
} // namespace neam


#endif // __N_106284930325156748_28512937_MEMORY_ALLOCATION_HPP__

