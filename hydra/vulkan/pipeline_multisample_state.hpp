//
// file : pipeline_multisample_state.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/pipeline_multisample_state.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 13 2016 13:55:53 GMT+0200 (CEST)
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

#ifndef __N_16252281432830929027_2391312862_PIPELINE_MULTISAMPLE_STATE_HPP__
#define __N_16252281432830929027_2391312862_PIPELINE_MULTISAMPLE_STATE_HPP__

#include <vulkan/vulkan.h>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps VkPipelineMultisampleStateCreateInfo
      class pipeline_multisample_state
      {
        public:
          /// \brief Default constructor disables multisampling
          pipeline_multisample_state()
            : vk_pmsci
          {
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            nullptr,
            0,
            VK_SAMPLE_COUNT_1_BIT, // rasterizationSamples
            VK_FALSE, // sampleShadingEnable
            1.f, // minSampleShading
            nullptr, // pSampleMask
            VK_FALSE, // alphaToCoverageEnable
            VK_FALSE, // alphaToOneEnable
          }
          {
          }

          /// \brief Construct a multisample state object with multisampling activated
          explicit pipeline_multisample_state(VkSampleCountFlagBits samples)
            : vk_pmsci
          {
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            nullptr,
            0,
            samples, // rasterizationSamples
            VK_FALSE, // sampleShadingEnable
            0.f, // minSampleShading
            nullptr, // pSampleMask
            VK_FALSE, // alphaToCoverageEnable
            VK_FALSE, // alphaToOneEnable
          }
          {
          }

          /// \brief Copy constructor
          pipeline_multisample_state(const pipeline_multisample_state &o) : vk_pmsci(o.vk_pmsci) {}
          /// \brief Copy operator
          pipeline_multisample_state &operator = (const pipeline_multisample_state &o) { vk_pmsci = o.vk_pmsci; return *this; }
          /// \brief Copy constructor
          pipeline_multisample_state(const VkPipelineMultisampleStateCreateInfo &o) : vk_pmsci(o)
          {
            vk_pmsci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            vk_pmsci.pNext = nullptr;
            vk_pmsci.flags = 0;
          }
          /// \brief Copy operator
          pipeline_multisample_state &operator = (const VkPipelineMultisampleStateCreateInfo &o)
          {
            vk_pmsci = o;
            vk_pmsci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            vk_pmsci.pNext = nullptr;
            vk_pmsci.flags = 0;
            return *this;
          }

          /// \brief Disable the multisampling
          void disable_multisampling()
          {
            vk_pmsci.minSampleShading = 1.f; // FIXME: does that mean anything ??
            vk_pmsci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
          }

          /// \brief Set the multisampling sample count (enable multisampling if samples != VK_SAMPLE_COUNT_1_BIT)
          void set_sample_count(VkSampleCountFlagBits samples)
          {
            vk_pmsci.minSampleShading = samples == VK_SAMPLE_COUNT_1_BIT ? 0.f : 1.f; // FIXME: does that mean anything ??
            vk_pmsci.rasterizationSamples = samples;
          }

          /// \brief Return the multisampling sample count
          VkSampleCountFlagBits get_sample_count() const
          {
            return vk_pmsci.rasterizationSamples;
          }

        public: // advanced
          /// \brief Yield a VkPipelineMultisampleStateCreateInfo
          operator const VkPipelineMultisampleStateCreateInfo &() { return vk_pmsci; }

        private:
          VkPipelineMultisampleStateCreateInfo vk_pmsci;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_16252281432830929027_2391312862_PIPELINE_MULTISAMPLE_STATE_HPP__

