//
// file : instance_layer.hpp
// in : file:///home/tim/projects/hydra/hydra/init/instance_layer.hpp
//
// created by : Timothée Feuillet
// date: Mon Apr 25 2016 18:28:12 GMT+0200 (CEST)
//
//
// Copyright (c) 2016 Timothée Feuillet
//
// Permission is hereby granted,  free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction,  including without limitation the rights
// to use,  copy, modify,  merge, publish,  distribute, sublicense,  and/or sell
// copies  of the  Software,  and to  permit  persons  to whom  the  Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT  WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING  BUT NOT  LIMITED TO THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR A PARTICULAR  PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS BE  LIABLE  FOR  ANY  CLAIM,  DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef __N_22740186361259331480_807329602_INSTANCE_LAYER_HPP__
#define __N_22740186361259331480_807329602_INSTANCE_LAYER_HPP__

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "../hydra_debug.hpp"
#include "extension.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Describe a vulkan layer (this is just a C++ wrapper around it)
      class layer
      {
        public: // advanced
          /// \brief Constructor for an instance layer
          layer(const VkLayerProperties &_properties) : properties(_properties)
          {
            VkResult res;
            uint32_t instance_extension_count;
            do
            {
              res = check::on_vulkan_error::n_assert_success(
                vkEnumerateInstanceExtensionProperties(properties.layerName, &instance_extension_count, nullptr));

              if (instance_extension_count == 0)
                break;

              extensions.resize(instance_extension_count);
              res = check::on_vulkan_error::n_assert_success(
                vkEnumerateInstanceExtensionProperties(properties.layerName, &instance_extension_count, extensions.data()));
            }
            while (res == VK_INCOMPLETE);
          }

          /// \brief Constructor for a device layer
          layer(const VkLayerProperties &_properties, VkPhysicalDevice gpu) : properties(_properties)
          {
            VkResult res;
            uint32_t instance_extension_count;
            do
            {
              res = check::on_vulkan_error::n_assert_success(
                vkEnumerateDeviceExtensionProperties(gpu, properties.layerName, &instance_extension_count, nullptr));

              if (instance_extension_count == 0)
                break;

              extensions.resize(instance_extension_count);
              res = check::on_vulkan_error::n_assert_success(
                vkEnumerateDeviceExtensionProperties(gpu, properties.layerName, &instance_extension_count, extensions.data()));
            }
            while (res == VK_INCOMPLETE);
          }

        public:
          /// \brief Return the layer name
          std::string get_name() const
          {
            return properties.layerName;
          }

          /// \brief Return the layer description (if any)
          std::string get_description() const
          {
            return properties.description;
          }

          /// \brief Return the version of the layer
          uint32_t get_revision() const
          {
            return properties.specVersion;
          }

          /// \brief Return the version of the vulkan API used by the layer
          uint32_t get_vulkan_version() const
          {
            return properties.implementationVersion;
          }

          /// \brief Return the number of extension this layer has
          size_t get_extension_count() const
          {
            return extensions.size();
          }

          /// \brief Get an extension
          extension operator[] (size_t index) const
          {
            return extension(extensions[index]);
          }

        private:
          VkLayerProperties properties;
          std::vector<VkExtensionProperties> extensions;

          friend class hydra_instance_creator;
          friend class physical_device;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_22740186361259331480_807329602_INSTANCE_LAYER_HPP__

