//
// file : clear_value.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/clear_value.hpp
//
// created by : Timothée Feuillet
// date: Fri Aug 26 2016 11:59:49 GMT+0200 (CEST)
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
#include <glm/glm.hpp>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkClearValue
      class clear_value
      {
        public:
          clear_value(const glm::ivec4 &clear_color = glm::ivec4(0, 0, 0, 0))
            : vk_cv { .color = {.int32 = {clear_color.r, clear_color.g, clear_color.b, clear_color.a}} }
          {
          }
          clear_value(const glm::uvec4 &clear_color)
            : vk_cv { .color = {.uint32 = {clear_color.r, clear_color.g, clear_color.b, clear_color.a}} }
          {
          }
          clear_value(const glm::vec4 &clear_color)
            : vk_cv { .color = {.float32 = {clear_color.r, clear_color.g, clear_color.b, clear_color.a}} }
          {
          }
          clear_value(float clear_depth, uint32_t clear_stencil)
            : vk_cv { .depthStencil = {clear_depth, clear_stencil} }
          {
          }

          clear_value(const clear_value &o) : vk_cv(o.vk_cv) {}
          clear_value &operator = (const clear_value &o) {vk_cv = o.vk_cv; return *this; }
          clear_value(const VkClearValue &o) : vk_cv(o) {}
          clear_value &operator = (const VkClearValue &o) {vk_cv = o; return *this; }

          /// \brief Return the depth clear value
          float get_depth_value() const { return vk_cv.depthStencil.depth; }

          /// \brief Set the depth clear value
          void set_depth_value(float _depth_value) { vk_cv.depthStencil.depth = _depth_value; }

          /// \brief Return stencil_value
          uint32_t get_stencil_value() const { return vk_cv.depthStencil.stencil; }

          /// \brief Set stencil_value
          void set_stencil_value(uint32_t _stencil_value) { vk_cv.depthStencil.stencil = _stencil_value; }

          /// \brief Return float32 clear color value
          glm::vec4 get_float_color_value() const { return glm::vec4(vk_cv.color.float32[0], vk_cv.color.float32[1], vk_cv.color.float32[2], vk_cv.color.float32[3]); }
          /// \brief Return int32 clear color value
          glm::ivec4 get_int_color_value() const { return glm::ivec4(vk_cv.color.int32[0], vk_cv.color.int32[1], vk_cv.color.int32[2], vk_cv.color.int32[3]); }
          /// \brief Return uint32 clear color value
          glm::ivec4 get_uint_color_value() const { return glm::uvec4(vk_cv.color.uint32[0], vk_cv.color.uint32[1], vk_cv.color.uint32[2], vk_cv.color.uint32[3]); }

          /// \brief Set clear color value (the float32 one)
          void set_color_value(const glm::vec4 &_color_value)
          {
            vk_cv.color = VkClearColorValue { .float32 = {_color_value.r, _color_value.g, _color_value.b, _color_value.a }};
          }
          /// \brief Set clear color value (the int32 one)
          void set_color_value(const glm::ivec4 &_color_value)
          {
            vk_cv.color = VkClearColorValue { .int32 = {_color_value.r, _color_value.g, _color_value.b, _color_value.a }};
          }
          /// \brief Set clear color value (the uint32 one)
          void set_color_value(const glm::uvec4 &_color_value)
          {
            vk_cv.color = VkClearColorValue { .uint32 = {_color_value.r, _color_value.g, _color_value.b, _color_value.a }};
          }

        public: // advanced
          /// \brief Yields a const reference to a VkClearValue
          operator const VkClearValue &() const { return vk_cv; }
        private:
          VkClearValue vk_cv;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



