//
// file : memory_allocator_chunk_chain.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/memory_allocator_chunk_chain.hpp
//
// created by : Timothée Feuillet
// date: Fri Sep 02 2016 15:51:26 GMT+0200 (CEST)
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

#ifndef __N_504010956183331340_3179031897_MEMORY_ALLOCATOR_CHUNK_CHAIN_HPP__
#define __N_504010956183331340_3179031897_MEMORY_ALLOCATOR_CHUNK_CHAIN_HPP__

#include "memory_allocator_chunk.hpp"
#include "memory_allocation.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief Manages a chain of memory_allocator_chunk
    class memory_allocator_chunk_chain
    {
      public:
        memory_allocator_chunk_chain() {}
        memory_allocator_chunk_chain(memory_allocator_chunk_chain &&o)
          : memory_type_index(o.memory_type_index), allocator(o.allocator), 
            type(o.type), dev(o.dev),
            chunk_list(std::move(o.chunk_list)), dev_to_mac(std::move(o.dev_to_mac))
        {}
        ~memory_allocator_chunk_chain() {}

        /// \brief Set the memory type index
        void set_memory_type_index(uint32_t mti) { memory_type_index = mti; }
        /// \brief Set the memory allocator
        void set_memory_allocator(memory_allocator *ma) { allocator = ma; }
        /// \brief Set the allocation type
        void set_allocation_type(allocation_type _type) { type = _type; }
        /// \brief Set the device
        void set_vk_device(vk::device *_dev) { dev = _dev; }


        memory_allocation allocate_memory(size_t size, size_t alignment)
        {
          for (memory_allocator_chunk &it : chunk_list)
          {
            if (it.can_allocate(size)) // fast heuristic to determinate if that chunk can allocate the requested memory
            {
              it.print_stats();
              int offset = it.allocate(size, alignment);
              it.print_stats();
              if (offset == -1)
                continue; // how unfortunate :/
              return memory_allocation(memory_type_index, false, type, offset, size, &it.get_device_memory(), allocator);
            }
          }

          // If we get here, we don't have enough chunk...
          chunk_list.emplace_front(*dev, memory_type_index);
          memory_allocator_chunk &it = chunk_list.front();
          dev_to_mac.emplace(std::make_pair(&it.get_device_memory(), &it));
          int offset = it.allocate(size, alignment);
          check::on_vulkan_error::n_assert(offset != -1, "m_a_c_c::allocate(): allocation failed in an empty chunk. yay.");
          return memory_allocation(memory_type_index, false, type, offset, size, &it.get_device_memory(), allocator);
        }

        void free_memory(const memory_allocation &ma)
        {
          memory_allocator_chunk *mac;
          auto dtm_it = dev_to_mac.find(ma.mem());
          check::on_vulkan_error::n_assert(dtm_it != dev_to_mac.end(), "m_a_c_c::free_memory(): invalid free (can't find the corresponding memory_allocator_chunk)");

          mac = dtm_it->second;

          mac->free(ma.offset(), ma.size());

          if (mac->is_empty()) // remove empty chunks
          {
            chunk_list.remove_if([&](const memory_allocator_chunk &_mac)
            {
              return &_mac.get_device_memory() == ma.mem();
            });
          }
        }

      private:
        uint32_t memory_type_index = 0;
        memory_allocator *allocator = nullptr;
        allocation_type type;
        vk::device *dev;

        std::list<memory_allocator_chunk> chunk_list; // normal chunk list
        std::unordered_map<const vk::device_memory *, memory_allocator_chunk *> dev_to_mac;
    };
  } // namespace hydra
} // namespace neam


#endif // __N_504010956183331340_3179031897_MEMORY_ALLOCATOR_CHUNK_CHAIN_HPP__

