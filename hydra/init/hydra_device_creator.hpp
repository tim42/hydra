//
// file : hydra_device_creator.hpp
// in : file:///home/tim/projects/hydra/hydra/init/hydra_device_creator.hpp
//
// created by : Timothée Feuillet
// date: Wed Apr 27 2016 11:35:49 GMT+0200 (CEST)
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

#ifndef __N_7880295321249221110_2854922646_HYDRA_DEVICE_CREATOR_HPP__
#define __N_7880295321249221110_2854922646_HYDRA_DEVICE_CREATOR_HPP__

#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <functional>

#include <vulkan/vulkan.h>

#include "../vulkan/device_features.hpp"
#include "../vulkan/instance.hpp"
#include "../vulkan/device.hpp"
#include "feature_requester_interface.hpp"

#include "../hydra_types.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief A helper to create vulkan logical devices.
    /// You should get and instance of this class via the method
    /// vk::instance::get_device_creator()
    class hydra_device_creator
    {
      private:
        struct queue_caps
        {
          VkQueueFlagBits flags;
          std::function<bool(size_t, const vk::physical_device &)> checker;
          temp_queue_familly_id_t id;
        };

      public: // advanced
        hydra_device_creator(const vk::instance &_instance) : instance(_instance) {}

      public:
        /// \brief Check if a device is compatible with the requirements of the
        /// aplication
        bool check_device(const vk::physical_device &gpu) const
        {
          // check features
          if (requested_features.check_against(gpu.get_features()) == false)
            return false;

          // check limits
          const VkPhysicalDeviceLimits &device_limits = gpu.get_limits();
          for (auto &checker : device_limit_checkers)
          {
            if (!checker(device_limits))
              return false;
          }

          // check layers (this not optimal)
          for (const std::string &layer_name : device_layers)
          {
            bool ok = false;
            for (const vk::layer &it : gpu.get_layers())
            {
              if (it.get_name() == layer_name)
              {
                ok = true;
                break;
              }
            }
            if (!ok)
              return false;
          }

          // check extensions (this not optimal)
          for (const std::string &extension_name : device_extensions)
          {
            bool ok = false;
            for (const vk::extension &it : gpu.get_extension())
            {
              if (it.get_name() == extension_name)
              {
                ok = true;
                break;
              }
            }
            if (!ok)
              return false;
          }

          // init queues
          std::vector<size_t> queue_consumption;
          for (size_t i = 0; i < gpu.get_queue_count(); ++i)
            queue_consumption.push_back(0);

          // shared queues
          for (const queue_caps &it : shared_queue_caps)
          {
            bool found = false;
            for (size_t i = 0; i < gpu.get_queue_count(); ++i)
            {
              const VkQueueFamilyProperties &qfp = gpu.get_queue_properties(i);
              if ((qfp.queueFlags & it.flags) == it.flags && it.checker(i, gpu)
                  && qfp.queueCount > queue_consumption[i])
              {
                ++queue_consumption[i];
                found = true;
                break;
              }
            }
            if (!found)
              return false;
          }
          // non-shared queues
          for (const queue_caps &it : unique_queue_caps)
          {
            bool found = false;
            for (size_t i = 0; i < gpu.get_queue_count(); ++i)
            {
              const VkQueueFamilyProperties &qfp = gpu.get_queue_properties(i);
              if ((qfp.queueFlags & it.flags) == it.flags && it.checker(i, gpu)
                  && qfp.queueCount > queue_consumption[i])
              {
                ++queue_consumption[i];
                found = true;
                break;
              }
            }
            if (!found)
              return false;
          }

          // done, everything matched
          return true;
        }

        /// \brief Return the list of devices that are compatible with the
        /// requirements of the application
        /// \note the result is a vector of device sorted from discrete/dedicated to integrated to others
        std::vector<vk::physical_device> filter_devices() const
        {
          std::vector<vk::physical_device> result;
          result.reserve(instance.get_device_count());

          for (size_t i = 0; i < instance.get_device_count(); ++i)
          {
            const vk::physical_device &gpu = instance.get_device(i);
            if (check_device(gpu) == true)
              result.push_back(gpu);
          }

          std::map<VkPhysicalDeviceType, int> prios
          ({
            {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 5},
            {VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, 4},

            {VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU, 2},
            {VK_PHYSICAL_DEVICE_TYPE_CPU, 1},
            {VK_PHYSICAL_DEVICE_TYPE_OTHER, 0},
          });

          // sort the result vector
          std::sort(result.begin(), result.end(), [&prios](const vk::physical_device &gpu1, const vk::physical_device &gpu2) -> bool
          {
            return prios[gpu1.get_type()] > prios[gpu2.get_type()];
          });

          return result;
        }

        /// \brief Require some features (like tesselation or geometry shaders, sparse binding, ...)
        void require_features(const vk::device_features &features)
        {
          requested_features |= features;
        }

        /// \brief Allow the user to require a limit to be higher than, lower than, equals to, ...
        /// an arbitrary value.
        /// Usage exemple:
        /// \code
        /// dev_creator.require_limits([](const VkPhysicalDeviceLimits &gpu_limits)
        /// {
        ///   return gpu_limits.maxImageDimension2D >= 4096;
        /// });
        /// \endcode
        void require_limits(const std::function<bool(const VkPhysicalDeviceLimits&)> &limit_checker)
        {
          device_limit_checkers.push_back(limit_checker);
        }

        /// \brief Require a device with some queue capacities (like graphic, compute, transfer, ...)
        /// \param flags The flags the queue must have
        /// \param can_be_shared Indicate whether or not you allow the queue to be shared with another component
        ///                      (coud be usefull if you require a queue with compute caps and and another queue with graphic caps
        ///                       and there's a queue with both capabilities).
        /// If you require some more specific checks than just flags bits (like presenting support) you can use
        /// the other require_queue_capacity() method
        ///
        /// The solver is pretty dumb. So if you request a lot of non-shared queues it will probably not find the ideal
        /// solution
        temp_queue_familly_id_t require_queue_capacity(VkQueueFlagBits flags, bool can_be_shared)
        {
          return require_queue_capacity(flags, [](size_t, const vk::physical_device &) {return true;}, can_be_shared);
        }

        /// \brief Require a device with some queue capacities (like graphic, compute, transfer, ...)
        /// \param flags The flags the queue must have
        /// \param queue_checker An additional condition to accept the queue family (first parameter is the queue family index, second the gpu)
        /// \param can_be_shared Indicate whether or not you allow the queue to be shared with another component
        ///                      (coud be usefull if you require a queue with compute caps and and another queue with graphic caps
        ///                       and there's a queue with both capabilities).
        /// The solver is pretty dumb. So if you request a lot of non-shared queues it will probably not find the ideal
        /// solution
        temp_queue_familly_id_t require_queue_capacity(VkQueueFlagBits flags, const std::function<bool(size_t, const vk::physical_device &)> &queue_checker, bool can_be_shared)
        {
          temp_queue_familly_id_t id = unique_queue_caps.size() + shared_queue_caps.size(); // this is a unique ID

          if (can_be_shared)
            shared_queue_caps.push_back(queue_caps{flags, queue_checker, id});
          else
            unique_queue_caps.push_back(queue_caps{flags, queue_checker, id});

          return id;
        }

        /// \brief Require an extension for the instance
        /// \throw neam::hydra::exception if the extension does not exists
        void require_extension(const std::string &extension_name)
        {
          if (!device_extensions.count(extension_name))
            device_extensions.emplace(extension_name);
        }
        /// \brief Require a list of extension for the instance
        /// \throw neam::hydra::exception if any of those extension does not exists
        void require_extensions(std::initializer_list<std::string> extension_names)
        {
          for (const auto &it : extension_names)
            require_extension(it);
        }

        /// \brief Require a layer for the instance
        /// \throw neam::hydra::exception if the layer does not exists
        void require_layer(const std::string &layer_name)
        {
          if (!device_layers.count(layer_name))
            device_layers.emplace(layer_name);
        }

        /// \brief Require a list of layer for the instance
        /// \throw neam::hydra::exception if any of those layer does not exists
        void require_layers(std::initializer_list<std::string> layer_names)
        {
          for (const auto &it : layer_names)
            require_layer(it);
        }

        /// \brief Let a requester ask for a list of extension, layers, features, ...
        /// A requester \e should have the following method:
        ///   void request_device_layers_extensions(const hydra_vulkan_instance &, hydra_device_creator &)
        /// \see feature_requester_interface
        template<typename RequesterT>
        void require(RequesterT requester)
        {
          requester.request_device_layers_extensions(instance, *this);
        }

        /// \brief Like require<>, but let you work with an abstract type
        void require(feature_requester_interface *requester)
        {
          require<feature_requester_interface &>(*requester);
        }
        /// \brief Like require<>, but let you work with an abstract type
        void require(feature_requester_interface &requester)
        {
          require<feature_requester_interface &>(requester);
        }

        /// \brief Create the device wrapper
        vk::device create_device(vk::physical_device &gpu) const
        {
          // please use a device from filter_devices
          check::on_vulkan_error::n_assert(check_device(gpu) == true, "the selected device does not fulfill the requirements of the application");

          // init queues
          std::vector<size_t> queue_consumption;
          for (size_t i = 0; i < gpu.get_queue_count(); ++i)
            queue_consumption.push_back(0);

          // this maps the temporary familly id to a [queue familly index ; #queue ]
          std::map<temp_queue_familly_id_t, std::pair<size_t, size_t>> id_to_fq;

          // shared queues
          for (const queue_caps &it : shared_queue_caps)
          {
            for (size_t i = 0; i < gpu.get_queue_count(); ++i)
            {
              const VkQueueFamilyProperties &qfp = gpu.get_queue_properties(i);
              if ((qfp.queueFlags & it.flags) == it.flags && it.checker(i, gpu)
                  && qfp.queueCount > queue_consumption[i])
              {
                id_to_fq[it.id] = std::make_pair(i, queue_consumption[i]);
                ++queue_consumption[i];
                break;
              }
            }
          }
          // non-shared queues
          for (const queue_caps &it : unique_queue_caps)
          {
            for (size_t i = 0; i < gpu.get_queue_count(); ++i)
            {
              const VkQueueFamilyProperties &qfp = gpu.get_queue_properties(i);
              if ((qfp.queueFlags & it.flags) == it.flags && it.checker(i, gpu)
                  && qfp.queueCount > queue_consumption[i])
              {
                id_to_fq[it.id] = std::make_pair(i, queue_consumption[i]);
                ++queue_consumption[i];
                break;
              }
            }
          }

          // create the queue create info array
          std::vector<VkDeviceQueueCreateInfo> queue_info;
          queue_info.resize(gpu.get_queue_count());
          std::vector<std::vector<float>> queue_prios;
          queue_prios.resize(gpu.get_queue_count());

          uint32_t queue_info_count = 0;
          for (size_t i = 0; i < gpu.get_queue_count(); ++i)
          {
            if (queue_consumption[i] == 0) // no queue here
              continue;

            queue_prios[i].resize(queue_consumption[i]);
            queue_info[queue_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[queue_info_count].flags = 0;
            queue_info[queue_info_count].pNext = nullptr;
            queue_info[queue_info_count].queueCount = queue_consumption[i];
            queue_info[queue_info_count].queueFamilyIndex = i;
            queue_info[queue_info_count].pQueuePriorities = queue_prios[i].data();

            ++queue_info_count;
          }

          check::on_vulkan_error::n_assert(queue_info_count != 0, "you have to request at least one queue at the device creation");

          // extensions and layers:
          std::vector<const char *> device_layers_c_str;
          device_layers_c_str.reserve(device_layers.size());
          for (const std::string &it : device_layers)
            device_layers_c_str.push_back(it.c_str());

          std::vector<const char *> device_extensions_c_str;
          device_extensions_c_str.reserve(device_extensions.size());
          for (const std::string &it : device_extensions)
            device_extensions_c_str.push_back(it.c_str());

          // features
          VkPhysicalDeviceFeatures features = requested_features._to_vulkan();

          // create the device create info struct
          VkDeviceCreateInfo device_info;
          device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
          device_info.pNext = nullptr;
          device_info.flags = 0;
          device_info.queueCreateInfoCount = queue_info_count;
          device_info.pQueueCreateInfos = queue_info.data();
          device_info.enabledExtensionCount = device_extensions_c_str.size();
          device_info.ppEnabledExtensionNames = device_extensions_c_str.data();
          device_info.enabledLayerCount = device_layers_c_str.size();
          device_info.ppEnabledLayerNames = device_layers_c_str.data();
          device_info.pEnabledFeatures = &features;

          VkDevice device;
          check::on_vulkan_error::n_throw_exception(vkCreateDevice(gpu._get_vulkan_physical_device(), &device_info, nullptr, &device));

          return vk::device(device, gpu, id_to_fq);
        }

      private:
        const vk::instance &instance;

        std::set<std::string> device_layers;
        std::set<std::string> device_extensions;
        std::vector<std::function<bool(const VkPhysicalDeviceLimits&)>> device_limit_checkers;

        std::vector<queue_caps> shared_queue_caps;
        std::vector<queue_caps> unique_queue_caps;

        vk::device_features requested_features;
    };



    // implementation of the method from hydra_vulkan_instance
    hydra_device_creator vk::instance::get_device_creator() const
    {
      return hydra_device_creator(*this);
    }
  } // namespace hydra
} // namespace neam

#endif // __N_7880295321249221110_2854922646_HYDRA_DEVICE_CREATOR_HPP__

