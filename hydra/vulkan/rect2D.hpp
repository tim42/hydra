//
// file : rect2D.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/rect2D.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 13 2016 11:12:20 GMT+0200 (CEST)
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
#include <hydra_glm.hpp>


namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a vulkan rect2D
      class rect2D
      {
        public:
          /// \brief Create a rect2D
          rect2D(const glm::ivec2 &offset, const glm::uvec2 &size) : rect { {offset.x, offset.y}, {size.x, size.y} } {}
          /// \brief Copy constructor
          rect2D(const rect2D &o) : rect(o.rect) {}
          /// \brief Copy operator
          rect2D &operator = (const rect2D &o) { rect = o.rect; return *this;}
          /// \brief Copy constructor
          rect2D(const VkRect2D &o) : rect(o) {}
          /// \brief Copy operator
          rect2D &operator = (const VkRect2D &o) { rect = o; return *this;}

          /// \brief Return the offset
          glm::ivec2 get_offset() const { return glm::ivec2(rect.offset.x, rect.offset.y); }
          /// \brief Set the offset
          void set_offset(const glm::ivec2 &offset) { rect.offset.x = offset.x; rect.offset.y = offset.y; }
          /// \brief Return the end offset
          glm::ivec2 get_end_offset() const { return glm::ivec2(rect.offset.x + rect.extent.width, rect.offset.y + rect.extent.height); }

          /// \brief Translate the offset by displ
          void translate_offset(const glm::ivec2 &displ) { set_offset(get_offset() + displ); }

          /// \brief Return the size
          glm::uvec2 get_size() const { return glm::uvec2(rect.extent.width, rect.extent.height); }
          /// \brief Set the size
          void set_size(const glm::uvec2 &size) { rect.extent.width = size.x; rect.extent.height = size.y; }

          /// \brief Grow / shrink the size of the rect by dt
          void grow_size(const glm::ivec2 &dt) { set_size(static_cast<glm::ivec2>(get_size()) + dt); }

        public: // advanced
          /// \brief Yield a VkRect2D
          operator const VkRect2D &() const { return rect; }

        private:
          VkRect2D rect;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



