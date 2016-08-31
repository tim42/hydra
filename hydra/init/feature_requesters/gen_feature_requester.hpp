//
// file : gen_feature_requester.hpp
// in : file:///home/tim/projects/hydra/hydra/init/feature_requesters/gen_feature_requester.hpp
//
// created by : Timothée Feuillet
// date: Thu Apr 28 2016 11:10:59 GMT+0200 (CEST)
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

#ifndef __N_2530138312652924671_396823700_GEN_FEATURE_REQUESTER_HPP__
#define __N_2530138312652924671_396823700_GEN_FEATURE_REQUESTER_HPP__

#include <list>
#include <string>

#include "../hydra_instance_creator.hpp"
#include "../hydra_device_creator.hpp"
#include "../feature_requester_interface.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief A generic feature requester, for lazy peoples that don't want to
    /// create a specialized class just for this.
    class gen_feature_requester : public feature_requester_interface
    {
      private:
        struct queue_caps
        {
          VkQueueFlagBits flags;
          std::function<bool(size_t, const vk::physical_device &)> checker;
          bool can_be_shared;
        };
      public:
        /// \brief Allow the user to request some GPU features
        vk::device_features gpu_features;

        /// \brief If different of 0, it will be used to request a specific vulkan API
        /// The macro VK_MAKE_VERSION() may help you
        uint32_t vulkan_api_version = 0;

        /// \brief Require for specific instance extensions
        void require_instance_extensions(std::initializer_list<std::string> list)
        {
          instance_extensions.insert(instance_extensions.end(), list.begin(), list.end());
        }
        /// \brief Require for a specific instance extension
        void require_instance_extension(std::string ext)
        {
          instance_extensions.push_back(ext);
        }

        /// \brief Require for specific instance layers
        void require_instance_layers(std::initializer_list<std::string> list)
        {
          instance_layers.insert(instance_extensions.end(), list.begin(), list.end());
        }
        /// \brief Require for a specific instance layer
        void require_instance_layer(std::string layer)
        {
          instance_layers.push_back(layer);
        }

        /// \brief Require for specific device extensions
        void require_device_extensions(std::initializer_list<std::string> list)
        {
          device_extensions.insert(device_extensions.end(), list.begin(), list.end());
        }
        /// \brief Require for a specific device extension
        void require_device_extension(std::string ext)
        {
          device_extensions.push_back(ext);
        }

        /// \brief Require for specific device layers
        void require_device_layers(std::initializer_list<std::string> list)
        {
          device_layers.insert(device_extensions.end(), list.begin(), list.end());
        }
        /// \brief Require for a specific device layer
        void require_device_layer(std::string layer)
        {
          device_layers.push_back(layer);
        }

        /// \brief Require a device with some queue capacities (like graphic, compute, transfer, ...)
        /// \see hydra_device_creator::require_queue_capacity()
        /// \return a pointer to a temporary queue familly id, that will be set to a correct value
        ///         when the device will be created
        temp_queue_familly_id_t *require_queue_capacity(VkQueueFlagBits flags, bool can_be_shared)
        {
          queue_capabilities.push_back(queue_caps
          {
            flags,
            [] (size_t, const vk::physical_device &) -> bool {return true;},
            can_be_shared
          });

          temporary_ids.push_back(0);
          return &temporary_ids.back();
        }

        /// \brief Require a device with some queue capacities (like graphic, compute, transfer, ...)
        /// \see hydra_device_creator::require_queue_capacity()
        temp_queue_familly_id_t *require_queue_capacity(VkQueueFlagBits flags, const std::function<bool(size_t, const vk::physical_device &)> &queue_checker, bool can_be_shared)
        {
          queue_capabilities.push_back(queue_caps
          {
            flags,
            queue_checker,
            can_be_shared
          });

          temporary_ids.push_back(0);
          return &temporary_ids.back();
        }

        /// \brief Reset the state of the requester
        void reset()
        {
          gpu_features.clear();
          vulkan_api_version = 0;
          instance_extensions.clear();
          instance_layers.clear();
          device_extensions.clear();
          device_layers.clear();
          queue_capabilities.clear();
          temporary_ids.clear();
        }

      public: // feature_requester_interface
        void request_instace_layers_extensions(neam::hydra::hydra_instance_creator &hic) override
        {
          for (const std::string &it : instance_extensions)
            hic.require_extension(it);
          for (const std::string &it : instance_layers)
            hic.require_layer(it);
          if (vulkan_api_version > 0)
            hic.set_vulkan_api_version(vulkan_api_version);
        }

        void request_device_layers_extensions(const neam::hydra::vk::instance &, neam::hydra::hydra_device_creator &hdc) override
        {
          hdc.require_features(gpu_features);
          for (const std::string &it : device_extensions)
            hdc.require_extension(it);
          for (const std::string &it : device_layers)
            hdc.require_layer(it);

          size_t i = 0;
          for (const queue_caps &qc : queue_capabilities)
            temporary_ids[i++] = hdc.require_queue_capacity(qc.flags, qc.checker, qc.can_be_shared);
        }

      private:
        std::list<std::string> device_extensions;
        std::list<std::string> device_layers;
        std::list<std::string> instance_extensions;
        std::list<std::string> instance_layers;
        std::list<queue_caps> queue_capabilities;
        std::deque<temp_queue_familly_id_t> temporary_ids;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_2530138312652924671_396823700_GEN_FEATURE_REQUESTER_HPP__

