//
// file : viewport.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/viewport.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 13 2016 10:51:42 GMT+0200 (CEST)
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

#ifndef __N_1359446532349210609_901327426_VIEWPORT_HPP__
#define __N_1359446532349210609_901327426_VIEWPORT_HPP__

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "rect2D.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a vulkan VkViewport
      class viewport
      {
        public:
          /// \brief Construct a viewport from a rect2D
          explicit viewport(const rect2D &rect, float mindepth = 0.f, float maxdepth = 1.f) : viewport(rect.get_size(), rect.get_offset(), mindepth, maxdepth) {}

          /// \brief Construct a viewport
          explicit viewport(const glm::vec2 &size, const glm::vec2 &offset = glm::vec2(0, 0), float mindepth = 0.f, float maxdepth = 1.f)
           : vk_viewport
          {
            offset.x,
            offset.y,
            size.x,
            size.y,
            mindepth,
            maxdepth
          }
          {
          }

          /// \brief copy constructor
          viewport(const viewport &o) : vk_viewport(o.vk_viewport) {}
          /// \brief copy operator
          viewport &operator = (const viewport &o)
          {
            vk_viewport = o.vk_viewport;
            return *this;
          }

          /// \brief Set the size of the viewport
          void set_size(const glm::vec2 &size) { vk_viewport.width = size.x; vk_viewport.height = size.y; }
          /// \brief Get the size of the viewport
          glm::vec2 get_size() const { return glm::vec2(vk_viewport.width, vk_viewport.height); }

          /// \brief Set the offset of the viewport
          void set_offset(const glm::vec2 &offset) { vk_viewport.x = offset.x; vk_viewport.y = offset.y; }
          /// \brief Get the offset of the viewport
          glm::vec2 get_offset() const { return glm::vec2(vk_viewport.x, vk_viewport.y); }

          /// \brief Set a rect2D for the offset/size
          void set_rect2d(const rect2D &r)
          {
            set_size(r.get_size());
            set_offset(r.get_offset());
          }

          /// \brief Return a rect2D for the offset/size
          rect2D get_rect2d() const { return rect2D(get_offset(), get_size()); }

          /// \brief Set the depth range of the viewport (x -> min depth, y -> max depth)
          /// \note should stay in the [0, 1] range
          void set_depth_range(const glm::vec2 &drange) { vk_viewport.minDepth = drange.x; vk_viewport.maxDepth = drange.y; }
          /// \brief Get the depth range of the viewport (x -> min depth, y -> max depth)
          glm::vec2 get_depth_range() const { return glm::vec2(vk_viewport.minDepth, vk_viewport.maxDepth); }

          /// \brief Set the min depth of the viewport (should stay in [0,1])
          void set_min_depth(float mdepth) { vk_viewport.minDepth = mdepth; }
          /// \brief Return the min depth
          float get_min_depth() const { return vk_viewport.minDepth; }

          /// \brief Set the max depth of the viewport (should stay in [0,1])
          void set_max_depth(float mdepth) { vk_viewport.maxDepth = mdepth; }
          /// \brief Return the max depth
          float get_max_depth() const { return vk_viewport.maxDepth; }

        public: // advanced
          /// \brief copy constructor
          viewport(const VkViewport &o) : vk_viewport(o) {}
          /// \brief copy operator
          viewport &operator = (const VkViewport &o)
          {
            vk_viewport = o;
            return *this;
          }

          /// \brief Lay a const reference to a VkViewport
          operator const VkViewport &() const { return vk_viewport; }

        private:
          VkViewport vk_viewport;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_1359446532349210609_901327426_VIEWPORT_HPP__

