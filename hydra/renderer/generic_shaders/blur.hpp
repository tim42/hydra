//
// created by : Timothée Feuillet
// date: 2022-7-31
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

#include <ntools/id/id.hpp>
#include <ntools/id/string_id.hpp>

#include <hydra/engine/hydra_context.hpp>
#include <hydra/utilities/pipeline_render_state.hpp>

namespace neam::hydra::shaders
{
  class blur
  {
    public:
      static constexpr string_id pipeline_id = "neam::hydra::shaders::blur"_rid;

      /// \brief For the setup() part of a render-pass
      static void make_pipeline(pipeline_render_state& prs, hydra_context& context);

      /// \brief Push an image barrier for a transition from a given state to the expected state for the effect:
      /// \note only provide the initial states.
      ///       source / src_ is the image that will be blureed,
      ///       temp is the image that will be used as a temporary
      ///       dest / dst_ (if different from source) is the image that will receive the results
      static void image_memory_barrier_pre(vk::command_buffer_recorder& cbr,
                                           VkPipelineStageFlagBits pre_stage,
                                           vk::image& source, VkImageLayout src_layout, VkAccessFlagBits src_access,
                                           vk::image& temp, VkImageLayout temp_layout, VkAccessFlagBits temp_access,
                                           vk::image& dest,
                                           // optional, required if source != dest
                                           VkImageLayout dst_layout = VK_IMAGE_LAYOUT_MAX_ENUM, VkAccessFlagBits dst_access = VK_ACCESS_NONE);

      /// \brief Push an image barrier for a transition internal to the effect (from a pass to the other):
      /// \note only provide the initial states.
      ///       source / src_ is the image that will be blureed,
      ///       temp is the image that will be used as a temporary
      ///       dest / dst_ (if different from source) is the image that will receive the results
      static void image_memory_barrier_internal(vk::command_buffer_recorder& cbr,
                                                vk::image& source,
                                                vk::image& temp,
                                                vk::image& dest);

      /// \brief Push an image barrier for a transition from the expected state for the effect to a given state:
      /// \note only provide the destination states.
      ///       source / src_ is the image that will be blureed,
      ///       temp is the image that will be used as a temporary
      ///       dest / dst_ (if different from source) is the image that will receive the results
      static void image_memory_barrier_post(vk::command_buffer_recorder& cbr,
                                           VkPipelineStageFlagBits post_stage,
                                            vk::image& source, VkImageLayout src_layout, VkAccessFlagBits src_access,
                                            vk::image& temp, VkImageLayout temp_layout, VkAccessFlagBits temp_access,
                                            vk::image& dest,
                                            // optional, required if source != dest
                                            VkImageLayout dst_layout = VK_IMAGE_LAYOUT_MAX_ENUM, VkAccessFlagBits dst_access = VK_ACCESS_NONE);

      /// \brief Blur an image (only perform a single pass of image blurring, two pass is required)
      /// \note only provide the destination states.
      ///       source is the image that will be blureed,
      ///       temp is the image that will be used as a temporary
      ///       dest (if different from source) is the image that will receive the results
      static void blur_image(hydra_context& context, vk::command_buffer_recorder& cbr,
                             const vk::image_view& source, const vk::image_view& dest,
                             glm::uvec2 image_size,
                             uint32_t strength,
                             bool is_horizontal);
  };
}

