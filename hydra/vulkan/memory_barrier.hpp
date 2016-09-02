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

#include "image.hpp"
#include "buffer.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief A wrapper around VkMemoryBarrier
      class memory_barrier : public VkMemoryBarrier
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
      };
      static_assert(sizeof(memory_barrier) == sizeof(VkMemoryBarrier), "compiler is not compatible with hydra");

      /// \brief A wrapper around VkBufferMemoryBarrier
      class buffer_memory_barrier : public VkBufferMemoryBarrier
      {
        public:
          /// \brief Initialize the memory_barrier
          buffer_memory_barrier(const ::neam::hydra::vk::buffer &_buffer, VkAccessFlags src_access_mask = 0, VkAccessFlags dest_access_mask = 0)
          {
            sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            pNext = nullptr;
            srcAccessMask = src_access_mask;
            dstAccessMask = dest_access_mask;
            buffer = _buffer._get_vk_buffer();
            size = VK_WHOLE_SIZE;
            offset = 0;
            srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          }

          /// \brief Set both source and destination access masks
          void set_access_masks(VkAccessFlags src_access_mask, VkAccessFlags dest_access_mask)
          {
            srcAccessMask = src_access_mask;
            dstAccessMask = dest_access_mask;
          }
      };
      static_assert(sizeof(buffer_memory_barrier) == sizeof(VkBufferMemoryBarrier), "compiler is not compatible with hydra");

      /// \brief A wrapper around VkImageMemoryBarrier
      class image_memory_barrier : public VkImageMemoryBarrier
      {
        public:
          /// \brief Initialize the memory_barrier
          image_memory_barrier(const ::neam::hydra::vk::image &_image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT)
          {
            sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            pNext = nullptr;
            srcAccessMask = 0;
            dstAccessMask = 0;
            srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            image = _image.get_vk_image();
            oldLayout = old_layout;
            newLayout = new_layout;
            subresourceRange.aspectMask = aspect_mask;

            // default values for 2D image, without mipmaps
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;

            autoset_access_masks();
          }

          /// \brief Set both source and destination access masks
          void set_access_masks(VkAccessFlags src_access_mask, VkAccessFlags dest_access_mask)
          {
            srcAccessMask = src_access_mask;
            dstAccessMask = dest_access_mask;
          }

          /// \brief Set the old and the new layouts
          void set_layouts(VkImageLayout old_layout, VkImageLayout new_layout)
          {
            oldLayout = old_layout;
            newLayout = new_layout;
          }

          /// \brief Helper to set the subresourceRange member
          void set_subresource_range(const VkImageSubresourceRange &isr)
          {
            subresourceRange.aspectMask = isr.aspectMask;
            subresourceRange.baseArrayLayer = isr.baseArrayLayer;
            subresourceRange.baseMipLevel = isr.baseMipLevel;
            subresourceRange.layerCount = isr.layerCount;
            subresourceRange.levelCount = isr.levelCount;
          }

          /// \brief Autoset the access masks from the layout member
          /// \note I have no idea if this will work in most of the cases.
          void autoset_access_masks()
          {
            if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
              srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
              dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
              srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
              dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
              srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
              dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            }
          }
      };
      static_assert(sizeof(image_memory_barrier) == sizeof(VkImageMemoryBarrier), "compiler is not compatible with hydra");
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_2556089501642613882_1390822852_MEMORY_BARRIER_HPP__

