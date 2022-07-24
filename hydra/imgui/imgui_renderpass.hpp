//
// created by : Timothée Feuillet
// date: 2021-11-21
//
//
// Copyright (c) 2021 Timothée Feuillet
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

#include <hydra/renderer/render_pass.hpp>
#include <hydra/engine/hydra_context.hpp>

#include "imgui_context.hpp"
#include <ntools/container_utils.hpp>

namespace neam::hydra::imgui
{
  class render_pass : public neam::hydra::render_pass
  {
    private:
      static void make_imgui_pipeline(hydra::hydra_context& context, imgui_context& related_context, hydra::pipeline_render_state& prs)
      {
        hydra::vk::descriptor_set_layout ds_layout =
        {
          context.device,
          {
            neam::hydra::vk::descriptor_set_layout_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, related_context.font_sampler),
          }
        };
        // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
        hydra::vk::pipeline_layout p_layout =
        {
          context.device, { &ds_layout }, { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4} }
        };

        prs.create_pipeline_info
        (
          std::move(ds_layout),
          hydra::vk::descriptor_pool
          {
            context.device, 512,
            {
              { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
            },
            VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
          },
          std::move(p_layout)
        );

        hydra::vk::pipeline_creator& pcr = prs.get_pipeline_creator();
        {
          neam::hydra::vk::pipeline_vertex_input_state pvis;
          pvis.add_binding_description(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX);
          pvis.add_attribute_description(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos));
          pvis.add_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv));
          pvis.add_attribute_description(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col));

          pcr.get_vertex_input_state() = pvis;
          pcr.get_input_assembly_state().set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
          pcr.get_pipeline_rasterization_state().set_cull_mode(VK_CULL_MODE_NONE);
        }

        pcr.get_viewport_state()
          .set_dynamic_viewports_count(1)
          .set_dynamic_scissors_count(1);

        pcr.get_pipeline_color_blending_state().add_attachment_color_blending(neam::hydra::vk::attachment_color_blending::create_alpha_blending());

        pcr.get_pipeline_shader_stage()
          .add_shader(context.shmgr.load_shader("shaders/engine/imgui/imgui.hsf:spirv(main_vs)"_rid), VK_SHADER_STAGE_VERTEX_BIT)
          .add_shader(context.shmgr.load_shader("shaders/engine/imgui/imgui.hsf:spirv(main_fs)"_rid), VK_SHADER_STAGE_FRAGMENT_BIT);
      }

    public:
      render_pass(imgui_context& _related_context, hydra_context& _context, ImGuiViewport* _imgui_viewport)
        : hydra::render_pass(_context),
          related_context(_related_context),
          imgui_viewport(_imgui_viewport),
          vk_render_pass(context.device)
      {
      }

      void setup(render_pass_context& rpctx) override
      {
        vk_render_pass.create_subpass().add_attachment(neam::hydra::vk::subpass::attachment_type::color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
        vk_render_pass.create_subpass_dependency(VK_SUBPASS_EXTERNAL, 0)
          .dest_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
          .source_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0);

        vk_render_pass.create_attachment().set_format(rpctx.output_format).set_samples(VK_SAMPLE_COUNT_1_BIT)
          .set_load_op(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
          .set_store_op(VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
          .set_layouts(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        vk_render_pass.refresh();

        // create the pipeline
        context.ppmgr.add_pipeline("imgui::pipeline"_rid, [this](auto& p) { make_imgui_pipeline(context, related_context, p); });

        if (related_context.font_texture_ds._get_vk_descritpor_set() == nullptr)
          related_context.font_texture_ds = context.ppmgr.allocate_descriptor_set("imgui::pipeline"_rid);
      }

      void refresh(hydra::vk::swapchain& /*swapchain*/)
      {
        vk_render_pass.refresh();
      }

      void prepare(render_pass_context&) override
      {
        check::on_vulkan_error::n_assert(related_context.is_current_context(), "Trying to draw an imgui context when it's not the current one. There might be an error somewhere before.");

        if (related_context.should_regenerate_fonts())
          setup_font_texture();
        related_context.set_font_as_regenerated();

        const ImDrawData* draw_data = imgui_viewport->DrawData;
        if (!draw_data)
          return;

        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        const int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
        const int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
          return;

        // Create or resize the vertex/index buffers
        const size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        const size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

        // nothing to do
        if (!vertex_size || !index_size)
          return;

        if (!vertex_buffer || vertex_buffer->buffer.size() < vertex_size)
        {
          if (vertex_buffer) context.vrd.postpone_destruction_to_next_fence(context.gqueue, std::move(*vertex_buffer));
          vertex_buffer.emplace(context.allocator, vk::buffer(context.device, vertex_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
        }
        if (!index_buffer || index_buffer->buffer.size() < index_size)
        {
          if (index_buffer) context.vrd.postpone_destruction_to_next_fence(context.gqueue, std::move(*index_buffer));
          index_buffer.emplace(context.allocator, vk::buffer(context.device, index_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT));
        }

        // Upload data to the buffers:
        {
          ImDrawVert* const vtx_dst = (ImDrawVert*)context.transfers.allocate_memory(vertex_size);
          ImDrawIdx* const idx_dst = (ImDrawIdx*)context.transfers.allocate_memory(index_size);
          {
            ImDrawVert* it_vtx_dst = vtx_dst;
            ImDrawIdx* it_idx_dst = idx_dst;

            for (int n = 0; n < draw_data->CmdListsCount; n++)
            {
              const ImDrawList* cmd_list = draw_data->CmdLists[n];
              memcpy(it_vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
              memcpy(it_idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
              it_vtx_dst += cmd_list->VtxBuffer.Size;
              it_idx_dst += cmd_list->IdxBuffer.Size;
            }
          }

          context.transfers.add_transfer(vertex_buffer->buffer, 0, vertex_size, vtx_dst, context.gqueue.get_queue_familly_index());
          context.transfers.add_transfer(index_buffer->buffer, 0, index_size, idx_dst, context.gqueue.get_queue_familly_index());
        }
      }

      render_pass_output submit(render_pass_context& rpctx) override
      {
        return draw(rpctx);
      }

    public: // imgui related:
      void setup_font_texture()
      {
        // unalloc the texture and its mem allocation
        if (related_context.font_texture)
        {
          context.vrd.postpone_destruction_to_next_fence(context.gqueue, std::move(*related_context.font_texture));
          related_context.font_texture.reset();
        }

        unsigned char* pixels;
        int width, height;
        related_context.get_io().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        size_t upload_size = width * height * 4 * sizeof(char);

        cr::out().debug("imgui: generating a {} x {} font texture", width, height);

        // create the texture:
        related_context.font_texture.emplace
        (
          context, vk::image::create_image_arg
          (
            context.device,
            neam::hydra::vk::image_2d
            (
              {width, height}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            )
          )
        );

        // upload the data to the texture:
        context.transfers.add_transfer(related_context.font_texture->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, upload_size, pixels);

        // update ds
        related_context.font_texture_ds.write_descriptor_set(0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        {
          { related_context.font_texture->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
        });
      }

      render_pass_output draw(render_pass_context& rpctx)
      {
        // nothing to do
        if (!vertex_buffer || !index_buffer)
          return {};

        const ImDrawData* draw_data = imgui_viewport->DrawData;
        if (!draw_data)
          return {};

        const int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
        const int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
          return {};

        vk::command_buffer cmd_buf = graphic_transient_cmd_pool.create_command_buffer();
        neam::hydra::vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // FIXME !
        {
          // setup pipeline barriers for both index and vertex buffer:
          cbr.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
          {
            vk::buffer_memory_barrier::queue_transfer(vertex_buffer->buffer, context.transfers.get_queue_familly_index(), context.gqueue.get_queue_familly_index()),
            vk::buffer_memory_barrier::queue_transfer(index_buffer->buffer, context.transfers.get_queue_familly_index(), context.gqueue.get_queue_familly_index())
          });
        }

        cbr.begin_render_pass(vk_render_pass, rpctx.output(), rpctx.viewport_rect, VK_SUBPASS_CONTENTS_INLINE, {});

        {
          const glm::vec2 scale = glm::vec2(2.0f / draw_data->DisplaySize.x, 2.0f / draw_data->DisplaySize.y);
          setup_renderstate(cbr, scale, -1.0f - glm::vec2(draw_data->DisplayPos.x, draw_data->DisplayPos.y) * scale, {fb_width, fb_height});
        }

        // Will project scissor/clipping rectangles into framebuffer space
        const ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
        const ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int global_vtx_offset = 0;
        int global_idx_offset = 0;

        ImTextureID last_texture_id = nullptr; // default font texture
        const vk::pipeline_layout& pipeline_layout = context.ppmgr.get_pipeline_layout("imgui::pipeline"_rid);

        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
          const ImDrawList* cmd_list = draw_data->CmdLists[n];
          for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
          {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL)
            {
              // User callback, registered via ImDrawList::AddCallback()
              // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
              if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
              {
                const glm::vec2 scale = glm::vec2(2.0f / draw_data->DisplaySize.x, 2.0f / draw_data->DisplaySize.y);
                setup_renderstate(cbr, scale, -1.0f - glm::vec2(draw_data->DisplayPos.x, draw_data->DisplayPos.y) * scale, {fb_width, fb_height});
                last_texture_id = nullptr;
              }
              else
              {
                pcmd->UserCallback(cmd_list, pcmd);
              }
            }
            else
            {
              // Project scissor/clipping rectangles into framebuffer space
              ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
              ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

              // Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
              if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
              if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
              if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
              if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
              if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                continue;

              VkRect2D scissor;
              scissor.offset.x = (int32_t)(clip_min.x);
              scissor.offset.y = (int32_t)(clip_min.y);
              scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
              scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);

              ImTextureID current_texture_id = pcmd->GetTexID();
              if (current_texture_id != last_texture_id)
              {
                last_texture_id = current_texture_id;
                if (current_texture_id == nullptr || current_texture_id == &related_context.font_texture->view)
                {
                  // easy #1: the font texture
                  cbr.bind_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, { &related_context.font_texture_ds });
                }
                else if (auto it = textures_ds_cache.find(current_texture_id); it != textures_ds_cache.end())
                {
                  // easy #2: already in cache
                  cbr.bind_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, { &it->second });
                }
                else
                {
                  // not the font texture / not in the cache: create the descriptor-set and update it
                  vk::descriptor_set current_texture_ds = context.ppmgr.allocate_descriptor_set("imgui::pipeline"_rid, true /*allow free*/);
                  current_texture_ds.write_descriptor_set(0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                  {
                    { *current_texture_id, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
                  });
                  cbr.bind_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, { &current_texture_ds });
                  textures_ds_cache.emplace(current_texture_id, std::move(current_texture_ds));
                }
              }

              cbr.set_scissor(scissor);
              cbr.draw_indexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
            }
          }
          global_idx_offset += cmd_list->IdxBuffer.Size;
          global_vtx_offset += cmd_list->VtxBuffer.Size;
        }

        cbr.end_render_pass();
        cmd_buf.end_recording();


        return { .graphic = cr::construct<std::vector>(std::move(cmd_buf)) };
      }

      void cleanup() override
      {
        // Flush the ds cache when not used:
        context.vrd.postpone_destruction_to_next_fence(context.gqueue, std::move(textures_ds_cache));
        decltype(textures_ds_cache) tmp;
        textures_ds_cache.swap(tmp);
      }

    private: // rendering stuff
      void setup_renderstate(vk::command_buffer_recorder& cbr, glm::vec2 scale, glm::vec2 translate, glm::ivec2 fb_size)
      {
        cbr.bind_pipeline(context.ppmgr.get_pipeline("imgui::pipeline"_rid, vk_render_pass, 0));
        const vk::pipeline_layout& pipeline_layout = context.ppmgr.get_pipeline_layout("imgui::pipeline"_rid);
        cbr.bind_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, { &related_context.font_texture_ds });

        // Setup viewport:
        {
          VkViewport viewport;
          viewport.x = 0;
          viewport.y = 0;
          viewport.width = (float)fb_size.x;
          viewport.height = (float)fb_size.y;
          viewport.minDepth = 0.0f;
          viewport.maxDepth = 1.0f;
          cbr.set_viewport({viewport}, 0, 1);
        }
        cbr.bind_vertex_buffer(vertex_buffer->buffer, 0);
        cbr.bind_index_buffer(index_buffer->buffer, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

        cbr.push_constants(pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::vec2) * 0, scale);
        cbr.push_constants(pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::vec2) * 1, translate);
      }

    private:
      imgui_context& related_context;
      ImGuiViewport* imgui_viewport = nullptr;
      neam::hydra::vk::render_pass vk_render_pass;

      struct buffer_holder
      {
        vk::buffer buffer;
        memory_allocation allocation;

        buffer_holder(memory_allocator& allocator, vk::buffer&& _buffer)
          : buffer(std::move(_buffer))
          , allocation(allocator.allocate_memory(buffer.get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, neam::hydra::allocation_type::persistent))
        {
          buffer.bind_memory(*allocation.mem(), allocation.offset());
        }
        buffer_holder(buffer_holder&&) = default;
        buffer_holder& operator = (buffer_holder&&) = default;
      };


      std::optional<buffer_holder> vertex_buffer;
      std::optional<buffer_holder> index_buffer;

      // Temporary cache of texture ds, so we can simply pass image_view around,
      // without having to deal with full-on descriptor sets
      std::map<ImTextureID, vk::descriptor_set> textures_ds_cache;
  };
}
