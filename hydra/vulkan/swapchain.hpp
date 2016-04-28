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

#include "device.hpp"

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
          swapchain(device &_dev, VkSwapchainKHR _vk_swapchain)
            : dev(_dev), vk_swapchain(_vk_swapchain)
          {
          }

        public:
          /// \brief Move constructor
          swapchain(swapchain &&o)
            : dev(o.dev), vk_swapchain(o.vk_swapchain)
          {
            o.vk_swapchain = nullptr;
          }

          ~swapchain()
          {
            if (vk_swapchain)
              vkDestroySwapchainKHR(dev._get_vulkan_device(), vk_swapchain, nullptr);
          }

        public: // advanced
          VkSwapchainKHR _get_vk_swapchain()
          {
            return vk_swapchain;
          }

        private:
          device &dev;
          VkSwapchainKHR vk_swapchain;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_2428110920100477524_239810610_SWAPCHAIN_HPP__

