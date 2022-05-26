//
// file : event.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/event.hpp
//
// created by : Timothée Feuillet
// date: Fri Apr 29 2016 14:13:49 GMT+0200 (CEST)
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

#ifndef __N_1291425262325875861_1914419081_EVENT_HPP__
#define __N_1291425262325875861_1914419081_EVENT_HPP__

#include <vulkan/vulkan.h>

#include "device.hpp"
#include "../hydra_debug.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wrap a vulkan event
      class event
      {
        public: // advanced
          /// \brief Create an event from the vulkan object
          event(device &_dev, VkEvent _vk_event)
           : dev(_dev), vk_event(_vk_event)
          {
          }

        public:
          /// \brief Create a new event object
          event(device &_dev)
            : dev(_dev)
          {
            VkEventCreateInfo eci;
            eci.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
            eci.pNext = nullptr;
            eci.flags = 0;

            check::on_vulkan_error::n_assert_success(dev._vkCreateEvent(&eci, nullptr, &vk_event));
          }

          /// \brief Move constructor
          event(event &&o)
           : dev(o.dev), vk_event(o.vk_event)
          {
            o.vk_event = nullptr;
          }

          ~event()
          {
            if (vk_event)
              dev._vkDestroyEvent(vk_event, nullptr);
          }

          /// \brief Reset the state of the event
          /// (set the event to unsignaled)
          void reset()
          {
            VkResult res = dev._vkResetEvent(vk_event);
#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
            check::on_vulkan_error::n_assert_success(forward_result(res) /* from vkResetEvent() */);
#endif
          }

          /// \brief Set the event to the signaled state
          void signal()
          {
            VkResult res = dev._vkSetEvent(vk_event);
#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
            check::on_vulkan_error::n_assert_success(forward_result(res) /* from vkSetEvent() */);
#endif
          }

          /// \brief Return the status of the event. True: signaled, false: unsignaled (reset)
          bool get_status() const
          {
            VkResult res = dev._vkSetEvent(vk_event);
            if (res == VK_EVENT_RESET)
              return false;
            else if (res == VK_EVENT_SET)
              return true;
#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
            check::on_vulkan_error::n_assert_success(forward_result(res) /* from vkSetEvent() */);
#endif
            return false; // ERROR
          }

        public: // advanced
          /// \brief Return the vulkan object
          VkEvent _get_vk_event() const
          {
            return vk_event;
          }

        private:
          inline static VkResult forward_result(VkResult res) {return res; }
        private:
          device &dev;
          VkEvent vk_event;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_1291425262325875861_1914419081_EVENT_HPP__

