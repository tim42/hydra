//
// file : swapchain.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/swapchain.hpp
//
// created by : Timothée Feuillet
// date: Thu Apr 28 2016 23:23:34 GMT+0200 (CEST)
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

#ifndef __N_2428110920100477524_239810610_SWAPCHAIN_HPP__
#define __N_2428110920100477524_239810610_SWAPCHAIN_HPP__

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "device.hpp"
#include "surface.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wrap the swapchain vulkan extension
      /// (could be compared to a GL double buffer)
      class swapchain
      {
        public: // advanced
          /// \brief Construct a swapchain from a vulkan swapchain object
          swapchain(device &_dev, surface &_surf, VkSwapchainKHR _vk_swapchain)
            : dev(_dev), surf(_surf), vk_swapchain(_vk_swapchain)
          {
          }

          /// \brief Create a swapchain from the vulkan create info structure.
          /// This is really what will give you the best fine tuning
          /// capabilities at the expense of user friendliness
          swapchain(device &_dev, surface &_surf, VkSwapchainCreateInfoKHR create_info)
           : dev(_dev), surf(_surf)
          {
            create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            create_info.pNext = nullptr;
            create_info.surface = _surf._get_vk_surface();
            check::on_vulkan_error::n_throw_exception(vkCreateSwapchainKHR(dev._get_vk_device(), &create_info, nullptr, &vk_swapchain));
          }

        public:
          /// \brief Create a swapchain with a lot of default parameters that
          /// should be correct enough for most uses
          /// \note preferred image size is just a hint, but should be set to the
          /// windows height and width
          swapchain(device &_dev, surface &_surf, const glm::uvec2 &preferred_image_size)
            : swapchain(_dev, _surf, _surf.get_preferred_format(),
                        (_surf.get_current_size().x == (uint32_t)-1 &&
                         _surf.get_current_size().y == (uint32_t) - 1) ? preferred_image_size : _surf.get_current_size())
          {
          }

          /// \brief Create a swapchain, specifying some image configurations
          swapchain(device &_dev, surface &_surf, VkFormat image_format, const glm::uvec2 &image_size,
                    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                   )
            : dev(_dev), surf(_surf)
          {
            size_t image_count = 2;
            if (image_count < _surf.get_min_image_count())
              image_count = _surf.get_min_image_count();
            else if (image_count > _surf.get_max_image_count())
              image_count = _surf.get_max_image_count();

            VkSwapchainCreateInfoKHR create_info;
            create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            create_info.pNext = nullptr;
            create_info.surface = _surf._get_vk_surface();
            create_info.minImageCount = image_count;
            create_info.imageFormat = image_format;
            create_info.imageExtent.width = image_size.x;
            create_info.imageExtent.height = image_size.y;
            create_info.preTransform = _surf.get_preferred_transform();
            create_info.compositeAlpha = composite_alpha;
            create_info.imageArrayLayers = 1;
            create_info.presentMode = _surf.get_preferred_present_mode();
            create_info.oldSwapchain = VK_NULL_HANDLE;
            create_info.clipped = true;
            create_info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            create_info.imageUsage = image_usage;
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = NULL;

            check::on_vulkan_error::n_throw_exception(vkCreateSwapchainKHR(dev._get_vk_device(), &create_info, nullptr, &vk_swapchain));
          }

          /// \brief Move constructor
          swapchain(swapchain &&o)
            : dev(o.dev), surf(o.surf), vk_swapchain(o.vk_swapchain)
          {
            o.vk_swapchain = nullptr;
          }

          ~swapchain()
          {
            if (vk_swapchain)
              vkDestroySwapchainKHR(dev._get_vk_device(), vk_swapchain, nullptr);
          }

        public: // advanced
          VkSwapchainKHR _get_vk_swapchain() const
          {
            return vk_swapchain;
          }

        private:
          device &dev;
          surface &surf;
          VkSwapchainKHR vk_swapchain;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_2428110920100477524_239810610_SWAPCHAIN_HPP__

