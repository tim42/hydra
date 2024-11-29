//
// file : semaphore.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/semaphore.hpp
//
// created by : Timothée Feuillet
// date: Fri Apr 29 2016 13:52:51 GMT+0200 (CEST)
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
#include "../hydra_debug.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wrap a semaphore
      /// A semaphore is an object that allows inter-queue synchronisation
      class semaphore
      {
        public: // advanced
          /// \brief Construct a semaphore from a vulkan object
          semaphore(device &_dev, VkSemaphore _vk_semaphore)
           : dev(_dev), vk_semaphore(_vk_semaphore)
          {
          }

        public:
          /// \brief Create a new semaphore object
          semaphore(device& _dev, const std::source_location& sloc = std::source_location::current())
           : semaphore(_dev, fmt::format("semaphore: {} : {} [{}]", sloc.file_name(), sloc.line(), sloc.function_name()))
          {}

          /// \brief Create a new semaphore object
          semaphore(device &_dev, const std::string& name)
            : dev(_dev)
          {
            VkSemaphoreCreateInfo sci;

            sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            sci.pNext = nullptr;
            sci.flags = 0;

            check::on_vulkan_error::n_assert_success(dev._vkCreateSemaphore(&sci, nullptr, &vk_semaphore));
            _set_debug_name(name);
          }

          /// \brief Move constructor
          semaphore(semaphore &&o)
           : dev(o.dev), vk_semaphore(o.vk_semaphore)
          {
            o.vk_semaphore = nullptr;
          }

          semaphore& operator = (semaphore &&o)
          {
            if (&o == this) return *this;

            check::debug::n_assert(&dev == &o.dev, "Cannot assign semaphore from another device");
            vk_semaphore = o.vk_semaphore;
            o.vk_semaphore = nullptr;
            return *this;
          }

          ~semaphore()
          {
            if (vk_semaphore)
              dev._vkDestroySemaphore(vk_semaphore, nullptr);
          }

        public: // advanced
          /// \brief Return the vulkan object
          VkSemaphore _get_vk_semaphore() const
          {
            return vk_semaphore;
          }

          void _set_debug_name(const std::string& name)
          {
            dev._set_object_debug_name((uint64_t)vk_semaphore, VK_OBJECT_TYPE_SEMAPHORE, name);
          }

        private:
          device &dev;
          VkSemaphore vk_semaphore;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



