//
// file : pipeline_color_blending.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/pipeline_color_blending.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 13 2016 16:16:43 GMT+0200 (CEST)
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


#include <vector>
#include <deque>
#include <initializer_list>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "attachment_color_blending.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkPipelineColorBlendStateCreateInfo
      class pipeline_color_blending
      {
        public:
          pipeline_color_blending()
            : vk_pcbci
          {
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0,
            VK_FALSE, /* logicOpEnable */
            VK_LOGIC_OP_COPY,
            0, nullptr,
            {0, 0, 0, 0}
          }
          {}

          /// \brief Default constructor disables logic OP color blending
          pipeline_color_blending(std::initializer_list<const attachment_color_blending> acb_list, const glm::vec4 &blend_constants = glm::vec4(0, 0, 0, 0))
            : vk_pcbci
          {
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0,
            VK_FALSE, /* logicOpEnable */
            VK_LOGIC_OP_COPY,
            0, nullptr,
            {blend_constants.r, blend_constants.g, blend_constants.b, blend_constants.a}
          }
          {
            add_attachment_color_blending(acb_list);
          }

          /// \brief constructor that enables logic op color blending
          pipeline_color_blending(std::initializer_list<const attachment_color_blending> acb_list, VkLogicOp op, const glm::vec4 &blend_constants = glm::vec4(0, 0, 0, 0))
            : vk_pcbci
          {
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0,
            VK_TRUE, /* logicOpEnable */
            op,
            0, nullptr,
            {blend_constants.r, blend_constants.g, blend_constants.b, blend_constants.a}
          }
          {
            add_attachment_color_blending(acb_list);
          }

          /// \brief Copy constructor
          pipeline_color_blending(const pipeline_color_blending &o) : vk_pcbci(o.vk_pcbci), vk_attachments(o.vk_attachments) {}
          /// \brief Move constructor
          pipeline_color_blending(pipeline_color_blending &&o) : vk_pcbci(o.vk_pcbci), vk_attachments(std::move(o.vk_attachments))  {}
          /// \brief Copy operator
          pipeline_color_blending &operator = (const pipeline_color_blending &o)
          {
            if (this == &o)
              return *this;
            vk_pcbci = o.vk_pcbci;
            vk_attachments = o.vk_attachments;
            return *this;

          }
          /// \brief Move operator
          pipeline_color_blending &operator = (pipeline_color_blending &&o)
          {
            if (this == &o)
              return *this;
            vk_pcbci = o.vk_pcbci;
            vk_attachments = std::move(o.vk_attachments);
            return *this;
          }

          /// \brief Add an attachment_color_blending
          void add_attachment_color_blending(attachment_color_blending acb)
          {
            vk_attachments.push_back(acb);
            refresh();
          }

          /// \brief Add multiple attachment_color_blending
          void add_attachment_color_blending(std::initializer_list<const attachment_color_blending> acb_list)
          {
            vk_attachments.insert(vk_attachments.end(), acb_list.begin(), acb_list.end());
            refresh();
          }

          /// \brief Refresh attachments (if they have changed)
          void refresh()
          {
            vk_pcbci.attachmentCount = vk_attachments.size();
            vk_pcbci.pAttachments = vk_attachments.data();
          }

          /// \brief Clear the attachments color blending
          void clear()
          {
            vk_attachments.clear();
            refresh();
          }

          /// \brief Return the number of attachments
          size_t get_attachment_count() const
          {
            return vk_attachments.size();
          }

          /// \brief enable or disable logic op blending
          void enable_bitwise_blending(bool enable = true)
          {
            vk_pcbci.logicOpEnable = enable ? VK_TRUE : VK_FALSE;
          }

          /// \brief Set the operation of the bitwise blending
          void set_bitwise_blending_operation(VkLogicOp op)
          {
            vk_pcbci.logicOp = op;
          }

          /// \brief Return true if bitwise blending is enabled
          bool is_bitwise_blending_enabled() const
          {
            return vk_pcbci.logicOpEnable == VK_TRUE;
          }

          /// \brief Set the blending constants
          void set_blending_constants(const glm::vec4 &bconsts)
          {
            vk_pcbci.blendConstants[0] = bconsts.r;
            vk_pcbci.blendConstants[1] = bconsts.g;
            vk_pcbci.blendConstants[2] = bconsts.b;
            vk_pcbci.blendConstants[3] = bconsts.a;
          }

        public:
          /// \brief Yield a const reference to a VkPipelineColorBlendStateCreateInfo
          operator const VkPipelineColorBlendStateCreateInfo &() const { return vk_pcbci; }

        private:
          VkPipelineColorBlendStateCreateInfo vk_pcbci;
          std::vector<VkPipelineColorBlendAttachmentState> vk_attachments;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



