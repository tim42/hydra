//
// file : pipeline_vertex_input_state.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/pipeline_vertex_input_state.hpp
//
// created by : Timothée Feuillet
// date: Sun Aug 14 2016 13:57:45 GMT+0200 (CEST)
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

#ifndef __N_18844283671003224974_3191428268_PIPELINE_VERTEX_INPUT_STATE_HPP__
#define __N_18844283671003224974_3191428268_PIPELINE_VERTEX_INPUT_STATE_HPP__

#include <vulkan/vulkan.h>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkPipelineVertexInputStateCreateInfo
      /// \note This class is far from being complete.
      class pipeline_vertex_input_state
      {
        public:
          /// \brief Create an empty vertex input state
          pipeline_vertex_input_state()
            : vk_pvisci
          {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0,
            0, nullptr,
            0, nullptr
          }
          {
          }

          pipeline_vertex_input_state(const pipeline_vertex_input_state &o) : vk_pvisci(o.vk_pvisci), vk_binding_desc(o.vk_binding_desc), vk_attr_desc(o.vk_attr_desc) {}
          pipeline_vertex_input_state &operator = (const pipeline_vertex_input_state &o)
          {
            vk_pvisci = o.vk_pvisci;
            vk_binding_desc = o.vk_binding_desc;
            vk_attr_desc = o.vk_attr_desc;
            vk_pvisci.vertexBindingDescriptionCount = vk_binding_desc.size();
            vk_pvisci.pVertexBindingDescriptions = vk_binding_desc.data();
            vk_pvisci.vertexAttributeDescriptionCount = vk_attr_desc.size();
            vk_pvisci.pVertexAttributeDescriptions = vk_attr_desc.data();
            return *this;
          }

          /// \brief Add a binding description
          pipeline_vertex_input_state &add_binding_description(uint32_t binding, uint32_t stride, VkVertexInputRate input_rate)
          {
            return add_binding_description(VkVertexInputBindingDescription
            {
              binding, stride, input_rate
            });
          }

          /// \brief Add an attribute description
          pipeline_vertex_input_state &add_attribute_description(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset)
          {
            return add_attribute_description(VkVertexInputAttributeDescription
            {
              location, binding, format, offset
            });
          }

          /// \brief Add a binding description, directly from the vulkan structure
          pipeline_vertex_input_state &add_binding_description(const VkVertexInputBindingDescription &desc)
          {
            vk_binding_desc.push_back(desc);
            vk_pvisci.vertexBindingDescriptionCount = vk_binding_desc.size();
            vk_pvisci.pVertexBindingDescriptions = vk_binding_desc.data();
            return *this;
          }

          /// \brief Add an attribute description, directly from the vulkan structure
          pipeline_vertex_input_state &add_attribute_description(const VkVertexInputAttributeDescription &desc)
          {
            vk_attr_desc.push_back(desc);
            vk_pvisci.vertexAttributeDescriptionCount = vk_attr_desc.size();
            vk_pvisci.pVertexAttributeDescriptions = vk_attr_desc.data();
            return *this;
          }

          /// \brief Return the number of binding
          size_t get_binding_count() const { return vk_binding_desc.size(); }

        public: // advanced
          /// \brief Yield a const reference to VkPipelineVertexInputStateCreateInfo
          operator const VkPipelineVertexInputStateCreateInfo &() const { return vk_pvisci; }
        private:
          VkPipelineVertexInputStateCreateInfo vk_pvisci;
          std::vector<VkVertexInputBindingDescription> vk_binding_desc;
          std::vector<VkVertexInputAttributeDescription> vk_attr_desc;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_18844283671003224974_3191428268_PIPELINE_VERTEX_INPUT_STATE_HPP__

