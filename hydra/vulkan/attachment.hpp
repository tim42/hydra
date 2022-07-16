//
// file : attachment.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/attachment.hpp
//
// created by : Timothée Feuillet
// date: Sun Aug 14 2016 14:45:55 GMT+0200 (CEST)
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

#ifndef __N_518920191126418147_250034947_ATTACHMENT_HPP__
#define __N_518920191126418147_250034947_ATTACHMENT_HPP__

#include <vulkan/vulkan.h>

#include "swapchain.hpp"
#include "pipeline_multisample_state.hpp"

#include <ntools/hash/fnv1a.hpp>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkAttachmentDescription
      /// \note you can, optionally, use a pointer to swapchain / pipeline_multisample_state
      ///       to fill for you some information (image format & sample count).
      ///       That way if you change those things, a simple refresh() call on the attachment will
      ///       automatically update the attachment. This is totally optional.
      /// \todo A better interface for that class
      class attachment
      {
        public:
          /// \brief Build the attachment description from a swapchain, a pipeline_multisample_state
          /// and some parameters
          /// \note You do not need to fill format and samples, it will be done automatically (and updated with a call to refresh())
          attachment(const swapchain &sw, const pipeline_multisample_state &pms, const VkAttachmentDescription &desc)
           : vk_attachment_desc(desc), swapchain_ptr(&sw), multisample_ptr(&pms)
          {
            refresh();
          }

          /// \brief Build the attachment from the vulkan structure
          attachment(const VkAttachmentDescription &desc)
            : vk_attachment_desc(desc), swapchain_ptr(nullptr), multisample_ptr(nullptr)
          {
          }

          /// \brief Copy constr.
          attachment(const attachment &o)
            : vk_attachment_desc(o.vk_attachment_desc), hash(o.hash), swapchain_ptr(o.swapchain_ptr), multisample_ptr(o.multisample_ptr)
          {
          }

          /// \brief Build the attachment from the vulkan structure
          attachment &operator = (const VkAttachmentDescription &desc)
          {
            vk_attachment_desc = (desc);
            swapchain_ptr = (nullptr);
            multisample_ptr = (nullptr);
            reset_hash();
            return *this;
          }

          /// \brief Copy constr.
          attachment &operator = (const attachment &o)
          {
            vk_attachment_desc = (o.vk_attachment_desc);
            hash = o.hash;
            swapchain_ptr = (o.swapchain_ptr);
            multisample_ptr = (o.multisample_ptr);
            return *this;
          }

          /// \brief Refresh format & samples (if a pipeline_multisample_state &a swapchain has been provided)
          void refresh()
          {
            reset_hash();
            if (swapchain_ptr)
              vk_attachment_desc.format = swapchain_ptr->get_image_format();
            if (multisample_ptr)
              vk_attachment_desc.samples = multisample_ptr->get_sample_count();

          }

          /// \brief Set the swapchain (set nullptr to disable the swapchain)
          attachment &set_swapchain(const swapchain *sw) { swapchain_ptr = sw; refresh(); return *this; }

          /// \brief Set the multisample structure (set nullptr to disable the pipeline_multisample_state)
          attachment &set_multisample_state(const pipeline_multisample_state *pms) { multisample_ptr = pms; refresh(); return *this; }

          /// \brief Set the color/depth & stencil load operation (if you don't care about stencil, simply leave the second argument as-is)
          attachment &set_load_op(VkAttachmentLoadOp color_depth_load_op, VkAttachmentLoadOp stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE)
          {
            reset_hash();
            vk_attachment_desc.loadOp = color_depth_load_op;
            vk_attachment_desc.stencilLoadOp = stencil_load_op;
            return *this;
          }

          /// \brief Set the color/depth & stencil store operation (if you don't care about stencil, simply leave the second argument as-is)
          attachment &set_store_op(VkAttachmentStoreOp color_depth_store_op = VK_ATTACHMENT_STORE_OP_STORE, VkAttachmentStoreOp stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE)
          {
            reset_hash();
            vk_attachment_desc.storeOp = color_depth_store_op;
            vk_attachment_desc.stencilStoreOp = stencil_store_op;
            return *this;
          }

          /// \brief Set the layouts (initial and final) of the attachment. The driver will make the transition for us
          attachment &set_layouts(VkImageLayout initial_layout, VkImageLayout final_layout)
          {
            reset_hash();
            vk_attachment_desc.initialLayout = initial_layout;
            vk_attachment_desc.finalLayout = final_layout;
            return *this;
          }

          /// \brief Set the format
          attachment &set_format(VkFormat format) { reset_hash(); vk_attachment_desc.format = format; return *this; }

          /// \brief Set the samples count
          attachment &set_samples(VkSampleCountFlagBits samples) { reset_hash(); vk_attachment_desc.samples = samples; return *this; }

        public: // advanced
          /// \brief Yield a const reference to VkAttachmentDescription
          operator const VkAttachmentDescription &() const { return vk_attachment_desc; }

          id_t compute_hash()
          {
            if (hash != id_t::none)
              return hash;

            hash = (id_t)ct::hash::fnv1a<64>(reinterpret_cast<const uint8_t*>(&vk_attachment_desc), sizeof(vk_attachment_desc));

            return hash;
          }

          void reset_hash()
          {
            hash = id_t::none;
          }

        private:
          VkAttachmentDescription vk_attachment_desc;
          id_t hash = id_t::none;
          const swapchain *swapchain_ptr;
          const pipeline_multisample_state *multisample_ptr;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_518920191126418147_250034947_ATTACHMENT_HPP__

