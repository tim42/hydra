//
// file : subpass.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/subpass.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 20 2016 16:40:15 GMT+0200 (CEST)
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

#ifndef __N_878112941936812685_369014797_SUBPASS_HPP__
#define __N_878112941936812685_369014797_SUBPASS_HPP__

#include <initializer_list>
#include <vector>
#include <cstring>

#include <vulkan/vulkan.h>

#include "../hydra_debug.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkSubpassDescription
      class subpass
      {
        public:
          enum class attachment_type
          {
            input,
            color
          };

        public:
          subpass(VkPipelineBindPoint pbp = VK_PIPELINE_BIND_POINT_GRAPHICS, bool _use_resolve = false)
            : use_resolve(_use_resolve)
          {
            memset(&vk_sd, 0, sizeof(vk_sd));
            vk_sd.pipelineBindPoint = pbp;
          }

          /// \brief Copy a subpass
          subpass(const subpass &o)
            : vk_sd(o.vk_sd), vk_input_attachment(o.vk_input_attachment),
              vk_color_attachment(o.vk_color_attachment),
              vk_resolve_attachment(o.vk_resolve_attachment),
              vk_depth_stencil_attachment(o.vk_depth_stencil_attachment),
              vk_preserve_attachment(o.vk_preserve_attachment),
              use_resolve(o.use_resolve)
          {
          }

          /// \brief Copy a subpass
          subpass &operator = (const subpass &o)
          {
            if (&o == this)
              return *this;
            vk_sd = o.vk_sd;
            vk_input_attachment = o.vk_input_attachment;
            vk_color_attachment = o.vk_color_attachment;
            vk_resolve_attachment = o.vk_resolve_attachment;
            vk_depth_stencil_attachment = o.vk_depth_stencil_attachment;
            vk_preserve_attachment = o.vk_preserve_attachment;
            use_resolve = o.use_resolve;
            return *this;
          }

          /// \brief Add an attachment (can be either an input attachment or a color attachment).
          /// If the sub pass has resolve enabled, an empty resolve entry is automatically added
          subpass &add_attachment(attachment_type type, VkImageLayout layout, size_t attachment_index)
          {
            switch (type)
            {
              case attachment_type::input:
                vk_input_attachment.push_back(VkAttachmentReference {(uint32_t)attachment_index, layout});
                vk_sd.pInputAttachments = vk_input_attachment.data();
                vk_sd.inputAttachmentCount = vk_input_attachment.size();
                break;
              case attachment_type::color:
                vk_color_attachment.push_back(VkAttachmentReference {(uint32_t)attachment_index, layout});
                vk_sd.pColorAttachments = vk_color_attachment.data();
                vk_sd.colorAttachmentCount = vk_color_attachment.size();
                if (use_resolve)
                {
                  vk_resolve_attachment.push_back(VkAttachmentReference {VK_ATTACHMENT_UNUSED, (VkImageLayout)0});
                  vk_sd.pResolveAttachments = vk_resolve_attachment.data();
                }
                break;
              default: break; // uh-oh
            }
            return *this;
          }

          /// \brief Add some attachments (can be either input attachments or color attachments).
          /// If the sub pass has resolve enabled, empty resolve entries are automatically added
          subpass &add_attachments(attachment_type type, VkImageLayout layout, std::initializer_list<size_t> attachments_indexes)
          {
            switch (type)
            {
              case attachment_type::input:
                for (size_t it : attachments_indexes)
                  vk_input_attachment.push_back(VkAttachmentReference {(uint32_t)it, layout});
                vk_sd.pInputAttachments = vk_input_attachment.data();
                vk_sd.inputAttachmentCount = vk_input_attachment.size();
                break;
              case attachment_type::color:
                for (size_t it : attachments_indexes)
                  vk_color_attachment.push_back(VkAttachmentReference {(uint32_t)it, layout});
                vk_sd.pColorAttachments = vk_color_attachment.data();
                vk_sd.colorAttachmentCount = vk_color_attachment.size();
                if (use_resolve)
                {
                  vk_resolve_attachment.resize(vk_resolve_attachment.size() + attachments_indexes.size(), VkAttachmentReference {VK_ATTACHMENT_UNUSED, (VkImageLayout)0});
                  vk_sd.pResolveAttachments = vk_resolve_attachment.data();
                }
                break;
              default: break; // uh-oh
            }
            return *this;
          }

          /// \brief Add a color attachment but also specify a resolve attachment
          subpass &add_attachment(VkImageLayout layout, size_t attachment_index, VkImageLayout resolve_layout, size_t resolve_attachment_index)
          {
            check::on_vulkan_error::n_assert(this->use_resolve == true, "using add_attachment with resolve attachment in a subpass that have resolve disabled");

            vk_color_attachment.push_back(VkAttachmentReference {(uint32_t)attachment_index, layout});
            vk_sd.pColorAttachments = vk_color_attachment.data();
            vk_sd.colorAttachmentCount = vk_color_attachment.size();

            vk_resolve_attachment.push_back(VkAttachmentReference {(uint32_t)resolve_attachment_index, resolve_layout});
            vk_sd.pResolveAttachments = vk_resolve_attachment.data();

            return *this;
          }

          /// \brief Add some color attachments but also specify a resolve attachment
          subpass &add_attachments(VkImageLayout layout, std::initializer_list<size_t> attachments_indexes,
                                   VkImageLayout resolve_layout, std::initializer_list<size_t> resolve_attachments_indexes)
          {
            check::on_vulkan_error::n_assert(this->use_resolve == true, "using add_attachments with some resolve attachments in a subpass that have resolve disabled");

            for (size_t it : attachments_indexes)
              vk_color_attachment.push_back(VkAttachmentReference {(uint32_t)it, layout});
            vk_sd.pColorAttachments = vk_color_attachment.data();
            vk_sd.colorAttachmentCount = vk_color_attachment.size();

            for (size_t it : resolve_attachments_indexes)
              vk_resolve_attachment.push_back(VkAttachmentReference {(uint32_t)it, resolve_layout});
            vk_sd.pResolveAttachments = vk_resolve_attachment.data();

            return *this;
          }

          /// \brief Add an attachment to be preserved by this pass
          subpass &add_attachment_to_preserve(size_t attachment_index)
          {
            vk_preserve_attachment.push_back(attachment_index);
            vk_sd.pPreserveAttachments = vk_preserve_attachment.data();
            vk_sd.preserveAttachmentCount = vk_preserve_attachment.size();
            return *this;
          }

          /// \brief Add some attachments to be preserved by this pass
          subpass &add_attachments_to_preserve(std::initializer_list<size_t> attachments_indexes)
          {
            vk_preserve_attachment.insert(vk_preserve_attachment.end(), attachments_indexes.begin(), attachments_indexes.end());
            vk_sd.pPreserveAttachments = vk_preserve_attachment.data();
            vk_sd.preserveAttachmentCount = vk_preserve_attachment.size();
            return *this;
          }

          /// \brief Clear the subpass
          void clear()
          {
            VkPipelineBindPoint pbp = vk_sd.pipelineBindPoint;
            memset(&vk_sd, 0, sizeof(vk_sd));
            vk_sd.pipelineBindPoint = pbp;

            vk_input_attachment.clear();
            vk_color_attachment.clear();
            vk_resolve_attachment.clear();
            vk_depth_stencil_attachment.clear();
            vk_preserve_attachment.clear();
          }

          /// \brief Clear, but also change the use_resolve flag
          void clear(bool _use_resolve)
          {
            clear();
            use_resolve = _use_resolve;
          }

        public:
          /// \brief Yield a const reference to the wrapped VkSubpassDescription
          operator const VkSubpassDescription &() const { return vk_sd; }

        private:
          VkSubpassDescription vk_sd;

          std::vector<VkAttachmentReference> vk_input_attachment;
          std::vector<VkAttachmentReference> vk_color_attachment;
          std::vector<VkAttachmentReference> vk_resolve_attachment;
          std::vector<VkAttachmentReference> vk_depth_stencil_attachment;
          std::vector<uint32_t> vk_preserve_attachment;
          bool use_resolve;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_878112941936812685_369014797_SUBPASS_HPP__

