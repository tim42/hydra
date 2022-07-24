//
// created by : Timothée Feuillet
// date: 2022-5-20
//
//
// Copyright (c) 2022 Timothée Feuillet
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

#include <optional>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <ntools/id/id.hpp>

#include <hydra/vulkan/framebuffer.hpp>
#include <hydra/vulkan/command_buffer.hpp>

namespace neam::hydra
{
  /// \brief 
  struct render_pass_context
  {
    // global / unchanged inputs:
    glm::uvec2 output_size;
    vk::viewport viewport;
    vk::rect2D viewport_rect;
    VkFormat output_format;

    // Please use output() instead of directly accessing those:
    vk::framebuffer& final_fb;
    std::optional<vk::framebuffer> output_fb;

    /// \brief Return the current framebuffer
    vk::framebuffer& output()
    {
      if (output_fb)
        return *output_fb;
      return final_fb;
    }

    /// \brief 
    vk::image& swap_buffers()
    {
    }
  };

  struct render_pass_output
  {
    std::vector<vk::command_buffer> graphic;
    std::vector<vk::command_buffer> compute;
//     std::vector<vk::command_buffer> transfer;

    void insert_back(render_pass_output&& o)
    {
      graphic.reserve(o.graphic.size());
      compute.reserve(o.compute.size());
//       transfer.reserve(o.transfer.size());
      for (auto& it : o.graphic)
        graphic.push_back(std::move(it));
      for (auto& it : o.compute)
        compute.push_back(std::move(it));
//       for (auto& it : o.transfer)
//         transfer.push_back(std::move(it));
    }
  };
}

