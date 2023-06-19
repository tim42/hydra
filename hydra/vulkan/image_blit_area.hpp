//
// file : image_blit_area.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/image_blit_area.hpp
//
// created by : Timothée Feuillet
// date: Thu Aug 25 2016 15:17:25 GMT+0200 (CEST)
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
#include <glm/glm.hpp>

#include "image_subresource_layers.hpp"
#include "rect2D.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkImageBlit
      class image_blit_area
      {
        public:
          /// \brief Construct a blit area for a 3D image
          image_blit_area(const glm::ivec3 &src_st_offset, const glm::ivec3 &src_end_offset,
                          const glm::ivec3 &dst_st_offset, const glm::ivec3 &dst_end_offset,
                          const image_subresource_layers &src_subres = image_subresource_layers(),
                          const image_subresource_layers &dst_subres = image_subresource_layers())
            : vk_ib
          {
            src_subres,
            { {src_st_offset.x, src_st_offset.y, src_st_offset.z}, {src_end_offset.x, src_end_offset.y, src_end_offset.z} },
            dst_subres,
            { {dst_st_offset.x, dst_st_offset.y, dst_st_offset.z}, {dst_end_offset.x, dst_end_offset.y, dst_end_offset.z} },
          }
          {}

          /// \brief Construct a blit area for a 2D image using glm::ivec2
          image_blit_area(const glm::ivec2 &src_st_offset, const glm::ivec2 &src_end_offset,
                          const glm::ivec2 &dst_st_offset, const glm::ivec2 &dst_end_offset,
                          const image_subresource_layers &src_subres = image_subresource_layers(),
                          const image_subresource_layers &dst_subres = image_subresource_layers())
            : vk_ib
          {
            src_subres,
            { {src_st_offset.x, src_st_offset.y, 0}, {src_end_offset.x, src_end_offset.y, 1} },
            dst_subres,
            { {dst_st_offset.x, dst_st_offset.y, 0}, {dst_end_offset.x, dst_end_offset.y, 1} },
          }
          {}

          /// \brief Construct a blit area for a 2D image using rect2D
          image_blit_area(const rect2D &src_rect, const rect2D &dst_rect,
                          const image_subresource_layers &src_subres = image_subresource_layers(),
                          const image_subresource_layers &dst_subres = image_subresource_layers())
            : vk_ib
          {
            src_subres,
            { {src_rect.get_offset().x, src_rect.get_offset().y, 0}, {src_rect.get_end_offset().x, src_rect.get_end_offset().y, 0} },
            dst_subres,
            { {dst_rect.get_offset().x, dst_rect.get_offset().y, 0}, {dst_rect.get_end_offset().x, dst_rect.get_end_offset().y, 0} },
          }
          {}

          /// \brief Construct a blit area for a 1D image
          image_blit_area(int32_t src_st_offset, int32_t src_end_offset,
                          int32_t dst_st_offset, int32_t dst_end_offset,
                          const image_subresource_layers &src_subres = image_subresource_layers(),
                          const image_subresource_layers &dst_subres = image_subresource_layers())
            : vk_ib
          {
            src_subres,
            { {src_st_offset, 0, 0}, {src_end_offset, 0, 0} },
            dst_subres,
            { {dst_st_offset, 0, 0}, {dst_end_offset, 0, 0} },
          }
          {}

          image_blit_area(const image_blit_area &o) : vk_ib(o.vk_ib) {}
          image_blit_area(const VkImageBlit &o) : vk_ib(o) {}
          image_blit_area &operator = (const image_blit_area &o) { vk_ib = o.vk_ib; return *this; }
          image_blit_area &operator = (const VkImageBlit &o) { vk_ib = o; return *this; }

          // TODO getters & setters

        public: // advanced
          /// \brief Yield a const reference to a VkImageBlit
          operator const VkImageBlit &() const { return vk_ib; }
        private:
          VkImageBlit vk_ib;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



