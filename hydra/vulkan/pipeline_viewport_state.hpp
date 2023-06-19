//
// file : pipeline_viewport_state.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/pipeline_viewport_state.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 13 2016 11:29:53 GMT+0200 (CEST)
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


#include <vector>
#include <deque>
#include <initializer_list>
#include <vulkan/vulkan.h>

#include "viewport.hpp"
#include "rect2D.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkPipelineViewportStateCreateInfo
      /// \note It only works with pointers (pointers to viewports & to rect2D)
      ///       You are responsible to keep things alive the time this class exists / a call to clear() is made
      /// \note A good thing to avoid stupid leaks / crashes it to use the swapchain
      ///       rect2D & viewport
      class pipeline_viewport_state
      {
        public:
          pipeline_viewport_state() { refresh(); }

          /// \brief Construct the pipeline_viewport_state from a viewport list and a scissors list
          /// \note You are responsible to keep things alive the time this class exists / a call to clear() is made
          pipeline_viewport_state(std::initializer_list<viewport> vp_list, std::initializer_list<rect2D> r_list)
            : viewports(vp_list), scissors(r_list)
          {
            refresh();
          }

          /// \brief Copy constructor (you can copy that object, as long as you keep things alive it will work)
          pipeline_viewport_state(const pipeline_viewport_state &o)
            : dynamic_viewport_count(o.dynamic_viewport_count), dynamic_scissors_count(o.dynamic_scissors_count),
              viewports(o.viewports), scissors(o.scissors), vk_viewports(o.vk_viewports), vk_scissors(o.vk_scissors),
              vk_pvsci(o.vk_pvsci)
          {
          }

          /// \brief Move constructor
          pipeline_viewport_state(pipeline_viewport_state &&o)
            : dynamic_viewport_count(o.dynamic_viewport_count), dynamic_scissors_count(o.dynamic_scissors_count),
              viewports(std::move(o.viewports)), scissors(std::move(o.scissors)), vk_viewports(std::move(o.vk_viewports)), vk_scissors(std::move(o.vk_scissors)),
              vk_pvsci(o.vk_pvsci)
          {
          }

          /// \brief Affectation operator
          pipeline_viewport_state &operator = (const pipeline_viewport_state &o)
          {
            if (this == &o)
              return *this;
            dynamic_viewport_count = (o.dynamic_viewport_count);
            dynamic_scissors_count = (o.dynamic_scissors_count);
            viewports = o.viewports;
            scissors = o.scissors;
            vk_viewports = o.vk_viewports;
            vk_scissors = o.vk_scissors;
            vk_pvsci = o.vk_pvsci;
            return *this;
          }
          /// \brief Move/affectation operator
          pipeline_viewport_state &operator = (pipeline_viewport_state &&o)
          {
            if (this == &o)
              return *this;
            dynamic_viewport_count = (o.dynamic_viewport_count);
            dynamic_scissors_count = (o.dynamic_scissors_count);
            viewports = std::move(o.viewports);
            scissors = std::move(o.scissors);
            vk_viewports = std::move(o.vk_viewports);
            vk_scissors = std::move(o.vk_scissors);
            vk_pvsci = o.vk_pvsci;
            return *this;
          }

          /// \brief Add a list of viewports
          /// \note You are responsible to keep things alive the time this class exists / a call to clear() is made
          pipeline_viewport_state &add_viewports(std::initializer_list<viewport> vp_list)
          {
            viewports.insert(viewports.end(), vp_list.begin(), vp_list.end());
            return *this;
          }

          /// \brief Set the number of dynamic viewports to use
          pipeline_viewport_state &set_dynamic_viewports_count(uint32_t count) { dynamic_viewport_count = count; return *this; }

          bool uses_dynamic_viewports() const { return dynamic_viewport_count != ~0u; }
          void uses_dynamic_viewports(bool uses) { if(uses) dynamic_viewport_count = ~0u; }

          /// \brief Set the number of dynamic scissors to use
          pipeline_viewport_state &set_dynamic_scissors_count(uint32_t count) { dynamic_scissors_count = count; return *this; }

          bool uses_dynamic_scissors() const { return dynamic_scissors_count != ~0u; }
          void uses_dynamic_scissors(bool uses) { if(uses) dynamic_scissors_count = ~0u; }

          /// \brief Add a single viewport
          pipeline_viewport_state &add_viewport(const viewport vp) { viewports.emplace_back(vp); return *this; }

          /// \brief Add a list of scissors
          /// \note You are responsible to keep things alive the time this class exists / a call to clear() is made
          pipeline_viewport_state &add_scissors(std::initializer_list<rect2D> r_list)
          {
            scissors.insert(scissors.end(), r_list.begin(), r_list.end());
            return *this;
          }
          /// \brief Add a single scissor
          pipeline_viewport_state &add_scissor(const rect2D r) { scissors.emplace_back(r); return *this; }

          /// \brief Refresh the vulkan object to account changes in the viewports / scissors
          void refresh()
          {
            // recreate vk_* vectors
            if (!uses_dynamic_viewports())
            {
              vk_viewports.clear();
              vk_viewports.reserve(viewports.size());
              for (auto it : viewports) vk_viewports.push_back(it);
              vk_pvsci.viewportCount = vk_viewports.size();
              vk_pvsci.pViewports = vk_viewports.data();
            }
            else
            {
              vk_pvsci.viewportCount = dynamic_viewport_count;
              vk_pvsci.pViewports = nullptr;
            }

            if (!uses_dynamic_scissors())
            {
              vk_scissors.clear();
              vk_scissors.reserve(scissors.size());
              for (auto it : scissors) vk_scissors.push_back(it);
              vk_pvsci.scissorCount = vk_scissors.size();
              vk_pvsci.pScissors = vk_scissors.data();
            }
            else
            {
              vk_pvsci.scissorCount = dynamic_scissors_count;
              vk_pvsci.pScissors = nullptr;
            }

            // recreate the vk_pvsci object
            vk_pvsci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            vk_pvsci.pNext = nullptr;
            vk_pvsci.flags = 0;
          }

          /// \brief Clear the pipeline_viewport_state
          void clear()
          {
            viewports.clear();
            scissors.clear();
            refresh();
          }

        public: // advanced
          operator const VkPipelineViewportStateCreateInfo &() const { return vk_pvsci; }

        private:
          uint32_t dynamic_viewport_count = ~0u;
          uint32_t dynamic_scissors_count = ~0u;

          std::deque<viewport> viewports;
          std::deque<rect2D> scissors;

          std::vector<VkViewport> vk_viewports;
          std::vector<VkRect2D> vk_scissors;
          VkPipelineViewportStateCreateInfo vk_pvsci;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



