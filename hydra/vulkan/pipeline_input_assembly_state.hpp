//
// file : pipeline_input_assembly_state.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/pipeline_input_assembly_state.hpp
//
// created by : Timothée Feuillet
// date: Sun Aug 14 2016 14:06:14 GMT+0200 (CEST)
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

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkPipelineInputAssemblyStateCreateInfo
      class pipeline_input_assembly_state
      {
        public:
          pipeline_input_assembly_state()
            : vk_piasci
          {
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE
          }
          {}

          /// \brief Create the VkPipelineInputAssemblyStateCreateInfo from two parameters
          /// \param allow_restart enable (or disable) a special index (and thus is only for indexed draws)
          ///                      that restarts the primitive. The index is either 0xFFFFFFFF for 32bit indexes or 0xFFFF for 16bit indexes.
          pipeline_input_assembly_state(VkPrimitiveTopology topology, bool allow_restart = false)
            : vk_piasci
          {
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0,
            topology,
            (VkBool32)(allow_restart ? VK_TRUE : VK_FALSE)
          }
          {
          }

          pipeline_input_assembly_state(const pipeline_input_assembly_state &o) : vk_piasci(o.vk_piasci) {}
          pipeline_input_assembly_state(const VkPipelineInputAssemblyStateCreateInfo &o) : vk_piasci(o) {}
          pipeline_input_assembly_state &operator = (const pipeline_input_assembly_state &o) { vk_piasci = o.vk_piasci; return *this; }
          pipeline_input_assembly_state &operator = (const VkPipelineInputAssemblyStateCreateInfo &o) { vk_piasci = o; return *this; }

          /// \brief Set the topology of the geometry
          pipeline_input_assembly_state &set_topology(VkPrimitiveTopology topology) { vk_piasci.topology = topology; return *this; }
          /// \brief Return the topology
          VkPrimitiveTopology get_topology() const { return vk_piasci.topology; }

          /// \brief Enable or disable primitive restart
          void enable_primitive_restart(bool enable) { vk_piasci.primitiveRestartEnable = (enable ? VK_TRUE : VK_FALSE); }
          /// \brief Return whether or not the primitive restart is enabled
          bool is_primitive_restart_enabled() const { return vk_piasci.primitiveRestartEnable == VK_TRUE; }

        public: // advanced
          /// \brief Yield a const reference to VkPipelineInputAssemblyStateCreateInfo
          operator const VkPipelineInputAssemblyStateCreateInfo &() const { return vk_piasci; }
        private:
          VkPipelineInputAssemblyStateCreateInfo vk_piasci;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



