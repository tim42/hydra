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

#pragma once


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
            : buffer_memory_barrier(_buffer._get_vk_buffer(), src_access_mask, dest_access_mask)
          {
          }
          buffer_memory_barrier(VkBuffer _buffer, VkAccessFlags src_access_mask = 0, VkAccessFlags dest_access_mask = 0)
          {
            sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            pNext = nullptr;
            srcAccessMask = src_access_mask;
            dstAccessMask = dest_access_mask;
            buffer = _buffer;
            size = VK_WHOLE_SIZE;
            offset = 0;
            srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          }
          static buffer_memory_barrier queue_transfer(const VkBuffer _buffer, uint32_t src_queue_family_index, uint32_t dst_queue_family_index, VkAccessFlags src_access_mask = 0, VkAccessFlags dest_access_mask = 0)
          {
            buffer_memory_barrier barrier(_buffer);
            barrier.srcQueueFamilyIndex = src_queue_family_index;
            barrier.dstQueueFamilyIndex = dst_queue_family_index;
            barrier.srcAccessMask = src_access_mask;
            barrier.dstAccessMask = dest_access_mask;
            return barrier;
          }
          static buffer_memory_barrier queue_transfer(const ::neam::hydra::vk::buffer &_buffer, uint32_t src_queue_family_index, uint32_t dst_queue_family_index, VkAccessFlags src_access_mask = 0, VkAccessFlags dest_access_mask = 0)
          {
            return queue_transfer(_buffer._get_vk_buffer(), src_queue_family_index, dst_queue_family_index, src_access_mask, dest_access_mask);
          }
          /// \brief Set both source and destination access masks
          void set_access_masks(VkAccessFlags src_access_mask, VkAccessFlags dest_access_mask)
          {
            srcAccessMask = src_access_mask;
            dstAccessMask = dest_access_mask;
          }

          buffer_memory_barrier& set_queue_transfer(uint32_t src_queue_family_index, uint32_t dst_queue_family_index)
          {
            srcQueueFamilyIndex = src_queue_family_index;
            dstQueueFamilyIndex = dst_queue_family_index;
            return *this;
          }
      };
      static_assert(sizeof(buffer_memory_barrier) == sizeof(VkBufferMemoryBarrier), "compiler is not compatible with hydra");

      /// \brief A wrapper around VkImageMemoryBarrier
      class image_memory_barrier : public VkImageMemoryBarrier
      {
        public:
          /// \brief Initialize the memory_barrier
          image_memory_barrier(const ::neam::hydra::vk::image &_image, VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src_access_mask, VkAccessFlags dest_access_mask, VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT)
            : image_memory_barrier(_image.get_vk_image(), old_layout, new_layout, src_access_mask, dest_access_mask, aspect_mask)
          {
          }

          /// \brief Initialize the memory_barrier
          image_memory_barrier(VkImage _image, VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src_access_mask, VkAccessFlags dest_access_mask, VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT)
          {
            sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            pNext = nullptr;
            srcAccessMask = 0;
            dstAccessMask = 0;
            srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            image = _image;
            oldLayout = old_layout;
            newLayout = new_layout;
            subresourceRange.aspectMask = aspect_mask;

            // default values for 2D image, without mipmaps
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

            set_access_masks(src_access_mask, dest_access_mask);
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
          image_memory_barrier& set_subresource_range(const VkImageSubresourceRange &isr)
          {
            subresourceRange.aspectMask = isr.aspectMask;
            subresourceRange.baseArrayLayer = isr.baseArrayLayer;
            subresourceRange.baseMipLevel = isr.baseMipLevel;
            subresourceRange.layerCount = isr.layerCount;
            subresourceRange.levelCount = isr.levelCount;
            return *this;
          }

          image_memory_barrier& set_queue_transfer(uint32_t src_queue_family_index, uint32_t dst_queue_family_index)
          {
            srcQueueFamilyIndex = src_queue_family_index;
            dstQueueFamilyIndex = dst_queue_family_index;
            return *this;
          }

          static image_memory_barrier queue_transfer(VkImage _img, uint32_t src_queue_family_index, uint32_t dst_queue_family_index, VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src_access_mask = 0, VkAccessFlags dest_access_mask = 0)
          {
            image_memory_barrier barrier(_img, old_layout, new_layout, src_access_mask, dest_access_mask);
            barrier.srcQueueFamilyIndex = src_queue_family_index;
            barrier.dstQueueFamilyIndex = dst_queue_family_index;
            return barrier;
          }
          static image_memory_barrier queue_transfer(const ::neam::hydra::vk::image& _img, uint32_t src_queue_family_index, uint32_t dst_queue_family_index, VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src_access_mask = 0, VkAccessFlags dest_access_mask = 0)
          {
            return queue_transfer(_img.get_vk_image(), src_queue_family_index, dst_queue_family_index, old_layout, new_layout, src_access_mask, dest_access_mask);
          }

      };
      static_assert(sizeof(image_memory_barrier) == sizeof(VkImageMemoryBarrier), "compiler is not compatible with hydra");
    } // namespace vk
  } // namespace hydra
} // namespace neam



