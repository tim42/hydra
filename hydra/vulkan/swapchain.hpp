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

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "device.hpp"
#include "surface.hpp"
#include "image.hpp"
#include "image_view.hpp"
#include "fence.hpp"
#include "semaphore.hpp"

#include "viewport.hpp"
#include "rect2D.hpp"
#include "pipeline_viewport_state.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wrap the swapchain vulkan extension
      /// (could be compared to a GL double/triple buffer)
      class swapchain
      {
        public: // advanced
          /// \brief Construct a swapchain from a vulkan swapchain object
          swapchain(device &_dev, surface &_surf, VkSwapchainKHR _vk_swapchain, const VkSwapchainCreateInfoKHR &_create_info)
            : dev(_dev), surf(_surf), vk_swapchain(_vk_swapchain), create_info(_create_info),
              sw_viewport(glm::vec2(create_info.imageExtent.width, create_info.imageExtent.height)),
              sw_rect(glm::ivec2(0, 0), glm::uvec2(create_info.imageExtent.width, create_info.imageExtent.height))
          {
            populate_image_vector();
          }

          /// \brief Create a swapchain from the vulkan create info structure.
          /// This is really what will give you the best fine tuning
          /// capabilities at the expense of user friendliness
          swapchain(device &_dev, surface &_surf, const VkSwapchainCreateInfoKHR &_create_info)
            : dev(_dev), surf(_surf), create_info(_create_info),
              sw_viewport(glm::vec2(create_info.imageExtent.width, create_info.imageExtent.height)),
              sw_rect(glm::ivec2(0, 0), glm::uvec2(create_info.imageExtent.width, create_info.imageExtent.height))
          {
            create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            create_info.pNext = nullptr;
            create_info.surface = _surf._get_vk_surface();
            check::on_vulkan_error::n_assert_success(vkCreateSwapchainKHR(dev._get_vk_device(), &create_info, nullptr, &vk_swapchain));

            populate_image_vector();
          }

        public:
          /// \brief Create a swapchain with a lot of default parameters that
          /// should be correct enough for most uses
          /// \note preferred image size is just a hint, but should be set to the
          /// windows height and width
          swapchain(device &_dev, surface &_surf, glm::uvec2 preferred_image_size)
            : swapchain(_dev, _surf, _surf.get_preferred_format(),
                        (_surf.get_current_size().x == (uint32_t) - 1 &&
                         _surf.get_current_size().y == (uint32_t) - 1) ? preferred_image_size : _surf.get_current_size())
          {
          }

          /// \brief Create a swapchain, specifying some image configurations
          swapchain(device &_dev, surface &_surf, VkFormat image_format, glm::uvec2 image_size,
                    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            : dev(_dev), surf(_surf), sw_viewport(glm::vec2(image_size.x, image_size.y)),
              sw_rect(glm::ivec2(0, 0), image_size)
          {
            size_t image_count = 3;
            if (image_count < _surf.get_min_image_count())
              image_count = _surf.get_min_image_count();
            else if (image_count > _surf.get_max_image_count())
              image_count = _surf.get_max_image_count();

            create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            create_info.pNext = nullptr;
            create_info.flags = 0;
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

            check::on_vulkan_error::n_assert_success(vkCreateSwapchainKHR(dev._get_vk_device(), &create_info, nullptr, &vk_swapchain));

            populate_image_vector();
          }

          /// \brief Move constructor
          swapchain(swapchain &&o)
            : dev(o.dev), surf(o.surf), vk_swapchain(o.vk_swapchain), create_info(o.create_info), swapchain_images(std::move(o.swapchain_images)),
              swapchain_image_views(std::move(o.swapchain_image_views)), sw_viewport(o.sw_viewport), sw_rect(o.sw_rect)
          {
            o.vk_swapchain = nullptr;
          }

          ~swapchain()
          {
            if (vk_swapchain)
              vkDestroySwapchainKHR(dev._get_vk_device(), vk_swapchain, nullptr);
          }

          /// \brief Return the image format of the swapchain
          VkFormat get_image_format() const
          {
            return create_info.imageFormat;
          }

          /// \brief Return the image count of the swapchain
          size_t get_image_count() const
          {
            return swapchain_images.size();
          }

          /// \brief Return a const reference to the image vector of the swapchain
          const std::deque<image> &get_image_vector() const
          {
            return swapchain_images;
          }

          /// \brief Return a const reference to the image vector of the swapchain
          const std::deque<image_view> &get_image_view_vector() const
          {
            return swapchain_image_views;
          }

          /// \brief Return a const reference to the image vector of the swapchain
          std::deque<image> &get_image_vector()
          {
            return swapchain_images;
          }

          /// \brief Return a const reference to the image vector of the swapchain
          std::deque<image_view> &get_image_view_vector()
          {
            return swapchain_image_views;
          }

          /// \brief Recreate the swapchain (do not forget to invalidate comand buffers and everything that depends on the swapchain !)
          /// \note You should call this at a correct timing (to avoid freeing in-use objects)
          swapchain recreate_swapchain(const glm::uvec2 &image_size)
          {
            create_info.oldSwapchain = vk_swapchain;
            swapchain ret(std::move(*this));

            swapchain_image_views.clear();
            swapchain_images.clear();
            surf.reload_capabilities();

            if (surf.get_current_size().x != ~0u)
            {
              create_info.imageExtent.width = surf.get_current_size().x;
              create_info.imageExtent.height = surf.get_current_size().y;
            }
            else
            {
              create_info.imageExtent.width = glm::clamp(image_size.x, surf.get_minimum_size().x, surf.get_maximum_size().x);
              create_info.imageExtent.height = glm::clamp(image_size.y, surf.get_minimum_size().y, surf.get_maximum_size().y);
            }
            check::on_vulkan_error::n_assert_success(vkCreateSwapchainKHR(dev._get_vk_device(), &create_info, nullptr, &vk_swapchain));

//             if (create_info.oldSwapchain)
//               vkDestroySwapchainKHR(dev._get_vk_device(), create_info.oldSwapchain, nullptr);
            create_info.oldSwapchain = nullptr;

            sw_viewport.set_size({create_info.imageExtent.width, create_info.imageExtent.height});
            sw_rect.set_size({create_info.imageExtent.width, create_info.imageExtent.height});

            populate_image_vector();
            return ret;
          }

          /// \brief Get the next image from the swapchain
          /// \note This version does not signal a fence nor a semaphore
          /// \param[out] should_recreate if specified, tells if the swapchain SHOULD be recreated
          /// \return the image index or -1 if the swapchain MUST be recreated
          /// \throw neam::hydra::exception_tpl on any error
          uint32_t get_next_image_index(uint64_t timeout_ns = std::numeric_limits<uint64_t>::max(), bool *should_recreate = nullptr)
          {
            uint32_t image_index;
            VkResult res = vkAcquireNextImageKHR(dev._get_vk_device(), vk_swapchain, timeout_ns, nullptr, nullptr, &image_index);

            if (res == VK_ERROR_OUT_OF_DATE_KHR)
            {
              if (should_recreate)
                *should_recreate = true;
              return (uint32_t)-1;
            }
            else if (res == VK_SUBOPTIMAL_KHR && should_recreate)
              *should_recreate = true;
            else // check for errors on vkAcquireNextImageKHR
              check::on_vulkan_error::n_assert_success(res);

            return image_index;
          }

          /// \brief Get the next image from the swapchain
          /// \note This version signals a fence
          /// \param[out] should_recreate if specified, tells if the swapchain SHOULD be recreated
          /// \return the image index or -1 if the swapchain MUST be recreated
          /// \throw neam::hydra::exception_tpl on any error
          uint32_t get_next_image_index(fence &fnc, uint64_t timeout_ns = std::numeric_limits<uint64_t>::max(), bool *should_recreate = nullptr)
          {
            uint32_t image_index;
            VkResult res = vkAcquireNextImageKHR(dev._get_vk_device(), vk_swapchain, timeout_ns, nullptr, fnc._get_vk_fence(), &image_index);

            if (res == VK_ERROR_OUT_OF_DATE_KHR)
            {
              if (should_recreate)
                *should_recreate = true;
              return (uint32_t)-1;
            }
            else if (res == VK_SUBOPTIMAL_KHR && should_recreate)
              *should_recreate = true;
            else // check for errors on vkAcquireNextImageKHR
              check::on_vulkan_error::n_assert_success(res);

            return image_index;
          }

          /// \brief Get the next image from the swapchain
          /// \note This version signals a semaphore
          /// \param[out] should_recreate if specified, tells if the swapchain SHOULD be recreated
          /// \return the image index or -1 if the swapchain MUST be recreated
          /// \throw neam::hydra::exception_tpl on any error
          uint32_t get_next_image_index(semaphore &sema, uint64_t timeout_ns = std::numeric_limits<uint64_t>::max(), bool *should_recreate = nullptr)
          {
            uint32_t image_index;
            VkResult res = vkAcquireNextImageKHR(dev._get_vk_device(), vk_swapchain, timeout_ns, sema._get_vk_semaphore(), nullptr, &image_index);

            if (res == VK_ERROR_OUT_OF_DATE_KHR)
            {
              if (should_recreate)
                *should_recreate = true;
              return (uint32_t)-1;
            }
            else if (res == VK_SUBOPTIMAL_KHR && should_recreate)
              *should_recreate = true;
            else // check for errors on vkAcquireNextImageKHR
              check::on_vulkan_error::n_assert_success(res);

            return image_index;
          }

          /// \brief Get the next image from the swapchain
          /// \note This version signals a semaphore and a fence
          /// \param[out] should_recreate if specified, tells if the swapchain SHOULD be recreated
          /// \return the image index or -1 if the swapchain MUST be recreated
          /// \throw neam::hydra::exception_tpl on any error
          uint32_t get_next_image_index(semaphore &sema, fence &fnc, uint64_t timeout_ns = std::numeric_limits<uint64_t>::max(), bool *should_recreate = nullptr)
          {
            uint32_t image_index;
            VkResult res = vkAcquireNextImageKHR(dev._get_vk_device(), vk_swapchain, timeout_ns, sema._get_vk_semaphore(), fnc._get_vk_fence(), &image_index);

            if (res == VK_ERROR_OUT_OF_DATE_KHR)
            {
              if (should_recreate)
                *should_recreate = true;
              return (uint32_t)-1;
            }
            else if (res == VK_SUBOPTIMAL_KHR && should_recreate)
              *should_recreate = true;
            else // check for errors on vkAcquireNextImageKHR
              check::on_vulkan_error::n_assert_success(res);

            return image_index;
          }

          /// \brief Return a reference to a viewport that covers the whole surface
          /// \note that reference is updated when the swapchain is recreated
          const viewport &get_full_viewport() const
          {
            return sw_viewport;
          }

          /// \brief Return a rectD that covers the whole viewport
          /// \note that reference is updated when the swapchain is recreated
          const rect2D &get_full_rect2D() const
          {
            return sw_rect;
          }

          /// \brief Return a uvec2 (not a reference !!) hat describe the width and height of the swapchain
          glm::uvec2 get_dimensions() const
          {
            return glm::uvec2(create_info.imageExtent.width, create_info.imageExtent.height);
          }


        public: // advanced
          VkSwapchainKHR _get_vk_swapchain() const
          {
            return vk_swapchain;
          }

          /// \brief Create a new pipeline_viewport_state from the swapchain
          /// \note It should not outlive the swapchain that created it
          pipeline_viewport_state _create_pipeline_viewport_state() const
          {
            return pipeline_viewport_state({get_full_viewport()}, {get_full_rect2D()});
          }

          surface& _get_surface()
          {
            return surf;
          }

        private: // helper
          void populate_image_vector()
          {
            uint32_t image_count;
            vkGetSwapchainImagesKHR(dev._get_vk_device(), vk_swapchain, &image_count, nullptr);
            std::vector<VkImage> vk_images;
            vk_images.resize(image_count);
            vkGetSwapchainImagesKHR(dev._get_vk_device(), vk_swapchain, &image_count, vk_images.data());

            // a fake create info with just enough populated to make the image class works correctly
            VkImageCreateInfo img_create_info;
            img_create_info.imageType = VK_IMAGE_TYPE_2D;
            img_create_info.format = create_info.imageFormat;
            img_create_info.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            // update/populate the vk::image vector
            for (size_t i = 0; i < vk_images.size(); ++i)
            {
              if (i >= swapchain_images.size())
                swapchain_images.emplace_back(dev, vk_images[i], img_create_info, true);
              else
                swapchain_images[i] = image(dev, vk_images[i], img_create_info, true);
            }

            // update/populate the vk::image_view vector
            for (size_t i = 0; i < swapchain_images.size(); ++i)
            {
              if (i >= swapchain_image_views.size())
                swapchain_image_views.emplace_back(dev, swapchain_images[i], VK_IMAGE_VIEW_TYPE_2D);
              else
                swapchain_image_views[i] = image_view(dev, swapchain_images[i], VK_IMAGE_VIEW_TYPE_2D);
            }
          }

        private: // properties
          device &dev;
          surface &surf;
          VkSwapchainKHR vk_swapchain = nullptr;
          VkSwapchainCreateInfoKHR create_info;

          std::deque<image> swapchain_images;
          std::deque<image_view> swapchain_image_views;
          viewport sw_viewport;
          rect2D sw_rect;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_2428110920100477524_239810610_SWAPCHAIN_HPP__

