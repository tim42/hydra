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

#pragma once


#include "memory_allocator_chunk.hpp"
#include "memory_allocation.hpp"
#if 0
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
            chunk_list(std::move(o.chunk_list))
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


        /*memory_allocation allocate_memory(size_t size, size_t alignment)
        {
          check::on_vulkan_error::n_assert(size < memory_allocator_chunk<>::chunk_allocation_size, "m_a_c_c::allocate(): cannot allocate more than the chunk size");
          check::on_vulkan_error::n_assert(size != 0, "m_a_c_c::allocate(): cannot allocate nothing");

          for (auto &it : chunk_list)
          {
              unsigned offset = it.first.allocate(size, alignment);
              if (offset == memory_allocator_chunk<>::invalid_allocation)
                continue; // how unfortunate :/
              ++allocation_count;
              return memory_allocation(memory_type_index, false, type, offset, size, &it.second, allocator, &it.first);
          }
          // If we get here, we don't have enough chunk...
          chunk_list.emplace_front(memory_allocator_chunk<>(), vk::device_memory::allocate(*dev, memory_allocator_chunk<>::chunk_allocation_size, memory_type_index));
          auto &it = chunk_list.front();
          int offset = it.first.allocate(size, alignment);
          check::on_vulkan_error::n_assert(offset != -1, "m_a_c_c::allocate(): allocation failed in an empty chunk. yay.");
          ++allocation_count;
          return memory_allocation(memory_type_index, false, type, offset, size, &it.second, allocator, &it.first);
        }*/

        void free_memory(const memory_allocation &ma)
        {
          memory_allocator_chunk<> *mac = reinterpret_cast<memory_allocator_chunk<> *>(ma._get_payload());
          check::on_vulkan_error::n_assert(mac != nullptr, "m_a_c_c::free_memory(): nullptr payload.");

          mac->free(ma.offset(), ma.size());

          --allocation_count;

          if (mac->is_empty()) // remove empty chunks
          {
//             chunk_list.remove_if([&](const std::pair<memory_allocator_chunk<>, vk::device_memory> &_mac)
//             {
//               return &_mac.second == ma.mem();
//             });
//             auto it = std::find_if(chunk_list.begin(), chunk_list.end(), [&](const std::pair<memory_allocator_chunk<>, vk::device_memory> &_mac)
//             {
//               return &_mac.second == ma.mem();
//             });
//             std::swap(chunk_list[chunk_list.begin() - it], chunk_list.back());
//             chunk_list.pop_back();
            while (chunk_list.size() > 0 && chunk_list.front().first.is_empty())
            {
              chunk_list.pop_front();
            }
          }
        }

        void print_stats(const std::string& /*name*/) const
        {
          size_t free_memory = 0;
          size_t total_memory = 0;
          size_t chunk_count = 0;
          for (const auto& chunk : chunk_list)
          {
            free_memory += chunk.first.get_total_free_memory();
            total_memory += chunk.second.get_size();
            ++chunk_count;
          }

          if (chunk_count == 0)
            return;

//           cr::get_global_logger().log() << "chain@" << this << "[" << name << "]:" << cr::newline
//                              << "  free memory:           " << (free_memory / (1024.f * 1024.f)) << " Mio" << cr::newline
//                              << "  allocated memory:      " << ((total_memory - free_memory) / (1024.f * 1024.f)) << " Mio" << cr::newline
//                              << "  total memory:          " << (total_memory / (1024.f * 1024.f)) << " Mio" << cr::newline
//                              << "  number of allocations: " << allocation_count << cr::newline
//                              << "  number of chunk:       " << (chunk_count) << std::endl;
        }

        size_t get_chain_reserved_memory() const
        {
          return chunk_list.size() * memory_allocator_chunk<>::chunk_allocation_size;
        }

      private:
        uint32_t memory_type_index = 0;
        memory_allocator *allocator = nullptr;
        allocation_type type;
        vk::device *dev;

        size_t allocation_count = 0;

        std::deque<std::pair<memory_allocator_chunk<>, vk::device_memory>> chunk_list; // normal chunk list
    };
  } // namespace hydra
} // namespace neam

#endif


