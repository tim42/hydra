//
// file : descriptor_set_layout_binding.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/descriptor_set_layout_binding.hpp
//
// created by : Timothée Feuillet
// date: Tue Aug 30 2016 14:44:56 GMT+0200 (CEST)
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

#ifndef __N_15937268572547121509_461228137_DESCRIPTOR_SET_LAYOUT_BINDING_HPP__
#define __N_15937268572547121509_461228137_DESCRIPTOR_SET_LAYOUT_BINDING_HPP__

#include <vector>

#include <vulkan/vulkan.h>
#include "sampler.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkDescriptorSetLayoutBinding
      class descriptor_set_layout_binding
      {
        public:
          /// \brief An empty binding
          descriptor_set_layout_binding(uint32_t binding) : descriptor_set_layout_binding(binding, (VkDescriptorType)0, 0, 0) {}

          /// \brief Construct the binding
          descriptor_set_layout_binding(uint32_t binding, VkDescriptorType descriptor_type, uint32_t descriptor_count = 1, VkShaderStageFlags stages = VK_SHADER_STAGE_ALL)
            : vk_dslb
            {
              binding, descriptor_type, descriptor_count, stages, nullptr
            }
          {
          }
          /// \brief Construct the binding
          descriptor_set_layout_binding(uint32_t binding, VkDescriptorType descriptor_type, uint32_t descriptor_count, VkShaderStageFlags stages, const sampler& immutable_sampler)
            : vk_dslb
            {
              binding, descriptor_type, descriptor_count, stages, immutable_sampler._get_vk_sampler_ptr()
            }
          {
          }
//           descriptor_set_layout_binding(uint32_t binding, VkDescriptorType descriptor_type, const std::vector<sampler *> &sampler_vct, VkShaderStageFlags stages = VK_SHADER_STAGE_ALL);

          descriptor_set_layout_binding(const descriptor_set_layout_binding &o) : vk_dslb(o.vk_dslb) {}
          descriptor_set_layout_binding &operator = (const descriptor_set_layout_binding &o) {vk_dslb = o.vk_dslb; return *this; }
          descriptor_set_layout_binding(const VkDescriptorSetLayoutBinding &o) : vk_dslb(o) {}
          descriptor_set_layout_binding &operator = (const VkDescriptorSetLayoutBinding &o) {vk_dslb = o; return *this; }

          // TODO: the remaining of the interface

        public: // advanced
          /// \brief Yields a const reference to a VkDescriptorSetLayoutBinding
          operator const VkDescriptorSetLayoutBinding &() const { return vk_dslb; }
        private:
          VkDescriptorSetLayoutBinding vk_dslb;
          std::vector<int> _TO_REMOVE_;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam


#endif // __N_15937268572547121509_461228137_DESCRIPTOR_SET_LAYOUT_BINDING_HPP__

