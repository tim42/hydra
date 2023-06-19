//
// file : fence.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/fence.hpp
//
// created by : Timothée Feuillet
// date: Fri Apr 29 2016 11:03:53 GMT+0200 (CEST)
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

#include "device.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wrap a vulkan fence
      /// A fence is an object has two states: signaled and not signaled.
      /// Some operations (like executing a queue) require a fence and once the
      /// operation is done, the fence become signaled. This allow some
      /// synchronization between the program and the GPU (waiting for an
      /// operation to be completed before doing something else)
      ///
      /// There's also some static methods to wait for multiple fences
      class fence
      {
        public: // advanced
          /// \brief Construct a fence from a vulkan fence object
          fence(device &_dev, VkFence _vk_fence)
            : dev(_dev), vk_fence(_vk_fence)
          {
          }

        public:
          /// \brief Create a new fence
          /// \param create_signaled If true, the fence will be created in the signaled state
          fence(device &_dev, bool create_signaled = false)
            : dev(_dev)
          {
            VkFenceCreateInfo fenceInfo;
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.pNext = NULL;
            fenceInfo.flags = create_signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
            check::on_vulkan_error::n_assert_success(dev._vkCreateFence(&fenceInfo, nullptr, &vk_fence));
          }

          /// \brief Move constructor
          fence(fence &&o)
            : dev(o.dev), vk_fence(o.vk_fence)
          {
            o.vk_fence = nullptr;
          }

          fence& operator = (fence &&o)
          {
            if (&o == this) return *this;
            check::on_vulkan_error::n_assert(&dev == &o.dev, "Invalid move operation");
            if (vk_fence)
              dev._vkDestroyFence(vk_fence, nullptr);
            vk_fence = o.vk_fence;
            o.vk_fence = nullptr;
            return *this;
          }

          ~fence()
          {
            if (vk_fence)
              dev._vkDestroyFence(vk_fence, nullptr);
          }

          /// \brief Wait for the fence to become signaled
          /// \note after the fence has been set in signaled state, you have to reset it
          void wait() const
          {
            VkResult res;
            do
              res = dev._vkWaitForFences(1, &vk_fence, VK_TRUE, 100000000);
            while (res == VK_TIMEOUT);

#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
            check::on_vulkan_error::n_assert_success(forward_result(res) /* from vkWaitForFences() */);
#endif
          }

          /// \brief Wait for a specified time. If the time is expired and the
          /// fence isn't signaled yet, the method will return false. It will
          /// return true if the fence has been signaled.
          /// \note the timeout is specified in nanoseconds
          /// \note after the fence has been set in signaled state, you have to reset it
          bool wait_for(uint64_t nanosecond_timeout) const
          {
            VkResult res;
            res = dev._vkWaitForFences(1, &vk_fence, VK_TRUE, nanosecond_timeout);
            if (res == VK_TIMEOUT)
              return false;
#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
            check::on_vulkan_error::n_assert_success(forward_result(res) /* from vkWaitForFences() */);
#endif
            return res == VK_SUCCESS;
          }

          /// \brief Reset the fence to a non-signaled state
          void reset()
          {
            VkResult res;
            res = dev._vkResetFences(1, &vk_fence);
#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
            check::on_vulkan_error::n_assert_success(forward_result(res) /* from vkResetFences() */);
#endif
          }

          /// \brief Check if the fence has been signaled
          /// \note after the fence has been set in signaled state, you have to reset it
          bool is_signaled() const
          {
            VkResult res;
            res = dev._vkGetFenceStatus(vk_fence);
            if (res == VK_NOT_READY)
              return false;
#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
            check::on_vulkan_error::n_assert_success(forward_result(res) /* from vkGetFenceStatus() */);
#endif
            return res == VK_SUCCESS;
          }

          /// \brief Wait for multiple fences
          /// \param dev The device
          /// \param fc The fence container (can be any container that works with for (it : fc) loops)
          /// \param wait_for_all true if you want the function to return once ALL the fences are signaled,
          ///                     false if you want to wait for ANY of the fence to become signaled
          template<typename FenceContainer>
          static void multiple_wait(device &dev, const FenceContainer &fc, bool wait_for_all = true)
          {
            std::vector<VkFence> vk_fences;
            for (const fence &it : fc)
              vk_fences.push_back(it.vk_fence);

            VkResult res;
            do
              res = dev._vkWaitForFences(vk_fences.size(), vk_fences.data(), wait_for_all ? VK_TRUE : VK_FALSE, 100000000);
            while (res == VK_TIMEOUT);

#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
            check::on_vulkan_error::n_assert_success(forward_result(res) /* from vkWaitForFences() */);
#endif
          }

          /// \brief Wait for multiple fences for a specified time
          /// \param dev The device
          /// \param fc The fence container (can be any container that works with for (it : fc) loops)
          /// \param nanosecond_timeout The timeout, in nanoseconds
          /// \param wait_for_all true if you want the function to return once ALL the fences are signaled,
          ///                     false if you want to wait for ANY of the fence to become signaled
          /// \return Depending on the wait_for_all parameter, the return value could change:
          ///         if wait_for_all is true: a return value of true mean that every fence is signaled, false means
          ///           that not every one has been signaled in the specified timeout
          ///         if wait_for_all is false: a return value of true mean that at least one fence is signaled, false means
          ///           that no fence has been signaled in the specified timeout
          template<typename FenceContainer>
          static bool multiple_wait_for(device &dev, const FenceContainer &fc, uint64_t nanosecond_timeout, bool wait_for_all = true)
          {
            std::vector<VkFence> vk_fences;
            for (const fence &it : fc)
              vk_fences.push_back(it.vk_fence);

            VkResult res;
            res = dev._vkWaitForFences(vk_fences.size(), vk_fences.data(), wait_for_all ? VK_TRUE : VK_FALSE, nanosecond_timeout);

            if (res == VK_TIMEOUT)
              return false;
#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
            check::on_vulkan_error::n_assert_success(forward_result(res) /* from vkWaitForFences() */);
#endif
            return res == VK_SUCCESS;
          }

        public: // advanced
          /// \brief Return the vulkan object
          VkFence _get_vk_fence() const
          {
            return vk_fence;
          }

        private:
          inline static VkResult forward_result(VkResult res) {return res; }
        private:
          device &dev;
          VkFence vk_fence;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



