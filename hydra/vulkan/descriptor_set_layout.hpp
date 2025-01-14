//
// file : descriptor_set_layout.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/descriptor_set_layout.hpp
//
// created by : Timothée Feuillet
// date: Tue Aug 30 2016 14:17:32 GMT+0200 (CEST)
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

#include "../hydra_debug.hpp"

#include "device.hpp"
#include "descriptor_set_layout_binding.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkDescriptorSetLayout
      class descriptor_set_layout
      {
        public: // advanced
          descriptor_set_layout(device &_dev, const VkDescriptorSetLayoutCreateInfo &create_info)
            : dev(_dev)
          {
            check::on_vulkan_error::n_assert_success(dev._vkCreateDescriptorSetLayout(&create_info, nullptr, &vk_ds_layout));
          }

          descriptor_set_layout(device &_dev, VkDescriptorSetLayout _vk_ds_layout) : dev(_dev), vk_ds_layout(_vk_ds_layout) {}

        public:
          /// \brief Create the layout from a set of bindings
          /// \note You MUST provide every binding (even if you don't use some of them, you can't provide binding 0 and binding 2)
          descriptor_set_layout(device &_dev, const std::vector<descriptor_set_layout_binding> &bindings, VkDescriptorSetLayoutCreateFlags flags = 0)
            : dev(_dev)
          {
            std::vector<VkDescriptorSetLayoutBinding> vk_bindings_vct;
            std::vector<VkDescriptorBindingFlags> vk_bindings_flags_vct;
            vk_bindings_vct.reserve(bindings.size());
            vk_bindings_flags_vct.reserve(bindings.size());
            for (const descriptor_set_layout_binding &it : bindings)
            {
              vk_bindings_vct.push_back(it);
              vk_bindings_flags_vct.push_back(it.get_binding_flag());
            }
            VkDescriptorSetLayoutBindingFlagsCreateInfo flags_create_info
            {
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO, nullptr,
              (uint32_t)vk_bindings_flags_vct.size(), vk_bindings_flags_vct.data()
            };
            VkDescriptorSetLayoutCreateInfo create_info
            {
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, &flags_create_info, flags,
              (uint32_t)vk_bindings_vct.size(), vk_bindings_vct.data()
            };
            check::on_vulkan_error::n_assert_success(dev._vkCreateDescriptorSetLayout(&create_info, nullptr, &vk_ds_layout));
          }

          descriptor_set_layout(descriptor_set_layout &&o) : dev(o.dev), vk_ds_layout(o.vk_ds_layout) {o.vk_ds_layout = nullptr;}
          descriptor_set_layout& operator = (descriptor_set_layout &&o)
          {
            if (&o == this)
              return *this;

            check::on_vulkan_error::n_assert(&dev == &o.dev, "Cannot move-assign a descriptor_set_layout to another one from another device");
            if (vk_ds_layout)
              dev._vkDestroyDescriptorSetLayout(vk_ds_layout, nullptr);
            vk_ds_layout = o.vk_ds_layout;
            o.vk_ds_layout = nullptr;
            return *this;
          }

          ~descriptor_set_layout()
          {
            if (vk_ds_layout)
              dev._vkDestroyDescriptorSetLayout(vk_ds_layout, nullptr);
          }

        public: // advanced
          /// \brief Return the VkDescriptorSetLayout
          VkDescriptorSetLayout _get_vk_descriptor_set_layout() const { return vk_ds_layout; }

          void _set_debug_name(const std::string& name)
          {
            dev._set_object_debug_name((uint64_t)vk_ds_layout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, name);
          }
        private:
          device &dev;
          VkDescriptorSetLayout vk_ds_layout;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam




