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

#include "rendering_info.hpp"
#include "rendering_attachment_info.hpp"

#include <ntools/id/id.hpp>

namespace neam::hydra::vk
{
  class pipeline_rendering_create_info
  {
    public:
      pipeline_rendering_create_info()
        : info
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 0,
        .pColorAttachmentFormats = nullptr,
        .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
      }
      {
      }

      pipeline_rendering_create_info(std::vector<VkFormat> formats, VkFormat depthFormat = VK_FORMAT_UNDEFINED, VkFormat stencilFormat = VK_FORMAT_UNDEFINED)
        : vk_formats(std::move(formats))
        , info
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = (uint32_t)vk_formats.size(),
        .pColorAttachmentFormats = vk_formats.data(),
        .depthAttachmentFormat = depthFormat,
        .stencilAttachmentFormat = stencilFormat,
      }
      {
      }

      /*implicit*/ pipeline_rendering_create_info(const rendering_info& rai)
        : info
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
      }
      {
        vk_formats.reserve(rai._get_view_count());
        for (uint32_t i = 0; i < rai._get_view_count(); ++i)
          vk_formats.push_back(rai._get_view_format(i));

        info.colorAttachmentCount = (uint32_t)vk_formats.size();
        info.pColorAttachmentFormats = vk_formats.data();

        info.depthAttachmentFormat = rai._get_depth_view_format();
        info.stencilAttachmentFormat = rai._get_stencil_view_format();
      }

      pipeline_rendering_create_info(const pipeline_rendering_create_info& o)
        : vk_formats(o.vk_formats)
        , info(o.info)
      {
        info.pColorAttachmentFormats = vk_formats.data();
      }

      pipeline_rendering_create_info& operator = (const pipeline_rendering_create_info& o)
      {
        if (&o == this) return *this;
        vk_formats = o.vk_formats;
        info = o.info;

        info.pColorAttachmentFormats = vk_formats.data();

        return *this;
      }

      id_t compute_hash() const
      {
        uint64_t ret = (vk_formats.size() << 32);
        for (VkFormat f : vk_formats)
          ret *= (uint64_t)f + 1;
        ret = (ret << 32) | (ret >> 32);
        ret *= (uint64_t)info.depthAttachmentFormat + 1;
        ret *= (uint64_t)info.stencilAttachmentFormat + 1;
        return (id_t)ret;
      }

    public:
      const VkPipelineRenderingCreateInfo& _get_vk_info() const { return info; }

    private:
      std::vector<VkFormat> vk_formats = {};
      VkPipelineRenderingCreateInfo info;
  };
}

