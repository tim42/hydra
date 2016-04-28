//
// file : hydra_vulkan_instance.hpp
// in : file:///home/tim/projects/hydra/hydra/init/hydra_vulkan_instance.hpp
//
// created by : Timothée Feuillet
// date: Tue Apr 26 2016 11:53:06 GMT+0200 (CEST)
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

#ifndef __N_692459433198810776_77086004_HYDRA_VULKAN_INSTANCE_HPP__
#define __N_692459433198810776_77086004_HYDRA_VULKAN_INSTANCE_HPP__

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "../hydra_exception.hpp"
#include "device.hpp"

namespace neam
{
  namespace hydra
  {
    class hydra_device_creator;

    /// \brief Wrap a vulkan instance
    class hydra_vulkan_instance
    {
      public:
        /// \brief The only reason this constructor is public is to allow
        /// compatibility with other means of creating a vulkan instance.
        /// Instead of calling this directly, you should use the class
        /// hydra_instance_creator that handle the creation for you.
        hydra_vulkan_instance(VkInstance _vulkan_instance, const std::string &_app_name = "")
          : vulkan_instance(_vulkan_instance), app_name(_app_name)
        {
          enumerate_devices();
        }

        /// \brief Move constructor.
        /// Allow move semantics
        hydra_vulkan_instance(hydra_vulkan_instance &&o)
         : vulkan_instance(o.vulkan_instance), app_name(o.app_name),
            gpu_list(std::move(o.gpu_list))
        {
          o.vulkan_instance = nullptr;
        }

        /// \brief Destructor
        ~hydra_vulkan_instance()
        {
          if (vulkan_instance != nullptr)
            vkDestroyInstance(vulkan_instance, nullptr);
        }

        /// \brief Return the number of GPU the system has
        size_t get_device_count() const
        {
          return gpu_list.size();
        }

        /// \brief Return a wrapper around a vulkan GPU
        const vk_device &get_device(size_t index) const
        {
          return gpu_list.at(index);
        }

        /// \brief Brief return a new device creator
        /// A device creator allows the user to request a logical device to vulkan
        /// in a easier fashion than the vulkan way
        /// \note the device creator should not live past the destruction of the
        ///       instance that created it
        ///
        /// implementation is in hydra_device_creator.hpp
        hydra_device_creator get_device_creator() const;

      public: // advanced
        /// \brief Return the vulkan instance object
        /// \note "Marked" as advanced ('cause that's not meant to be used by the end user)
        VkInstance _get_vulkan_instance()
        {
          return vulkan_instance;
        }

      private:
        void enumerate_devices()
        {
          uint32_t gpu_count = 0;
          check::on_vulkan_error::n_throw_exception(vkEnumeratePhysicalDevices(vulkan_instance, &gpu_count, nullptr));
          check::on_vulkan_error::n_assert(gpu_count > 0, "no compatible GPU found");

          std::vector<VkPhysicalDevice> vk_gpu_list;

          vk_gpu_list.resize(gpu_count);
          check::on_vulkan_error::n_throw_exception(vkEnumeratePhysicalDevices(vulkan_instance, &gpu_count, vk_gpu_list.data()));

          for (const VkPhysicalDevice &it : vk_gpu_list)
            gpu_list.emplace_back(vk_device(it));
        }

      private:
        VkInstance vulkan_instance;
        std::string app_name;
        std::vector<vk_device> gpu_list;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_692459433198810776_77086004_HYDRA_VULKAN_INSTANCE_HPP__
