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

#include "blur.hpp"
#include "shader_structs.hpp"

namespace neam::hydra::shaders
{
  void blur::make_pipeline(pipeline_render_state& prs, hydra_context& context)
  {
    prs.create_simple_compute(context, "shaders/engine/generic/blur.hsf:spirv(main)"_rid);
  }

  void blur::image_memory_barrier_pre(vk::command_buffer_recorder& cbr,
                                VkPipelineStageFlagBits pre_stage,
                                vk::image& source, VkImageLayout src_layout, VkAccessFlagBits src_access,
                                vk::image& temp, VkImageLayout temp_layout, VkAccessFlagBits temp_access,
                                vk::image& dest, VkImageLayout dst_layout, VkAccessFlagBits dst_access)
  {
    std::vector<neam::hydra::vk::image_memory_barrier> ppb
    {
      vk::image_memory_barrier{source, src_layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, src_access, VK_ACCESS_SHADER_READ_BIT },
      vk::image_memory_barrier{temp, temp_layout, VK_IMAGE_LAYOUT_GENERAL, temp_access, VK_ACCESS_SHADER_WRITE_BIT },
    };

    if (&source != &dest)
    {
      // will avoid a transition later on
      ppb.push_back(vk::image_memory_barrier{dest, dst_layout, VK_IMAGE_LAYOUT_GENERAL, dst_access, VK_ACCESS_SHADER_WRITE_BIT });
    }

    cbr.pipeline_barrier(pre_stage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, ppb);
  }

  void blur::image_memory_barrier_internal(vk::command_buffer_recorder& cbr,
                                           vk::image& source,
                                           vk::image& temp,
                                           vk::image& dest)
  {
    std::vector<neam::hydra::vk::image_memory_barrier> ppb
    {
      vk::image_memory_barrier{temp, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                     VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT },
    };

    if (&source == &dest)
    {
      ppb.push_back({source, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                              VK_ACCESS_SHADER_READ_BIT,               VK_ACCESS_SHADER_WRITE_BIT, });
    }

    cbr.pipeline_barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, ppb);
  }

  void blur::image_memory_barrier_post(vk::command_buffer_recorder& cbr,
                                       VkPipelineStageFlagBits post_stage,
                                       vk::image& source, VkImageLayout src_layout, VkAccessFlagBits src_access,
                                       vk::image& temp, VkImageLayout temp_layout, VkAccessFlagBits temp_access,
                                       vk::image& dest, VkImageLayout dst_layout, VkAccessFlagBits dst_access)
  {
    std::vector<neam::hydra::vk::image_memory_barrier> ppb
    {
    };

    if (&temp != &source && &temp != &dest)
      ppb.push_back(vk::image_memory_barrier{temp, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, temp_layout, VK_ACCESS_SHADER_READ_BIT, temp_access, });

    if (&source != &dest)
    {
      ppb.push_back(vk::image_memory_barrier{source, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, src_layout, VK_ACCESS_SHADER_READ_BIT, src_access, });
      ppb.push_back(vk::image_memory_barrier{dest, VK_IMAGE_LAYOUT_GENERAL, dst_layout, VK_ACCESS_SHADER_WRITE_BIT, dst_access, });
    }
    else
    {
      ppb.push_back(vk::image_memory_barrier{source, VK_IMAGE_LAYOUT_GENERAL, src_layout, VK_ACCESS_SHADER_WRITE_BIT, src_access, });
    }

    cbr.pipeline_barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, post_stage, 0, ppb);
  }

  void blur::blur_image(hydra_context& context, render_pass_context& rpctx, vk::command_buffer_recorder& cbr,
                        const vk::image_view& source, const vk::image_view& dest,
                        glm::uvec2 image_size,
                        uint32_t strength,
                        bool is_horizontal)
  {
    const vk::pipeline_layout& pipeline_layout = context.ppmgr.get_pipeline_layout<blur>();

    blur_descriptor_set blur_ds
    {
      .u_input = source,
      .u_output = dest,
    };
    blur_ds.update_descriptor_set(context);

    cbr.bind_pipeline(context.ppmgr.get_pipeline<blur>());
    cbr.bind_descriptor_set(context, blur_ds);
    cbr.push_constants(pipeline_layout, 0, blur_push_constants
    {
      .image_size = image_size,
      .is_horizontal = is_horizontal ? 1u: 0u,
      .strength = strength,
    });

    glm::uvec2 rounded_img_sz = (image_size + 63u) / 64u;
    if (is_horizontal)
      cbr.dispatch(glm::uvec3(rounded_img_sz.x, image_size.y, 1u));
    else
      cbr.dispatch(glm::uvec3(rounded_img_sz.y, image_size.x, 1u));

    context.dfe.defer_destruction(std::move(blur_ds));
  }
}

