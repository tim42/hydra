//
// file : image_subresource_range.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/image_subresource_range.hpp
//
// created by : Timothée Feuillet
// date: Thu Aug 25 2016 11:43:54 GMT+0200 (CEST)
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


namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkImageSubresourceRange
      class image_subresource_range
      {
        public:
          /// \brief Create the image subresource range.
          /// _range parameters have the first component (x) as the base layer/mipmap level and the
          /// second (y) as the number of mipmaps / layers included in the subresource range
          image_subresource_range(VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT, const glm::uvec2 &mips_range = glm::uvec2(0, VK_REMAINING_MIP_LEVELS),
                                  const glm::uvec2 &layer_range = glm::uvec2(0, 1))
            : vk_isr {aspect_mask, mips_range.x, mips_range.y, layer_range.x, layer_range.y }
          {}

          image_subresource_range(const image_subresource_range &o) : vk_isr(o.vk_isr) {}
          image_subresource_range &operator = (const image_subresource_range &o) { vk_isr = o.vk_isr; return *this; }
          image_subresource_range(const VkImageSubresourceRange &o) : vk_isr(o) {}
          image_subresource_range &operator = (const VkImageSubresourceRange &o) { vk_isr = o; return *this; }

          /// \brief Return the aspect mask of the sub-resource (what kind of data is included in the subresource)
          VkImageAspectFlags get_aspect_mask() const { return vk_isr.aspectMask; }
          /// \brief Set the aspect mask of the sub-resource (what kind of data is included in the subresource)
          void set_aspect_mask(VkImageAspectFlags aspect_mask) { vk_isr.aspectMask = aspect_mask; }

          /// \brief Return the mipmap range (X is the base level, Y is the number of level)
          glm::uvec2 get_mipmap_range() const { return glm::uvec2(vk_isr.baseMipLevel, vk_isr.levelCount); }
          /// \brief Set the mipmap range (X is the base level, Y is the number of level)
          void set_mipmap_range(const glm::uvec2 &range) { vk_isr.baseMipLevel = range.x; vk_isr.levelCount = range.y; }

          /// \brief Return the layer range (X is the base layer, Y is the number of layers)
          glm::uvec2 get_layer_range() const { return glm::uvec2(vk_isr.baseArrayLayer, vk_isr.layerCount); }
          /// \brief Set the layer range (X is the base level, Y is the number of layers)
          void set_layer_range(const glm::uvec2 &range) { vk_isr.baseArrayLayer = range.x; vk_isr.layerCount = range.y; }

        public: // advanced
          /// \brief Yield a const reference to a VkImageSubresourceRange
          operator const VkImageSubresourceRange &() const { return vk_isr; }
        private:
          VkImageSubresourceRange vk_isr;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



