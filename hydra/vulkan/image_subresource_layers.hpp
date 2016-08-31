//
// file : image_subresource_layers.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/image_subresource_layers.hpp
//
// created by : Timothée Feuillet
// date: Thu Aug 25 2016 13:12:00 GMT+0200 (CEST)
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

#ifndef __N_274878448709227246_3276410813_IMAGE_SUBRESOURCE_LAYERS_HPP__
#define __N_274878448709227246_3276410813_IMAGE_SUBRESOURCE_LAYERS_HPP__

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkImageSubresourceLayers
      class image_subresource_layers
      {
        public:
          /// \brief Create the image subresource layers.
          /// _layers parameters have the first component (x) as the base layer and the
          /// second (y) as the number of layers included in the subresource layers
          image_subresource_layers(VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT, uint32_t mip_level = 0,
                                  const glm::uvec2 &layer_range = glm::uvec2(0, 1))
            : vk_isl {aspect_mask, mip_level, layer_range.x, layer_range.y }
          {}

          image_subresource_layers(const image_subresource_layers &o) : vk_isl(o.vk_isl) {}
          image_subresource_layers &operator = (const image_subresource_layers &o) { vk_isl = o.vk_isl; return *this; }
          image_subresource_layers(const VkImageSubresourceLayers &o) : vk_isl(o) {}
          image_subresource_layers &operator = (const VkImageSubresourceLayers &o) { vk_isl = o; return *this; }

          /// \brief Return the aspect mask of the sub-resource (what kind of data is included in the subresource)
          VkImageAspectFlags get_aspect_mask() const { return vk_isl.aspectMask; }
          /// \brief Set the aspect mask of the sub-resource (what kind of data is included in the subresource)
          void set_aspect_mask(VkImageAspectFlags aspect_mask) { vk_isl.aspectMask = aspect_mask; }

          /// \brief Return the mipmap level
          uint32_t get_mipmap_level() const { return vk_isl.mipLevel; }
          /// \brief Set the mipmap level
          void set_mipmap_level(uint32_t level) { vk_isl.mipLevel = level; }

          /// \brief Return the layer range (X is the base layer, Y is the number of layers)
          glm::uvec2 get_layer_range() const { return glm::uvec2(vk_isl.baseArrayLayer, vk_isl.layerCount); }
          /// \brief Set the layer range (X is the base level, Y is the number of layers)
          void set_layer_range(const glm::uvec2 &range) { vk_isl.baseArrayLayer = range.x; vk_isl.layerCount = range.y; }

        public: // advanced
          /// \brief Yield a const reference to a VkImageSubresourceLayers
          operator const VkImageSubresourceLayers &() const { return vk_isl; }
        private:
          VkImageSubresourceLayers vk_isl;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_274878448709227246_3276410813_IMAGE_SUBRESOURCE_LAYERS_HPP__

