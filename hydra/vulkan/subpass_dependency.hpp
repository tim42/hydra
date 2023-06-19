//
// file : subpass_dependency.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/subpass_dependency.hpp
//
// created by : Timothée Feuillet
// date: Tue Aug 23 2016 13:38:46 GMT+0200 (CEST)
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
      /// \brief A subpass dependency (wraps a VkSubpassDependency)
      /// \note It has been made to allow chained calls
      class subpass_dependency
      {
        public:
          /// \brief Start to initialize a subpass dependency
          subpass_dependency(size_t src_index, size_t dst_index, VkDependencyFlags dependency_flags = 0)
          {
            vk_sd.srcSubpass = src_index;
            vk_sd.dstSubpass = dst_index;
            vk_sd.dependencyFlags = dependency_flags;
          }

          /// \brief Set both the stage mask and access mask of the source subpass
          subpass_dependency &source_subpass_masks(VkPipelineStageFlags stage_mask, VkAccessFlags access_mask) { vk_sd.srcStageMask = stage_mask; vk_sd.srcAccessMask = access_mask; return *this; }
          /// \brief Set both the stage mask and access mask of the destination subpass
          subpass_dependency &dest_subpass_masks(VkPipelineStageFlags stage_mask, VkAccessFlags access_mask) { vk_sd.dstStageMask = stage_mask; vk_sd.dstAccessMask = access_mask; return *this; }

          /// \brief Set both the source and the dest subpass indexes
          subpass_dependency &set_subpass_indexes(size_t src_index, size_t dst_index) { vk_sd.srcSubpass = src_index; vk_sd.dstSubpass = dst_index; return *this; }
          /// \brief Set the source subpass index
          subpass_dependency &set_source_subpass_index(size_t src_index) { vk_sd.srcSubpass = src_index; return *this; }
          /// \brief Set the dest subpass index
          subpass_dependency &set_dest_subpass_index(size_t dst_index) { vk_sd.dstSubpass = dst_index; return *this; }

          /// \brief Set the dependency flags
          subpass_dependency &set_dependency_flags(VkDependencyFlags dependency_flags) { vk_sd.dependencyFlags = dependency_flags; return *this; }

        public: // advanced
          /// \brief Return a const reference to VkSubpassDependency
          operator const VkSubpassDependency &() const { return vk_sd; }
        private:
          VkSubpassDependency vk_sd;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



