//
// file : framebuffer.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/framebuffer.hpp
//
// created by : Timothée Feuillet
// date: Mon Aug 22 2016 18:16:57 GMT+0200 (CEST)
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

#include "../hydra_debug.hpp"
#include "render_pass.hpp"
#include "image_view.hpp"
#include "swapchain.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkFramebuffer
      class framebuffer
      {
        public:
          /// \brief Create a framebuffer for a render pass with a vector of image view,
          ///        a swapchain (for automatic height & width) and an optional layer count
          framebuffer(device &_dev, const render_pass &rp, const std::vector<const image_view *> &ivv, const swapchain &_sw, size_t layers = 1)
            : dev(_dev), pass(&rp), image_view_vector(ivv), fb_dimension(_sw.get_dimensions(), layers), sw(&_sw)
          {
            create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            create_info.pNext = nullptr;
            create_info.flags = 0;
            create_info.renderPass = nullptr;
            create_info.attachmentCount = 0;
            create_info.pAttachments = nullptr;
            create_info.width = 0;
            create_info.height = 0;
            create_info.layers = 0;
            refresh();
          }

          /// \brief Create a framebuffer for a render pass with a vector of image view,
          ///        and a glm::uvec3 for the framebuffer dimensions
          framebuffer(device &_dev, const render_pass &rp, const std::vector<const image_view *> &ivv, const glm::uvec3 &dims)
            : dev(_dev), pass(&rp), image_view_vector(ivv), fb_dimension(dims)
          {
            create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            create_info.pNext = nullptr;
            create_info.flags = 0;
            create_info.renderPass = nullptr;
            create_info.attachmentCount = 0;
            create_info.pAttachments = nullptr;
            create_info.width = 0;
            create_info.height = 0;
            create_info.layers = 0;
            refresh();
          }

          /// \brief Move constructor
          framebuffer(framebuffer &&o)
            : dev(o.dev), vk_framebuffer(o.vk_framebuffer), create_info(o.create_info),
              pass(o.pass), vk_image_view_vector(std::move(o.vk_image_view_vector)),
              image_view_vector(std::move(o.image_view_vector)), fb_dimension(o.fb_dimension),
              sw(o.sw)
          {
            o.vk_framebuffer = nullptr;
          }
          framebuffer&operator = (framebuffer &&o)
          {
            if (&o == this) return *this;

            vk_framebuffer = (o.vk_framebuffer);
            create_info = (o.create_info);
            pass = (o.pass);
            vk_image_view_vector = (std::move(o.vk_image_view_vector));
            image_view_vector = (std::move(o.image_view_vector));
            fb_dimension = (o.fb_dimension);
            sw = (o.sw);

            o.vk_framebuffer = nullptr;
            return *this;
          }

          /// \brief Destructor
          ~framebuffer()
          {
            if (vk_framebuffer)
              dev._vkDestroyFramebuffer(vk_framebuffer, nullptr);
          }

          /// \brief Recreate the framebuffer if needed (if force is true, then always recreate the framebuffer)
          /// \note if a swapchain has been specified its height and width will be used in place of the uvec3 dimension x & y values
          void refresh(bool force = false)
          {
            bool recreate = force;
            if (!vk_framebuffer) recreate = true;
            if (get_dimensions() != get_future_dimensions()) recreate = true;
            if (pass->get_vk_render_pass() != create_info.renderPass) recreate = true;
            for (size_t i = 0; i < image_view_vector.size() && !recreate; ++i)
              recreate |= (vk_image_view_vector[i] != image_view_vector[i]->get_vk_image_view());
            if (!recreate) return;

            if (vk_framebuffer)
              dev._vkDestroyFramebuffer(vk_framebuffer, nullptr);

            glm::uvec3 dims = get_future_dimensions();
            create_info.width = dims.x;
            create_info.height = dims.y;
            create_info.layers = dims.z;
            create_info.renderPass = pass->get_vk_render_pass();

            vk_image_view_vector.clear();
            for (const image_view *iv : image_view_vector)
              vk_image_view_vector.push_back(iv->get_vk_image_view());
            create_info.pAttachments = vk_image_view_vector.data();
            create_info.attachmentCount = vk_image_view_vector.size();

            check::on_vulkan_error::n_assert_success(dev._vkCreateFramebuffer(&create_info, nullptr, &vk_framebuffer));
          }

          /// \brief Set the framebuffer dimensions
          /// \note To apply the change, you'll have to call refresh()
          /// \note if a swapchain has been set, the width and height aren't used
          void set_dimensions(const glm::uvec3 &dims) { fb_dimension = dims; }

          /// \brief Return the *current* size of the framebuffer
          ///        (a call to set_dimensions will not change the value until a call to refresh() is done)
          glm::uvec3 get_dimensions() const
          {
            return glm::uvec3(create_info.width, create_info.height, create_info.layers);
          }

          /// \brief Return the future size of the framebuffer
          ///        (a call to set_dimensions *will* change the value)
          ///        If a swapchain is specified, if will use its width and height
          glm::uvec3 get_future_dimensions() const
          {
            if (sw != nullptr)
              return glm::uvec3(sw->get_dimensions(), fb_dimension.z);
            return fb_dimension;
          }

          /// \brief Link a swapchain (set to nullptr to disable)
          /// The swapchain is used to get the width & height of the framebuffer
          void set_swapchain(const swapchain *_sw) { sw = _sw; }

          /// \brief Return the linked swapchain or nullptr
          const swapchain *get_swapchain() const { return sw; }

          /// \brief Set the render pass (call refresh for the change to take effect)
          void set_render_pass(const render_pass &rp) { pass = &rp; }

          /// \brief Return the render pass
          const render_pass &get_render_pass() const { return *pass; }

          VkFormat get_view_format (uint32_t view_index) const
          {
            check::debug::n_assert(view_index < image_view_vector.size(), "Out of bound access on image view vector");
            return image_view_vector[view_index] != nullptr ? image_view_vector[view_index]->get_view_format() : VK_FORMAT_UNDEFINED;
          }

          uint32_t get_view_count() const { return image_view_vector.size(); }

        public: // advanced
          /// \brief Return the Vulkan resource
          VkFramebuffer get_vk_framebuffer() const { return vk_framebuffer; }
          const std::vector<const image_view *>& get_image_view_vector() const { return image_view_vector; }

        private:
          device &dev;
          VkFramebuffer vk_framebuffer = nullptr;
          VkFramebufferCreateInfo create_info;

          const render_pass *pass;
          std::vector<VkImageView> vk_image_view_vector;
          std::vector<const image_view *> image_view_vector;
          glm::uvec3 fb_dimension;
          const swapchain *sw = nullptr;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



