//
// created by : Timothée Feuillet
// date: 2023-5-4
//
//
// Copyright (c) 2023 Timothée Feuillet
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
#include <vector>

#include "rendering_attachment_info.hpp"
#include "rect2D.hpp"
#include "../hydra_debug.hpp"

namespace neam::hydra::vk
{
  class rendering_info
  {
    public:
      rendering_info()
        : info
      {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = vk::rect2D{glm::uvec2(0, 0), glm::uvec2(0, 0)},
        .layerCount = 1,
        .viewMask = 0,

        .colorAttachmentCount = 0,
        .pColorAttachments = nullptr,

        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr,
      }
      {
      }

      rendering_info(const vk::rect2D& rect, std::vector<rendering_attachment_info> color_attachments,
                               const rendering_attachment_info& depth = {},
                               const rendering_attachment_info& stencil = {})
        : rendering_info(0, rect, std::move(color_attachments), depth, stencil)
      {
      }

      rendering_info(VkRenderingFlags flags, const vk::rect2D& rect, std::vector<rendering_attachment_info> color_attachments,
                               const rendering_attachment_info& depth = {},
                               const rendering_attachment_info& stencil = {})
      : color_info(std::move(color_attachments))
      , depth_info(depth)
      , stencil_info(stencil)
      , info
      {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = flags,
        .renderArea = rect,
        .layerCount = 1,
        .viewMask = 0,

        .colorAttachmentCount = 0,
        .pColorAttachments = nullptr,

        .pDepthAttachment = &depth._get_vk_info(),
        .pStencilAttachment = &stencil._get_vk_info(),
      }
      {
        update();
      }

      rendering_info(const rendering_info& o)
      {
        color_info = o.color_info;
        depth_info = o.depth_info;
        stencil_info = o.stencil_info;
        info = o.info;
        update();
      }

      rendering_info& operator = (const rendering_info& o)
      {
        if (&o == this) return *this;
        color_info = o.color_info;
        depth_info = o.depth_info;
        stencil_info = o.stencil_info;
        info = o.info;

        update();
        return *this;
      }

    public:
      const VkRenderingInfo& _get_vk_info() const { return info; }
      uint32_t _get_view_count() const { return (uint32_t)color_info.size(); }
      VkFormat _get_view_format(uint32_t view_index) const
      {
        check::debug::n_assert(view_index < _get_view_count(), "out of bound access");
        return color_info[view_index]._get_view_format();
      }

      VkFormat _get_depth_view_format() const { return depth_info._get_view_format(); }
      VkFormat _get_stencil_view_format() const { return stencil_info._get_view_format(); }

    private:
      void update()
      {
        vk_color_info.clear();
        vk_color_info.reserve(color_info.size());
        for (const auto& it : color_info)
          vk_color_info.push_back(it._get_vk_info());

        info.colorAttachmentCount = (uint32_t)vk_color_info.size();
        info.pColorAttachments = vk_color_info.data();

        info.pDepthAttachment = &depth_info._get_vk_info();
        info.pStencilAttachment = &stencil_info._get_vk_info();
      }

    private:
      std::vector<rendering_attachment_info> color_info;
      rendering_attachment_info depth_info;
      rendering_attachment_info stencil_info;

      std::vector<VkRenderingAttachmentInfo> vk_color_info;
      VkRenderingInfo info;
  };
}

