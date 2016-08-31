//
// file : image_copy_area.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/image_copy_area.hpp
//
// created by : Timothée Feuillet
// date: Thu Aug 25 2016 13:17:40 GMT+0200 (CEST)
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

#ifndef __N_19939691371062712_2840632657_IMAGE_COPY_AREA_HPP__
#define __N_19939691371062712_2840632657_IMAGE_COPY_AREA_HPP__

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "image_subresource_layers.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkImageCopy
      class image_copy_area
      {
        public:
          /// \brief Initialize a copy area for a 3D image
          image_copy_area(const glm::ivec3 &src_offset, const glm::uvec3 &area_size, const glm::ivec3 &dst_offset,
                          const image_subresource_layers &src_subres = image_subresource_layers(),
                          const image_subresource_layers &dst_subres = image_subresource_layers())
            : vk_ic
          {
            src_subres,
            { src_offset.x, src_offset.y, src_offset.z },
            dst_subres,
            { dst_offset.x, dst_offset.y, dst_offset.z },
            { area_size.x, area_size.y, area_size.z }
          }
          {}
          /// \brief Initialize a copy area for a 2D image
          image_copy_area(const glm::ivec2 &src_offset, const glm::uvec2 &area_size, const glm::ivec2 &dst_offset,
                          const image_subresource_layers &src_subres = image_subresource_layers(),
                          const image_subresource_layers &dst_subres = image_subresource_layers())
            : vk_ic
          {
            src_subres,
            { src_offset.x, src_offset.y, 0 },
            dst_subres,
            { dst_offset.x, dst_offset.y, 0 },
            { area_size.x, area_size.y, 1 }
          }
          {}
          /// \brief Initialize a copy area for a 1D image
          image_copy_area(int32_t src_offset, uint32_t area_size, int32_t dst_offset,
                          const image_subresource_layers &src_subres = image_subresource_layers(),
                          const image_subresource_layers &dst_subres = image_subresource_layers())
            : vk_ic
          {
            src_subres,
            { src_offset, 0, 0 },
            dst_subres,
            { dst_offset, 0, 0 },
            { area_size, 1, 1 }
          }
          {}

          /// \brief Return the offset in the source image
          /// \note in 2D image Z is unused and in 1D images Z and Y are unused
          glm::ivec3 get_source_offset() const { return glm::ivec3(vk_ic.srcOffset.x, vk_ic.srcOffset.y, vk_ic.srcOffset.z); }
          /// \brief Return the offset in the destination image
          /// \note in 2D image Z is unused and in 1D images Z and Y are unused
          glm::ivec3 get_dest_offset() const { return glm::ivec3(vk_ic.dstOffset.x, vk_ic.dstOffset.y, vk_ic.dstOffset.z); }
          /// \brief Return the size of the area to copy
          /// \note in 2D image Z is unused and in 1D images Z and Y are unused
          glm::uvec3 get_area_size() const { return glm::ivec3(vk_ic.extent.width, vk_ic.extent.height, vk_ic.extent.depth); }

          /// \brief Set the offset in the source image
          /// \note in 2D image Z is unused and in 1D images Z and Y are unused
          void set_source_offset(const glm::ivec3 &offset) { vk_ic.srcOffset.x = offset.x; vk_ic.srcOffset.y = offset.y; vk_ic.srcOffset.z = offset.z; }
          /// \brief Set the offset in the source image
          void set_source_offset(const glm::ivec2 &offset) { set_source_offset(glm::ivec3(offset, 0)); }
          /// \brief Set the offset in the source image
          void set_source_offset(int32_t offset) { set_source_offset(glm::ivec3(offset, 0, 0)); }

          /// \brief Set the offset in the destination image
          /// \note in 2D image Z is unused and in 1D images Z and Y are unused
          void set_dest_offset(const glm::ivec3 &offset) { vk_ic.dstOffset.x = offset.x; vk_ic.dstOffset.y = offset.y; vk_ic.dstOffset.z = offset.z; }
          /// \brief Set the offset in the source image
          void set_dest_offset(const glm::ivec2 &offset) { set_dest_offset(glm::ivec3(offset, 0)); }
          /// \brief Set the offset in the dest image
          void set_dest_offset(int32_t offset) { set_dest_offset(glm::ivec3(offset, 0, 0)); }

          /// \brief Set the size of the area to copy
          /// \note in 2D image Z is unused and in 1D images Z and Y are unused
          void set_area_size(const glm::uvec3 &sz) { vk_ic.extent.width = sz.x; vk_ic.extent.height = sz.y; vk_ic.extent.depth = sz.z; }
          /// \brief Set the offset in the source image
          void set_area_size(const glm::uvec2 &sz) { set_area_size(glm::uvec3(sz, 1)); }
          /// \brief Set the offset in the source image
          void set_area_size(uint32_t sz) { set_area_size(glm::uvec3(sz, 1, 1)); }

        public: // advanced
          /// \brief Yields a const reference to a VkImageCopy
          operator const VkImageCopy &() const { return vk_ic; }
        private:
          VkImageCopy vk_ic;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_19939691371062712_2840632657_IMAGE_COPY_AREA_HPP__

