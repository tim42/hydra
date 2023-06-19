//
// file : surface.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/surface.hpp
//
// created by : Timothée Feuillet
// date: Thu Apr 28 2016 22:11:55 GMT+0200 (CEST)
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

#include "instance.hpp"
#include "physical_device.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wrap the KHR surface extension
      /// \note The surface is tied to a physical device
      class surface
      {
        public: // advanced
          surface(instance &_inst, VkSurfaceKHR _vk_surface)
            : inst(_inst), phydev(physical_device::create_null_physical_device()), vk_surface(_vk_surface)
          {
          }

          surface(instance &_inst, const physical_device &_phydev, VkSurfaceKHR _vk_surface)
            : inst(_inst), phydev(_phydev), vk_surface(_vk_surface)
          {
            reload_capabilities();

            uint32_t count = 0;
            check::on_vulkan_error::n_assert_success(
              vkGetPhysicalDeviceSurfaceFormatsKHR(phydev._get_vk_physical_device(), vk_surface, &count, nullptr)
            );
            formats.resize(count);
            check::on_vulkan_error::n_assert_success(
              vkGetPhysicalDeviceSurfaceFormatsKHR(phydev._get_vk_physical_device(), vk_surface, &count, formats.data())
            );

            count = 0;
            check::on_vulkan_error::n_assert_success(
              vkGetPhysicalDeviceSurfacePresentModesKHR(phydev._get_vk_physical_device(), vk_surface, &count, nullptr)
            );
            modes.resize(count);
            check::on_vulkan_error::n_assert_success(
              vkGetPhysicalDeviceSurfacePresentModesKHR(phydev._get_vk_physical_device(), vk_surface, &count, modes.data())
            );
          }

        public:
          /// \brief Copy operator
          surface(surface &&o)
            : inst(o.inst), phydev(o.phydev), vk_surface(o.vk_surface),
              capabilities(o.capabilities), formats(std::move(o.formats)),
              modes(std::move(o.modes))
          {
            o.vk_surface = nullptr;
          }

          ~surface()
          {
            if (vk_surface)
              vkDestroySurfaceKHR(inst._get_vk_instance(), vk_surface, nullptr);
          }

          /// \brief Reload the surface capabilities
          void reload_capabilities()
          {
            if (phydev._get_vk_physical_device() == nullptr)
              return;
            check::on_vulkan_error::n_assert_success(
              vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phydev._get_vk_physical_device(), vk_surface, &capabilities)
            );
          }

          /// \brief Set the physical device of the surface
          void set_physical_device(physical_device _phydev)
          {
            phydev = _phydev;

            reload_capabilities();
            uint32_t count = 0;
            check::on_vulkan_error::n_assert_success(
              vkGetPhysicalDeviceSurfaceFormatsKHR(phydev._get_vk_physical_device(), vk_surface, &count, nullptr)
            );
            formats.resize(count);
            check::on_vulkan_error::n_assert_success(
              vkGetPhysicalDeviceSurfaceFormatsKHR(phydev._get_vk_physical_device(), vk_surface, &count, formats.data())
            );

            count = 0;
            check::on_vulkan_error::n_assert_success(
              vkGetPhysicalDeviceSurfacePresentModesKHR(phydev._get_vk_physical_device(), vk_surface, &count, nullptr)
            );
            modes.resize(count);
            check::on_vulkan_error::n_assert_success(
              vkGetPhysicalDeviceSurfacePresentModesKHR(phydev._get_vk_physical_device(), vk_surface, &count, modes.data())
            );
          }

          /// \brief Return an arbitrary defined preferred present mode
          /// It will look for the best one (fast and nice) then the fast one
          /// and finally the fallback
          VkPresentModeKHR get_preferred_present_mode() const
          {
            VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
            for (size_t i = 0; i < modes.size(); i++)
            {
              if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
              {
                mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
              }
              else if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
                mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
            return mode;
          }

          /// \brief Return an arbitrary defined image format for the surface
          /// (it will try to get the 32bit BGRA)
          VkFormat get_preferred_format() const
          {
            if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
              return VK_FORMAT_B8G8R8A8_UNORM;
            else if (formats.size() >= 1)
            {
              for (auto &it : formats)
              {
                if (it.format == VK_FORMAT_B8G8R8A8_UNORM)
                  return VK_FORMAT_B8G8R8A8_UNORM;
              }
              return formats[0].format;
            }

#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
            check::on_vulkan_error::n_assert(false, "no image format is supported by the surface");
#endif
            return VK_FORMAT_B8G8R8A8_UNORM;
          }

          /// \brief Return an arbitrary defined transform that will most likely
          /// fit every need
          VkSurfaceTransformFlagBitsKHR get_preferred_transform() const
          {
            if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
              return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            return capabilities.currentTransform;
          }

          /// \brief Get the minimum number of images the surface can have
          /// (used when defining the swapchain)
          size_t get_min_image_count() const
          {
            return capabilities.minImageCount;
          }
          /// \brief Get the maximum number of images the surface can have
          /// (used when defining the swapchain)
          size_t get_max_image_count() const
          {
            return capabilities.maxImageCount;
          }

          /// \brief Return the current surface size
          glm::uvec2 get_current_size() const
          {
            return glm::uvec2(capabilities.currentExtent.width, capabilities.currentExtent.height);
          }

          /// \brief Return the minimum surface size
          glm::uvec2 get_minimum_size() const
          {
            return glm::uvec2(capabilities.minImageExtent.width, capabilities.minImageExtent.height);
          }
          /// \brief Return the maximum surface size
          glm::uvec2 get_maximum_size() const
          {
            return glm::uvec2(capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);
          }

          /// \brief Return the current transformation of the surface
          VkSurfaceTransformFlagsKHR get_current_transform() const
          {
            return capabilities.currentTransform;
          }

          /// \brief Return the transformations the surface can handle
          VkSurfaceTransformFlagsKHR get_supported_transforms() const
          {
            return capabilities.supportedTransforms;
          }

          /// \brief Return the what type of alpha compositing the surface supports
          VkCompositeAlphaFlagsKHR get_supported_composite_alpha() const
          {
            return capabilities.supportedCompositeAlpha;
          }

          /// \brief Return the image usage flags accepted by the surface
          VkImageUsageFlags get_image_usage_flags() const
          {
            return capabilities.supportedUsageFlags;
          }

          /// \brief Return all the different presenting modes the surface supports
          /// \see get_preferred_present_mode()
          const std::vector<VkPresentModeKHR> &get_supported_present_modes() const
          {
            return modes;
          }

          /// \brief Return the formats (image format and color space) the surface can handle
          const std::vector<VkSurfaceFormatKHR> &get_supported_surface_formats() const
          {
            return formats;
          }

        public: // advanced
          /// \brief Return the wrapped vulkan surface
          VkSurfaceKHR _get_vk_surface() const
          {
            return vk_surface;
          }

        private:
          instance &inst;
          physical_device phydev;
          VkSurfaceKHR vk_surface;

          VkSurfaceCapabilitiesKHR capabilities;
          std::vector<VkSurfaceFormatKHR> formats;
          std::vector<VkPresentModeKHR> modes;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



