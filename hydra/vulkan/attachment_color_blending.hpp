//
// file : attachment_color_blending.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/attachment_color_blending.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 13 2016 14:56:13 GMT+0200 (CEST)
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

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkPipelineColorBlendAttachmentState
      class attachment_color_blending
      {
        public:
          /// \brief Default constructor: disable color blending
          attachment_color_blending()
          {
            vk_pcbas.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            vk_pcbas.blendEnable = VK_FALSE;
            vk_pcbas.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            vk_pcbas.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            vk_pcbas.colorBlendOp = VK_BLEND_OP_ADD;
            vk_pcbas.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            vk_pcbas.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            vk_pcbas.alphaBlendOp = VK_BLEND_OP_ADD;
          }

          /// \brief Copy constructor
          attachment_color_blending(const attachment_color_blending &o) : vk_pcbas(o.vk_pcbas) {}
          /// \brief Affectation
          attachment_color_blending &operator = (const attachment_color_blending &o) { vk_pcbas = o.vk_pcbas; return *this; }
          /// \brief Copy constructor
          attachment_color_blending(const VkPipelineColorBlendAttachmentState &o) : vk_pcbas(o) {}
          /// \brief Affectation
          attachment_color_blending &operator = (const VkPipelineColorBlendAttachmentState &o) { vk_pcbas = o; return *this; }

          /// \brief Create a attachment_color_blending that do alpha blending
          static attachment_color_blending create_alpha_blending()
          {
            attachment_color_blending ret;
            ret.set_alpha_blending();
            return ret;
          }
          /// \brief Create a attachment_color_blending from the equation
          static attachment_color_blending create_blending_from_equation(VkBlendFactor src_color, VkBlendOp color_op, VkBlendFactor dst_color,
                                                                       VkBlendFactor src_alpha, VkBlendOp alpha_op, VkBlendFactor dst_alpha)
          {
            attachment_color_blending ret;
            ret.set_equation(src_color, color_op, dst_color, src_alpha, alpha_op, dst_alpha);
            return ret;
          }

          /// \brief Set the attachment_color_blending to alpha blending
          void set_alpha_blending()
          {
            *this = VkPipelineColorBlendAttachmentState
            {
              VK_TRUE, // blendEnable
              VK_BLEND_FACTOR_SRC_ALPHA,            // src (color)
              VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,  // dst (color)
              VK_BLEND_OP_ADD,                      // op  (color)

              VK_BLEND_FACTOR_ONE,                  // src (alpha)
              VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,  // dst (alpha)
              VK_BLEND_OP_ADD,                      // op  (alpha)

              // color write mask
              vk_pcbas.colorWriteMask
            };
          }

          /// \brief Disable color blending
          void disable_blending() { vk_pcbas.blendEnable = VK_FALSE; }
          /// \brief Enable color blending
          /// \warning DO NOT DO THIS ON A DEFAULT INITIALIZED attachment_color_blending object
          void enable_blending(bool enable = true) { vk_pcbas.blendEnable = enable ? VK_TRUE : VK_FALSE; }

          /// \brief Set the blending equation
          void set_equation(VkBlendFactor src_color, VkBlendOp color_op, VkBlendFactor dst_color, VkBlendFactor src_alpha, VkBlendOp alpha_op, VkBlendFactor dst_alpha)
          {
            *this = VkPipelineColorBlendAttachmentState
            {
              VK_TRUE,    // blendEnable
              src_color,  // src (color)
              dst_color,  // dst (color)
              color_op,   // op  (color)

              src_alpha,  // src (alpha)
              dst_alpha,  // dst (alpha)
              alpha_op,   // op  (alpha)

              // color write mask
              vk_pcbas.colorWriteMask
            };
          }

          /// \brief Set the color write mask
          void set_color_write_mask(VkColorComponentFlags mask) { vk_pcbas.colorWriteMask = mask; }

        public: // advanced
          /// \brief Yields a VkPipelineColorBlendAttachmentState
          operator const VkPipelineColorBlendAttachmentState &() const { return vk_pcbas; }
          VkPipelineColorBlendAttachmentState& _get_vk_blend_state() { return vk_pcbas; }
        private:
          VkPipelineColorBlendAttachmentState vk_pcbas;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



