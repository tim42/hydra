//
// file : image_2d.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/image_creators/image_2d.hpp
//
// created by : Timothée Feuillet
// date: Sun May 01 2016 11:55:44 GMT+0200 (CEST)
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
      /// \brief Image creator for 2D images
      class image_2d
      {
        public:
          image_2d(const glm::uvec2 &_size, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, uint32_t _mip_levels = 1, VkImageLayout _initial_layout = VK_IMAGE_LAYOUT_PREINITIALIZED)
           : size(_size), format(_format), tiling(_tiling), usage(_usage), mip_levels(_mip_levels), initial_layout(_initial_layout) {}
          image_2d(const image_2d &o) = default;

          void update_image_create_info(VkImageCreateInfo &create_info) const
          {
            create_info.imageType = VK_IMAGE_TYPE_2D;
            create_info.extent = {size.x, size.y, 1};
            create_info.format = format;
            create_info.tiling = tiling;
            create_info.usage = usage;
            create_info.arrayLayers = 1;
            create_info.mipLevels = mip_levels;
            create_info.initialLayout = initial_layout;
            create_info.samples = VK_SAMPLE_COUNT_1_BIT;
          }

        private:
          glm::uvec2 size;
          VkFormat format;
          VkImageTiling tiling;
          VkImageUsageFlags usage;
          uint32_t mip_levels;
          VkImageLayout initial_layout;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



