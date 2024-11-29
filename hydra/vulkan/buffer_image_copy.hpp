//
// file : buffer_image_copy.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/buffer_image_copy.hpp
//
// created by : Timothée Feuillet
// date: Fri Sep 02 2016 20:47:08 GMT+0200 (CEST)
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
#include <hydra_glm.hpp>


#include "image_subresource_layers.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkBufferImageCopy
      /// Designed to be the size of VkBufferImageCopy
      class buffer_image_copy
      {
        public:
          buffer_image_copy(size_t buffer_offset, const glm::ivec2 &image_offset, const glm::uvec2 &image_size, const image_subresource_layers &isl = image_subresource_layers())
            : buffer_image_copy(buffer_offset, glm::ivec3(image_offset, 0), glm::uvec3(image_size, 1), isl)
          {}
          buffer_image_copy(size_t buffer_offset, int32_t image_offset, uint32_t image_size, const image_subresource_layers &isl = image_subresource_layers())
            : buffer_image_copy(buffer_offset, glm::ivec3(image_offset, 0, 0), glm::uvec3(image_size, 1, 1), isl)
          {}
          buffer_image_copy(size_t buffer_offset, const glm::ivec3 &image_offset, const glm::uvec3 &image_size, const image_subresource_layers &isl = image_subresource_layers())
            : vk_bic
          {
            buffer_offset, 0, 0,
            isl,
            {image_offset.x, image_offset.y, image_offset.z},
            {image_size.x, image_size.y, image_size.z}
          }
          {}

          buffer_image_copy(const buffer_image_copy &o) : vk_bic(o.vk_bic) {}
          buffer_image_copy &operator = (const buffer_image_copy &o) {vk_bic = o.vk_bic; return *this; }
          buffer_image_copy(const VkBufferImageCopy &o) : vk_bic(o) {}
          buffer_image_copy &operator = (const VkBufferImageCopy &o) {vk_bic = o; return *this; }

          /// \brief Return buffer_offset
          size_t get_buffer_offset() const { return vk_bic.bufferOffset; }
          /// \brief Set buffer_offset
          void set_buffer_offset(size_t _buffer_offset) { vk_bic.bufferOffset = _buffer_offset; }

          /// \brief Return image_offset
          glm::ivec3 get_image_offset() const { return glm::ivec3(vk_bic.imageOffset.x, vk_bic.imageOffset.y, vk_bic.imageOffset.z); }
          /// \brief Set image_offset
          void set_image_offset(const glm::ivec3 &_image_offset) { vk_bic.imageOffset = {_image_offset.x, _image_offset.y, _image_offset.z}; }

          /// \brief Return image_size
          glm::uvec3 get_image_size() const { return glm::uvec3(vk_bic.imageExtent.width, vk_bic.imageExtent.height, vk_bic.imageExtent.depth); }
          /// \brief Set image_size
          void set_image_size(const glm::uvec3 &_image_size) { vk_bic.imageExtent = {_image_size.x, _image_size.y, _image_size.z}; }

          /// \brief Return image_subres
          image_subresource_layers get_image_subres() const { return vk_bic.imageSubresource; }
          /// \brief Set image_subres
          void set_image_subres(const image_subresource_layers &_image_subres) { vk_bic.imageSubresource = _image_subres; }

        public: // advanced
          /// \brief Yields a const reference to a VkBufferImageCopy
          operator const VkBufferImageCopy &() const { return vk_bic; }
        private:
          VkBufferImageCopy vk_bic;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



