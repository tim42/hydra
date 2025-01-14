//
// file : pipeline_layout.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/pipeline_layout.hpp
//
// created by : Timothée Feuillet
// date: Sun Aug 14 2016 13:43:28 GMT+0200 (CEST)
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

#include "descriptor_set_layout.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkPipelineLayout and its creation
      /// \note This class is far from being complete
      class pipeline_layout
      {
        public: // advanced
        public:
          pipeline_layout(device& _dev, VkPipelineLayout _playout) : dev(_dev), vk_playout(_playout) {}
          /// \brief Create an empty pipeline layout
          explicit pipeline_layout(device &_dev)
           : dev(_dev)
          {
            VkPipelineLayoutCreateInfo plci
            {
              VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0,
              0, nullptr,
              0, nullptr
            };
            check::on_vulkan_error::n_assert_success(dev._vkCreatePipelineLayout(&plci, nullptr, &vk_playout));
          }

          /// \brief Create a pipeline layout (without push constants)
          pipeline_layout(device &_dev, const std::vector<descriptor_set_layout *> &dsl_vct, const std::vector<VkPushConstantRange> &pc_range_vct = {})
           : dev(_dev)
          {
            std::vector<VkDescriptorSetLayout> vk_dsl_vct;
            vk_dsl_vct.reserve(dsl_vct.size());
            for (descriptor_set_layout *it : dsl_vct)
              vk_dsl_vct.push_back(it->_get_vk_descriptor_set_layout());
            VkPipelineLayoutCreateInfo plci
            {
              VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0,
              (uint32_t)vk_dsl_vct.size(), vk_dsl_vct.data(),
              (uint32_t)pc_range_vct.size(), pc_range_vct.data()
            };
            check::on_vulkan_error::n_assert_success(dev._vkCreatePipelineLayout(&plci, nullptr, &vk_playout));
          }

          pipeline_layout(pipeline_layout&& o) : dev(o.dev), vk_playout(o.vk_playout) { o.vk_playout = nullptr; }
          pipeline_layout& operator = (pipeline_layout&& o)
          {
            if (&o == this)
              return *this;

            check::on_vulkan_error::n_assert(&dev == &o.dev, "Cannot move-assign a pipeline layout to a pipeline layout from another device");
            if (vk_playout)
              dev._vkDestroyPipelineLayout(vk_playout, nullptr);
            vk_playout = o.vk_playout;
            o.vk_playout = nullptr;
            return *this;
          }

          ~pipeline_layout()
          {
            if (vk_playout)
              dev._vkDestroyPipelineLayout(vk_playout, nullptr);
          }

        public: // advanced
          /// \brief Return the VkPipelineLayout
          VkPipelineLayout _get_vk_pipeline_layout() const { return vk_playout; }

          void _set_debug_name(const std::string& name)
          {
            dev._set_object_debug_name((uint64_t)vk_playout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, name);
          }
        private:
          device &dev;
          VkPipelineLayout vk_playout = nullptr;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



