//
// file : memory_allocator.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/memory_allocator.hpp
//
// created by : Timothée Feuillet
// date: Tue Aug 30 2016 18:37:47 GMT+0200 (CEST)
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

#ifndef __N_167156602804017135_753626619_MEMORY_ALLOCATOR_HPP__
#define __N_167156602804017135_753626619_MEMORY_ALLOCATOR_HPP__

#include <list>
#include <unordered_map>
#include <cstdint>

#include "../hydra_exception.hpp"
#include "../vulkan/device.hpp"
#include "../vulkan/device_memory.hpp"

#include "memory_allocation.hpp"
#include "memory_allocator_chunk.hpp"
#include "memory_allocator_chunk_chain.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief An utility to manage GPU memory and perform less allocations
    /// \note It may not be ultra-efficient but should give correct results
    ///
    /// It uses chunks (buckets ?) described by hierarchical bitmaps.
    /// The minimum sub-allocation size is 512bytes
    class memory_allocator
    {
      public:
        /// \brief Create the memory allocator
        /// \note chunks / allocations greater than the default chunk size will have
        ///       their own vk::device_memory (allocation) and won't be kept when freed.
        /// \note The chunk size is 8Mio, meaning that at most 1024 allocations can be done
        ///       on a 8Gio device. The allocator is guaranteed to never allocate less than
        ///       the chunk size.
        memory_allocator(vk::device &_dev) : dev(_dev)
        {
          memory_types.resize(dev.get_physical_device().get_memory_property().memoryTypeCount);
          for (size_t i = 0; i < memory_types.size(); ++i)
          {
            memory_types[i].set_memory_type_index(i);
            memory_types[i].set_memory_allocator(this);
            memory_types[i].set_vk_device(&dev);
          }

#ifndef HYDRA_NO_MESSAGES
          // A lovely print
          memory_allocator_chunk::print_nfo();
#endif
        }

        /// \brief Free the memory, destroy the allocator
        ~memory_allocator() {}

        /// \brief Allocate some memory (throw on error)
        /// \param short_lived Indicate whether or not the memory will be freed soon.
        ///                    You may gain performance (and have less fragmentation) in the long term
        ///                    if you correctly set this flag
        memory_allocation allocate_memory(const VkMemoryRequirements &reqs, VkMemoryPropertyFlags flags, bool short_lived = false)
        {
          uint32_t mti = vk::device_memory::get_memory_type_index(dev, flags, reqs.memoryTypeBits);
          check::on_vulkan_error::n_assert(mti != (uint32_t)-1, "could not find a suitable memory type to allocate");
          return allocate_memory(reqs.size, reqs.alignment, mti, short_lived);
        }

        /// \brief Allocate some memory (throw on error)
        /// \note currently, alignment is unused as everything is allocated on a 512-byte boundary.
        /// \param short_lived Indicate whether or not the memory will be freed soon.
        /// \param short_lived Indicate whether or not the memory will be freed soon.
        ///                    You may gain performance (and have less fragmentation) in the long term
        ///                    if you correctly set this flag
        memory_allocation allocate_memory(size_t size, uint32_t alignment, uint32_t memory_type_index, bool short_lived = false)
        {
          (void) alignment;

          if (size > memory_allocator_chunk::chunk_allocation_size)
          {
            // non-shared allocation, we have a requested size too big to fit in a chunk
            non_shared_list.emplace_back(vk::device_memory::allocate(dev, size, memory_type_index));

            dmlist_iterator it = --non_shared_list.end();
            non_shared_map.emplace((intptr_t)&(*it), it);

            // update stats
            ++allocation_count;
            allocated_memory += size;
            used_memory += size;
            return memory_allocation(memory_type_index, true, 0, size, &(*it), this);
          }
          else
          {
            check::on_vulkan_error::n_assert(memory_type_index < memory_types.size(), "allocate_memory(): invalid memory type index");
            if (short_lived)
              return memory_types[memory_type_index].allocate_memory_short_lived(size);
            else
              return memory_types[memory_type_index].allocate_memory(size);
          }
        }

        /// \brief Return a chunk of memory previously allocated
        /// \note This isn't done automatically by vk:: objects, so you may need
        /// to do that by yourself when the object is destroyed or another memory
        /// is bound to it
        /// May throw on error
        /// \see memory_allocation::free()
        void free_memory(const memory_allocation &mem)
        {
          if (mem._is_non_shared()) // check in the non_shared map&list
          {
            check::on_vulkan_error::n_assert(mem.offset() == 0, "free_memory(): invalid free (invalid offset in device memory)");
            auto map_it = non_shared_map.find((intptr_t)mem.mem());
            check::on_vulkan_error::n_assert(map_it != non_shared_map.end(), "free_memory(): invalid free / double free");

            dmlist_iterator it = map_it->second;
            check::on_vulkan_error::n_assert(it->is_allocated() == true, "free_memory(): double free");
            it->free(); // explicitly free the memory
            non_shared_list.erase(it);
            non_shared_map.erase(map_it);

            // update stats
            allocation_count -= 1;
            allocated_memory -= mem.size();
            used_memory -= mem.size();
          }
          else
          {
            check::on_vulkan_error::n_assert(mem._type_index() < memory_types.size(), "allocate_memory(): invalid memory type index");
            memory_types[mem._type_index()].free_memory(mem);
          }
        }

      private:
        vk::device &dev;

        // stats //
        size_t allocation_count = 0;
        size_t allocated_memory = 0;
        size_t used_memory = 0;

        // types //
        using dmlist_iterator = std::list<vk::device_memory>::iterator;

        // Non-shared allocations //
        std::list<vk::device_memory> non_shared_list; // list of all allocations
        std::unordered_map<intptr_t, dmlist_iterator> non_shared_map; // a fast way to get the list entry

        // shared allocations //
        std::vector<memory_allocator_chunk_chain> memory_types;
    };


    // // // //


    void memory_allocation::free() const
    {
      if (_allocator)
        _allocator->free_memory(*this);
    }
  } // namespace hydra
} // namespace neam

#endif // __N_167156602804017135_753626619_MEMORY_ALLOCATOR_HPP__

