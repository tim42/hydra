//
// file : device.hpp
// in : file:///home/tim/projects/hydra/hydra/init/device.hpp
//
// created by : Timothée Feuillet
// date: Tue Apr 26 2016 17:29:10 GMT+0200 (CEST)
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

#ifndef __N_683913722694924741_2220927619_DEVICE_HPP__
#define __N_683913722694924741_2220927619_DEVICE_HPP__

#include <vector>
#include <vulkan/vulkan.h>

#include "device_features.hpp"
#include "layer.hpp"
#include "extension.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wrap a vulkan physical device (GPU)
      /// \note This is low level, you probably don't want to deal with this directly
      class physical_device
      {
        private:
          // if you want to use it, explicitly ask it via ::create_null_physical_device()
          // having a null physical device may lead to crashes if used like a normal physical
          // device.
          physical_device() : gpu(nullptr) {}

        public: // advanced
          physical_device(VkPhysicalDevice _gpu) : gpu(_gpu)
          {
            vkGetPhysicalDeviceProperties(gpu, &properties);
            vkGetPhysicalDeviceMemoryProperties(gpu, &memory_properties);

            // load features
            VkPhysicalDeviceFeatures vk_features;
            vkGetPhysicalDeviceFeatures(gpu, &vk_features);
            features = device_features(vk_features);

            // load queues information
            uint32_t count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, nullptr);
            queues.resize(count);
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, queues.data());

            VkResult res;
            // load layers for this device
            std::vector<VkLayerProperties> vk_layers;
            do
            {
              check::on_vulkan_error::n_assert_success(vkEnumerateDeviceLayerProperties(gpu, &count, nullptr));
              if (count == 0)
                break;
              vk_layers.resize(count);
              res = check::on_vulkan_error::n_assert_success(vkEnumerateDeviceLayerProperties(gpu, &count, vk_layers.data()));
            }
            while (res == VK_INCOMPLETE);
            for (const VkLayerProperties &it : vk_layers)
              layers.emplace_back(layer(it, gpu));

            // load extensions for this device
            std::vector<VkExtensionProperties> vk_extensions;
            do
            {
              check::on_vulkan_error::n_assert_success(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr));
              if (count == 0)
                break;
              vk_extensions.resize(count);
              res = check::on_vulkan_error::n_assert_success(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, vk_extensions.data()));
            }
            while (res == VK_INCOMPLETE);
            for (const VkExtensionProperties &it : vk_extensions)
              extensions.emplace_back(extension(it));
          }

          /// \brief Create a null physical_device object
          /// \warning you can't use the returned object to query things to vulkan
          static physical_device create_null_physical_device()
          {
            return physical_device();
          }

        public:
          /// \brief Return the name of the device
          std::string get_name() const
          {
            return properties.deviceName;
          }

          /// \brief Return the vendor ID of the device
          uint32_t get_vendor_id() const
          {
            return properties.vendorID;
          }

          /// \brief Return the vulkan API version supported by the driver
          uint32_t get_vulkan_api_version() const
          {
            return properties.apiVersion;
          }

          /// \brief Return the driver version
          uint32_t get_driver_version() const
          {
            return properties.driverVersion;
          }

          /// \brief Return the type of the device
          VkPhysicalDeviceType get_type() const
          {
            return properties.deviceType;
          }

          /// \brief Return the pipeline cache UUID (that is 16 byte long)
          const uint8_t* get_pipeline_cache_uuid() const
          {
            return properties.pipelineCacheUUID;
          }

          /// \brief Return the limits of the device
          /// (this is a vulkan structure)
          const VkPhysicalDeviceLimits &get_limits() const
          {
            return properties.limits;
          }

          /// \brief Return the sparse properties (kind of flags) of the device
          /// (this is a vulkan structure)
          const VkPhysicalDeviceSparseProperties &get_sparse_properties() const
          {
            return properties.sparseProperties;
          }

          /// \brief Return the memory properties of the device
          /// (this is a vulkan structure)
          const VkPhysicalDeviceMemoryProperties &get_memory_property() const
          {
            return memory_properties;
          }

          /// \brief Return an object that defines the vulkan feature the device supports
          /// or not
          /// You have to ask the object if you plan to activate some features and want to check
          /// if the device/driver support that feature
          const device_features &get_features() const
          {
            return features;
          }

          /// \brief Return the list of device (validation) layers for this device
          const std::vector<layer> &get_layers() const
          {
            return layers;
          }

          /// \brief Return the list of device extensions available for this device
          const std::vector<extension> &get_extension() const
          {
            return extensions;
          }

          /// \brief Return the number of queue the device has
          size_t get_queue_count() const
          {
            return queues.size();
          }

          /// \brief Return the properties of a queue
          /// (this is a vulkan structure)
          const VkQueueFamilyProperties &get_queue_properties(size_t index) const
          {
            return queues.at(index);
          }

        public: // advanced
          /// \brief Return the vulkan object that represent the physical device
          /// \note marked as advanced
          VkPhysicalDevice _get_vk_physical_device() const
          {
            return gpu;
          }

        private:
          VkPhysicalDevice gpu;
          VkPhysicalDeviceProperties properties;
          VkPhysicalDeviceMemoryProperties memory_properties;
          device_features features;

          std::vector<VkQueueFamilyProperties> queues;
          std::vector<layer> layers;
          std::vector<extension> extensions;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_683913722694924741_2220927619_DEVICE_HPP__

