//
// file : image_view.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/image_view.hpp
//
// created by : Timothée Feuillet
// date: Fri May 06 2016 14:34:34 GMT+0200 (CEST)
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

#ifndef __N_2068815951828916619_2910431576_IMAGE_VIEW_HPP__
#define __N_2068815951828916619_2910431576_IMAGE_VIEW_HPP__

#include <vulkan/vulkan.h>

#include "../hydra_debug.hpp"
#include "image.hpp"
#include "rgba_swizzle.hpp"
#include "image_subresource_range.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief A image view. You should not create one by yourself, but instead
      /// ask an image instance for an image_view.
      class image_view
      {
        private:
          static VkImageViewType get_view_type_from_image(VkImageType t, glm::uvec3 img_sz)
          {
            switch (t)
            {
              case VK_IMAGE_TYPE_1D:
                if (img_sz.y == 1)
                  return VK_IMAGE_VIEW_TYPE_1D;
                return VK_IMAGE_VIEW_TYPE_1D_ARRAY;

              case VK_IMAGE_TYPE_2D:
                if (img_sz.z == 1)
                  return VK_IMAGE_VIEW_TYPE_2D;
                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

              case VK_IMAGE_TYPE_3D: return VK_IMAGE_VIEW_TYPE_3D;

              case VK_IMAGE_TYPE_MAX_ENUM:
              default:
                check::debug::n_check(false, "invalid image format");
            }
            return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
          }
        public: // advanced
          /// \brief Construct an image_view from a vulkan create-info object
          image_view(device &_dev, const VkImageViewCreateInfo &vk_create_info, VkImageView _vk_image_view = nullptr)
           : dev(_dev), vk_image_view(_vk_image_view), view_create_info(vk_create_info)
          {
            if (!vk_image_view)
              check::on_vulkan_error::n_assert_success(dev._vkCreateImageView(&view_create_info, nullptr, &vk_image_view));
          }

          /// \brief Create a VkImageViewCreateInfo from a set of 2+1+3 parameters
          image_view(device &_dev, const image &img, VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_MAX_ENUM, VkFormat view_format = VK_FORMAT_MAX_ENUM, const rgba_swizzle &comp_mapping = rgba_swizzle(), const image_subresource_range &isr = image_subresource_range())
           : image_view(_dev, VkImageViewCreateInfo
          {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            img.get_vk_image(),
            (view_type == VK_IMAGE_VIEW_TYPE_MAX_ENUM ? get_view_type_from_image(img.get_image_type(), img.get_size()) : view_type),
            (view_format == VK_FORMAT_MAX_ENUM ? img.get_image_format() : view_format),
            comp_mapping,
            isr
          })
          {
          }

        public:
          /// \brief Move constructor
          image_view(image_view &&o)
           : dev(o.dev), vk_image_view(o.vk_image_view), view_create_info(o.view_create_info)
          {
            o.vk_image_view = nullptr;
          }

          /// \brief Move
          image_view &operator = (image_view &&o)
          {
            if (&o == this)
              return *this;
            check::on_vulkan_error::n_assert(&o.dev == &dev, "can't assign image views with different vulkan devices");

            if (vk_image_view)
              dev._vkDestroyImageView(vk_image_view, nullptr);


            vk_image_view = o.vk_image_view;
            view_create_info = o.view_create_info;
            return *this;
          }

          /// \brief Destructor
          ~image_view()
          {
            if (vk_image_view)
              dev._vkDestroyImageView(vk_image_view, nullptr);
          }

        public: // advanced
          /// \brief Return the vulkan image view
          VkImageView get_vk_image_view() const
          {
            return vk_image_view;
          }

        private:
          device &dev;
          VkImageView vk_image_view;

          VkImageViewCreateInfo view_create_info;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_2068815951828916619_2910431576_IMAGE_VIEW_HPP__

