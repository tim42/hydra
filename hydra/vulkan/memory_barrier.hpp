//
// file : memory_barrier.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/memory_barrier.hpp
//
// created by : Timothée Feuillet
// date: Fri Apr 29 2016 14:38:48 GMT+0200 (CEST)
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

#ifndef __N_2556089501642613882_1390822852_MEMORY_BARRIER_HPP__
#define __N_2556089501642613882_1390822852_MEMORY_BARRIER_HPP__

#include <vulkan/vulkan.h>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief A wrapper around VkMemoryBarrier
      class memory_barrier : private VkMemoryBarrier
      {
        public:
          /// \brief Initialize the memory_barrier
          memory_barrier(VkAccessFlags src_access_mask = 0, VkAccessFlags dest_access_mask = 0)
          {
            sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            pNext = nullptr;
            srcAccessMask = src_access_mask;
            dstAccessMask = dest_access_mask;
          }

          /// \brief Set both source and destination access masks
          void set_access_masks(VkAccessFlags src_access_mask, VkAccessFlags dest_access_mask)
          {
            srcAccessMask = src_access_mask;
            dstAccessMask = dest_access_mask;
          }

          /// \brief Set the source access mask
          void set_source_access_mask(VkAccessFlags access_mask)
          {
            srcAccessMask = access_mask;
          }

          /// \brief Set the source access mask
          void set_destination_access_mask(VkAccessFlags access_mask)
          {
            dstAccessMask = access_mask;
          }

          /// \brief Set the source access mask
          VkAccessFlags get_source_access_mask() const
          {
            return srcAccessMask;
          }

          /// \brief Set the source access mask
          VkAccessFlags get_destination_access_mask() const
          {
            return dstAccessMask;
          }

        public: // advanced
          /// \brief Return the vulkan memory_barrier object
          VkMemoryBarrier *get_vk_memory_barrier()
          {
            return this;
          }

          /// \brief Return the vulkan memory_barrier object
          const VkMemoryBarrier *get_vk_memory_barrier() const
          {
            return this;
          }
      };
      static_assert(sizeof(memory_barrier) == sizeof(VkMemoryBarrier), "compiler is not compatible with hydra");
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_2556089501642613882_1390822852_MEMORY_BARRIER_HPP__

