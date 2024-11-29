//
// file : image_2d.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/image_creators/image_2d.hpp
//
// created by : Timothée Feuillet
// date: Sun May 01 2016 11:55:44 GMT+0200 (CEST)
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


#include <initializer_list>
#include <functional>

#include <vulkan/vulkan.h>
#include <hydra_glm.hpp>
#include "../queue.hpp"


namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Image creator for concurrent images
      class image_concurrent
      {
        public:
          image_concurrent(std::initializer_list<std::reference_wrapper<const vk::queue>> queues)
          {
            for (const auto& q : queues)
            {
              if (std::find(family_indices.begin(), family_indices.end(), q.get().get_queue_familly_index()) == family_indices.end())
                family_indices.emplace_back(q.get().get_queue_familly_index());
            }
          }
          image_concurrent(const image_concurrent& o) = default;

          void update_image_create_info(VkImageCreateInfo &create_info) const
          {
            create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = family_indices.size();
            create_info.pQueueFamilyIndices = family_indices.data();
          }

        private:
          std::vector<uint32_t> family_indices;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



