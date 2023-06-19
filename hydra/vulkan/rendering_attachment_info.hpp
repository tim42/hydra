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

#include "clear_value.hpp"
#include "image_view.hpp"

namespace neam::hydra::vk
{
  class rendering_attachment_info
  {
    public:
      rendering_attachment_info()
        : info
      {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,

        .imageView = nullptr,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,

        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = nullptr,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,

        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

        .clearValue = clear_value{},
      }
      , view_format(VK_FORMAT_UNDEFINED)
      {
      }

      rendering_attachment_info(const image_view& view, VkImageLayout layout, VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op, clear_value cv = {})
        : info
      {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,

        .imageView = view.get_vk_image_view(),
        .imageLayout = layout,

        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = nullptr,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,

        .loadOp = load_op,
        .storeOp = store_op,

        .clearValue = cv,
      }
      , view_format(view.get_view_format())
      {
      }

      // FIXME: resolve constructor

      rendering_attachment_info(const rendering_attachment_info&) = default;
      rendering_attachment_info& operator = (const rendering_attachment_info&) = default;

    public:
      const VkRenderingAttachmentInfo& _get_vk_info() const { return info; }
      VkFormat _get_view_format() const { return view_format; }

    private:
      VkRenderingAttachmentInfo info;
      VkFormat view_format;
  };
}

