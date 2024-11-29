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
#include <hydra_glm.hpp>

#include <ntools/id/id.hpp>

#include <hydra/vulkan/framebuffer.hpp>
#include <hydra/vulkan/command_buffer.hpp>
#include <hydra/vulkan/command_buffer_recorder.hpp>
#include <hydra/utilities/transfer_context.hpp>

namespace neam::hydra
{
  /// \brief 
  struct render_pass_context
  {
    // state:
    class transfer_context& transfers;

    // global / unchanged inputs:
    glm::uvec2 output_size;
    vk::viewport viewport;
    vk::rect2D viewport_rect;

    // Please use output() instead of directly accessing those:
    std::vector<vk::image*> final_fb_images;
    std::vector<vk::image_view*> final_fb_images_views;

    std::optional<std::vector<vk::image*>> output_fb_images;
    std::optional<std::vector<vk::image_view*>> output_fb_images_views;

    VkImageLayout current_layout;

    std::vector<vk::image*>& output_images()
    {
      if (output_fb_images)
        return *output_fb_images;
      return final_fb_images;
    }
    std::vector<vk::image_view*>& output_images_views()
    {
      if (output_fb_images_views)
        return *output_fb_images_views;
      return final_fb_images_views;
    }

    vk::image& output_image(uint32_t index)
    {
      auto& vct = output_images();
      check::debug::n_assert(index < vct.size(), "Out of bound access on image vector");
      check::debug::n_assert(vct[index] != nullptr, "Trying to get a ref to a null image");
      return *vct[index];
    }
    vk::image_view& output_image_view(uint32_t index)
    {
      auto& vct = output_images_views();
      check::debug::n_assert(index < vct.size(), "Out of bound access on image-view vector");
      check::debug::n_assert(vct[index] != nullptr, "Trying to get a ref to a null image-view");
      return *vct[index];
    }

    /// \brief Helper to transition the whole context to a new layout / do a read/write barrier
    void pipeline_barrier(vk::command_buffer_recorder& cbr, VkImageLayout new_layout,
                          VkAccessFlags src_access, VkAccessFlags dst_access,
                          VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
    {
      auto& vct = output_images();
      std::vector<vk::image_memory_barrier> imb;
      for (auto& it : vct)
      {
        imb.push_back(vk::image_memory_barrier{*it, current_layout, new_layout,
                                               src_access, dst_access });
      }
      cbr.pipeline_barrier(src_stage, dst_stage, 0, imb);
      current_layout = new_layout;

    }

    /// \brief Helper to do a read/write barrier on the context
    void pipeline_barrier(vk::command_buffer_recorder& cbr,
                          VkAccessFlags src_access, VkAccessFlags dst_access,
                          VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
    {
      pipeline_barrier(cbr, current_layout, src_access, dst_access, src_stage, dst_stage);
    }

    /// \brief Helper for a generic begin rendering
    void begin_rendering(vk::command_buffer_recorder& cbr, VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op)
    {
      auto& vct = output_images_views();
      std::vector<hydra::vk::rendering_attachment_info> rai;
      rai.reserve(vct.size());
      for (auto* imgv : vct)
      {
        rai.push_back
        ({
            *imgv, current_layout,
            load_op, store_op
        });
      }
      cbr.begin_rendering
      ({
        viewport_rect, rai,
      });
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

