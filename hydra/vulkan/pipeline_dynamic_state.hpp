//
// file : pipeline_dynamic_state.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/pipeline_dynamic_state.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 13 2016 18:47:48 GMT+0200 (CEST)
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

#ifndef __N_1976322101225802682_168463513_PIPELINE_DYNAMIC_STATE_HPP__
#define __N_1976322101225802682_168463513_PIPELINE_DYNAMIC_STATE_HPP__

#include <initializer_list>
#include <vector>
#include <vulkan/vulkan.h>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkPipelineDynamicStateCreateInfo
      class pipeline_dynamic_state
      {
        public:
          pipeline_dynamic_state()
            : vk_pdsci
          {
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0,
            0, nullptr
          }
          {}

          pipeline_dynamic_state(std::initializer_list<VkDynamicState> ds_list)
            : vk_pdsci
          {
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0,
            0, nullptr
          }, vk_dyn_states(ds_list)
          {
            vk_pdsci.dynamicStateCount = vk_dyn_states.size();
            vk_pdsci.pDynamicStates = vk_dyn_states.data();
          }

          /// \brief Copy constr.
          pipeline_dynamic_state(const pipeline_dynamic_state &o) : vk_pdsci(o.vk_pdsci), vk_dyn_states(o.vk_dyn_states) {}
          /// \brief Copy op.
          pipeline_dynamic_state &operator = (const pipeline_dynamic_state &o)
          {
            vk_pdsci = o.vk_pdsci;
            vk_dyn_states = o.vk_dyn_states;
            return *this;
          }

          /// \brief Add a dynamic state
          void add(VkDynamicState ds)
          {
            vk_dyn_states.push_back(ds);
            vk_pdsci.dynamicStateCount = vk_dyn_states.size();
            vk_pdsci.pDynamicStates = vk_dyn_states.data();
          }

          /// \brief Add a list of dynamic state
          void add(std::initializer_list<VkDynamicState> ds_list)
          {
            vk_dyn_states.insert(vk_dyn_states.end(), ds_list.begin(), ds_list.end());
            vk_pdsci.dynamicStateCount = vk_dyn_states.size();
            vk_pdsci.pDynamicStates = vk_dyn_states.data();
          }

          void remove(VkDynamicState ds)
          {
            vk_dyn_states.erase(std::remove(vk_dyn_states.begin(), vk_dyn_states.end(), ds), vk_dyn_states.end());
            vk_pdsci.dynamicStateCount = vk_dyn_states.size();
            vk_pdsci.pDynamicStates = vk_dyn_states.data();
          }

          /// \brief Remove every single dynamic states
          void clear()
          {
            vk_dyn_states.clear();
            vk_pdsci.dynamicStateCount = 0;
            vk_pdsci.pDynamicStates = nullptr;
          }

          /// \brief Check if the class contains any dynamic states
          bool has_dynamic_states() const
          {
            return !vk_dyn_states.empty();
          }

        public: // advanced
          /// \brief Yields a const reference to VkPipelineDynamicStateCreateInfo
          operator const VkPipelineDynamicStateCreateInfo &() const  { return vk_pdsci; }

        private:
          VkPipelineDynamicStateCreateInfo vk_pdsci;
          std::vector<VkDynamicState> vk_dyn_states;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_1976322101225802682_168463513_PIPELINE_DYNAMIC_STATE_HPP__

