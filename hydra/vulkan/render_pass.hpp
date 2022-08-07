//
// file : render_pass.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/render_pass.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 20 2016 16:54:30 GMT+0200 (CEST)
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

#ifndef __N_155238131197813153_3057226920_RENDER_PASS_HPP__
#define __N_155238131197813153_3057226920_RENDER_PASS_HPP__

#include <deque>
#include <vulkan/vulkan.h>

#include "../hydra_debug.hpp"

#include "device.hpp"
#include "subpass.hpp"
#include "subpass_dependency.hpp"
#include "attachment.hpp"

#include <ntools/id/id.hpp>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief A render pass
      /// As opposed to some other vulkan-wrapping classes, it has a delayed creation
      /// and allows recreation if there's changes.
      class render_pass
      {
        public:
          /// \brief Create a render-pass with the explicit goal of being destroyed
          render_pass(device &_dev, VkRenderPass pass) : dev(_dev), vk_render_pass(pass) {}

        public:
          /// \brief Create a renderpass with subpass_count subpasses and attachment_count attachments
          render_pass(device &_dev, size_t subpass_count = 0, size_t attachment_count = 0)
            : dev(_dev)
          {
            // only set once
            create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            create_info.pNext = nullptr;
            create_info.flags = 0;

            if (subpass_count > 0)
              subpasses.resize(subpass_count);
            if (attachment_count > 0)
              attachments.resize(attachment_count, attachment(VkAttachmentDescription {}));
          }

          /// \brief Create a renderpass with subpass_count subpasses and attachment_count attachments (preinitialized with a swapchain and a pipeline_multisample_state)
          render_pass(device &_dev, size_t subpass_count, size_t attachment_count, const swapchain &sw, const pipeline_multisample_state &pms)
            : dev(_dev)
          {
            // only set once
            create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            create_info.pNext = nullptr;
            create_info.flags = 0;

            if (subpass_count > 0)
              subpasses.resize(subpass_count);
            if (attachment_count > 0)
              attachments.resize(attachment_count, attachment(sw, pms, VkAttachmentDescription {}));
          }

          /// \brief Move constr.
          render_pass(render_pass &&o)
            : dev(o.dev), vk_render_pass(o.vk_render_pass), create_info(o.create_info),
              vk_subpasses(std::move(o.vk_subpasses)), subpasses(std::move(o.subpasses)),
              vk_attachments(std::move(o.vk_attachments)), attachments(std::move(o.attachments)),
              vk_subpass_dependencies(std::move(o.vk_subpass_dependencies)), subpass_dependencies(std::move(o.subpass_dependencies))
          {
            o.vk_render_pass = nullptr;
          }

          ~render_pass()
          {
            if (vk_render_pass)
              dev._vkDestroyRenderPass(vk_render_pass, nullptr);
          }


          // // SUBPASSES // //

          /// \brief Add a new subpass (the returned reference will always be valid)
          subpass &create_subpass(VkPipelineBindPoint pbp = VK_PIPELINE_BIND_POINT_GRAPHICS, bool use_resolve = false)
          {
            subpasses.emplace_back(pbp, use_resolve);
            return subpasses.back();
          }

          /// \brief Remove all subpasses
          void clear_subpasses() { subpasses.clear(); }

          /// \brief Get a subpass by its index (the returned reference will always be valid)
          subpass &get_subpass(size_t index) { return subpasses.at(index); }
          /// \brief Get a subpass by its index (the returned reference will always be valid)
          const subpass &get_subpass(size_t index) const { return subpasses.at(index); }

          /// \brief Return the number of subpasses
          size_t get_subpass_count() const { return subpasses.size(); }


          /// \brief Create a new subpass dependency
          /// \note either src_pass or dst_pass can be VK_SUBPASS_EXTERNAL to indicate external dependency
          subpass_dependency &create_subpass_dependency(size_t src_pass, size_t dst_pass, VkDependencyFlags dependency_flags = 0)
          {
            subpass_dependencies.emplace_back(src_pass, dst_pass, dependency_flags);
            return subpass_dependencies.back();
          }

          /// \brief Return an existing subpass dependency for src_pass and dst_pass
          /// If that dependency does not exist yet, it returns nullptr
          /// \note time complexity: O(n)
          subpass_dependency *get_subpass_dependency(size_t src_pass, size_t dst_pass)
          {
            for (subpass_dependency &it : subpass_dependencies)
            {
              const VkSubpassDependency &vsd = it;
              if (vsd.srcSubpass == src_pass && vsd.dstSubpass == dst_pass)
                return &it;
            }
            return nullptr;
          }

          /// \brief Return an existing subpass dependency for src_pass and dst_pass
          /// If that dependency does not exist yet, it returns nullptr
          /// \note time complexity: O(n)
          const subpass_dependency *get_subpass_dependency(size_t src_pass, size_t dst_pass) const
          {
            for (const subpass_dependency &it : subpass_dependencies)
            {
              const VkSubpassDependency &vsd = it;
              if (vsd.srcSubpass == src_pass && vsd.dstSubpass == dst_pass)
                return &it;
            }
            return nullptr;
          }

          /// \brief Checks if a subpass dependency exists
          /// \note time complexity: O(n)
          bool subpass_dependency_exists(size_t src_pass, size_t dst_pass) const
          {
            for (const subpass_dependency &it : subpass_dependencies)
            {
              const VkSubpassDependency &vsd = it;
              if (vsd.srcSubpass == src_pass && vsd.dstSubpass == dst_pass)
                return true;
            }
            return false;
          }


          // // ATTACHMENTS // //

          /// \brief Add a new attachment to the renderpass
          attachment &create_attachment() { attachments.emplace_back(VkAttachmentDescription {}); return attachments.back(); }

          /// \brief Add a new attachment to the renderpass (the attachment is preinitialized with a swapchain and a pipeline_multisample_state)
          attachment &create_attachment(const swapchain &sw, const pipeline_multisample_state &pms)
          {
            attachments.emplace_back(sw, pms, VkAttachmentDescription {});
            return attachments.back();
          }

          /// \brief Remove all attachments
          void clear_attachments() { attachments.clear(); }

          /// \brief Return an attachment by its index (the returned reference will always be valid)
          attachment &get_attachment(size_t index) { return attachments.at(index); }
          /// \brief Return an attachment by its index (the returned reference will always be valid)
          const attachment &get_attachment(size_t index) const { return attachments.at(index); }

          /// \brief Return the number of attachments
          size_t get_attachment_count() const { return attachments.size(); }


          // // REFRESH // //

          /// \brief Re/create the render_pass
          /// \note return the old render_pass that should be destroyed
          vk::render_pass refresh()
          {
            // update the vk_ vectors
            vk_attachments.clear();
            vk_attachments.reserve(attachments.size());
            vk_subpasses.clear();
            vk_subpasses.reserve(subpasses.size());
            vk_subpass_dependencies.clear();
            vk_subpass_dependencies.reserve(subpass_dependencies.size());

            for (attachment &it : attachments)
            {
              it.refresh(); // update the attachment (only usefull if a swapchain or a pipeline_multisample_state has been linked)
              vk_attachments.push_back(it);
            }
            for (const subpass &it : subpasses)
              vk_subpasses.push_back(it);
            for (const subpass_dependency &it : subpass_dependencies)
              vk_subpass_dependencies.push_back(it);

            // update the create_info object
            create_info.attachmentCount = vk_attachments.size();
            create_info.pAttachments = vk_attachments.data();
            create_info.subpassCount = vk_subpasses.size();
            create_info.pSubpasses = vk_subpasses.data();
            create_info.dependencyCount = vk_subpass_dependencies.size();
            create_info.pDependencies = vk_subpass_dependencies.data();

            // create the vulkan render pass object
            VkRenderPass old_vk_render_pass = vk_render_pass;
//             if (vk_render_pass)
//               dev._vkDestroyRenderPass(vk_render_pass, nullptr);
            check::on_vulkan_error::n_assert_success(dev._vkCreateRenderPass(&create_info, nullptr, &vk_render_pass));
            return { dev, old_vk_render_pass };
          }

        public: // advanced
          /// \brief Return the underlying render pass
          VkRenderPass get_vk_render_pass() const { return vk_render_pass; }

          id_t compute_subpass_hash(uint32_t subpass)
          {
            if (subpass >= subpasses.size())
              return id_t::none;

            const auto& sp = subpasses[subpass];
            id_t hash = id_t::none;
            for (const auto& ref : sp.vk_color_attachment)
            {
              if (ref.attachment >= attachments.size())
                continue;
              id_t subhash = attachments[ref.attachment].compute_hash();
              subhash = (id_t)ct::hash::fnv1a_continue<64>((uint64_t)subhash, (const uint8_t*)&ref.layout, sizeof(ref.layout));
              hash = combine(hash, subhash);
            }
            for (const auto& ref : sp.vk_depth_stencil_attachment)
            {
              if (ref.attachment >= attachments.size())
                continue;
              id_t subhash = attachments[ref.attachment].compute_hash();
              subhash = (id_t)ct::hash::fnv1a_continue<64>((uint64_t)subhash, (const uint8_t*)&ref.layout, sizeof(ref.layout));
              hash = combine(hash, subhash);
            }
            return hash;
          }

        private:
          device &dev;
          VkRenderPass vk_render_pass = nullptr;
          VkRenderPassCreateInfo create_info;

          std::vector<VkSubpassDescription> vk_subpasses;
          std::vector<subpass> subpasses;
          std::vector<VkAttachmentDescription> vk_attachments;
          std::vector<attachment> attachments;
          std::vector<VkSubpassDependency> vk_subpass_dependencies;
          std::vector<subpass_dependency> subpass_dependencies;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_155238131197813153_3057226920_RENDER_PASS_HPP__

