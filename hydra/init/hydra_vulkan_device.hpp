//
// file : hydra_vulkan_device.hpp
// in : file:///home/tim/projects/hydra/hydra/init/hydra_vulkan_device.hpp
//
// created by : Timothée Feuillet
// date: Wed Apr 27 2016 16:54:15 GMT+0200 (CEST)
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

#ifndef __N_95735850311973920_182594734_HYDRA_VULKAN_DEVICE_HPP__
#define __N_95735850311973920_182594734_HYDRA_VULKAN_DEVICE_HPP__

#include <string>

#include <vulkan/vulkan.h>

#include "../hydra_exception.hpp"

#define HYDRA_GET_DEVICE_PROC_ADDR(dev, entrypoint)                            \
    {                                                                          \
        demo->fp##entrypoint =                                                 \
            (PFN_vk##entrypoint)vkGetDeviceProcAddr(dev, "vk" #entrypoint);    \
        if (demo->fp##entrypoint == NULL) {                                    \
            ERR_EXIT("vkGetDeviceProcAddr failed to find vk" #entrypoint,      \
                     "vkGetDeviceProcAddr Failure");                           \
        }                                                                      \
    }

namespace neam
{
  namespace hydra
  {
    /// \brief Wraps a vulkan logical device
    class hydra_vulkan_device
    {
      public: // advanced
        /// \brief You shouldn't have to call this directly, but instead you should
        /// ask the hydra_device_creator class a new device
        hydra_vulkan_device(VkDevice _device)
         : device(_device)
        {
//           vkGetDeviceProcAddr();
        }

      public:
        ~hydra_vulkan_device()
        {
          vkDestroyDevice(device, nullptr);
        }

      public: // advanced
        /// \brief Return the address of a procedure. No check is performed.
        inline PFN_vkVoidFunction _get_proc_addr(const std::string &name)
        {
          return vkGetDeviceProcAddr(device, name.c_str());
        }

      private:
        VkDevice device;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_95735850311973920_182594734_HYDRA_VULKAN_DEVICE_HPP__

