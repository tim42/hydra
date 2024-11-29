//
// file : image_subresource_layers.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/image_subresource_layers.hpp
//
// created by : Timothée Feuillet
// date: Fri Aug 30 2024 21:42:21 GMT+0200 (CEST)
//
//
// Copyright (c) 2024Timothée Feuillet
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


namespace neam::hydra::vk
{
  /// \brief Wraps a VkImageSubresource
  class image_subresource
  {
    public:
      /// \brief Create the image subresource layers.
      /// _layers parameters have the first component (x) as the base layer and the
      /// second (y) as the number of layers included in the subresource layers
      image_subresource(VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT, uint32_t mip_level = 0,
                              uint32_t layer = 0)
        : vk_is {aspect_mask, mip_level, layer }
      {}
      image_subresource(uint32_t mip_level,
                        uint32_t layer = 0)
        : vk_is {VK_IMAGE_ASPECT_COLOR_BIT, mip_level, layer }
      {}

      image_subresource(const image_subresource &o) = default;
      image_subresource &operator = (const image_subresource &o) = default;
      image_subresource(const VkImageSubresource &o) : vk_is(o) {}
      image_subresource &operator = (const VkImageSubresource &o) { vk_is = o; return *this; }

      /// \brief Return the aspect mask of the sub-resource (what kind of data is included in the subresource)
      VkImageAspectFlags get_aspect_mask() const { return vk_is.aspectMask; }
      /// \brief Set the aspect mask of the sub-resource (what kind of data is included in the subresource)
      void set_aspect_mask(VkImageAspectFlags aspect_mask) { vk_is.aspectMask = aspect_mask; }

      /// \brief Return the mipmap level
      uint32_t get_mipmap_level() const { return vk_is.mipLevel; }
      /// \brief Set the mipmap level
      void set_mipmap_level(uint32_t level) { vk_is.mipLevel = level; }

      /// \brief Return the layer
      uint32_t get_layer() const { return vk_is.arrayLayer; }
      /// \brief Set the layer
      void set_layer(uint32_t layer) { vk_is.arrayLayer = layer; }

    public: // advanced
      /// \brief Yield a const reference to a VkImageSubresource
      operator const VkImageSubresource &() const { return vk_is; }
    private:
      VkImageSubresource vk_is;
  };
} // namespace neam




