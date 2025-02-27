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

#include <hydra/renderer/generic_shaders/blur.hpp>
#include <hydra/engine/hydra_context.hpp>
#include <hydra/utilities/holders.hpp>

#include "imgui_context.hpp"
#include "shader_structs.hpp"
#include "imgui_drawdata.hpp"
#include "imgui_engine_module.hpp"
#include "imgui_setup_pass.hpp"
#include <ntools/container_utils.hpp>
#include <ntools/tracy.hpp>

// #define N_IMGUI_BLUR 1 // FIXME: validation errors
// #define N_FULLSIZE_BLUR 1 // FIXME: validation errors

namespace neam::hydra::imgui::components
{
  class render_pass : public ecs::internal_component<render_pass>, renderer::concepts::gpu_task_producer::concept_provider<render_pass>
  {
    public:
      render_pass(param_t p, hydra_context& _hctx, ImGuiViewport* _imgui_viewport)
        : internal_component_t(p)
        , gpu_task_producer_provider_t(*this, _hctx)
        , related_context(hctx.engine->get_module<imgui_module>()->get_imgui_context())
        , imgui_viewport(_imgui_viewport)
      {}

    private:
      struct prepare_state_t
      {
        buffer_holder vertex_buffer;
        buffer_holder index_buffer;

        renderer::exported_image backbuffer;

        std::map<ImTextureID, vk::descriptor_set> textures_ds_cache;
      };

    private:
      static void make_imgui_pipeline(hydra::hydra_context& context, imgui_context& related_context, hydra::pipeline_render_state& prs)
      {
        hydra::vk::graphics_pipeline_creator& pcr = prs.get_graphics_pipeline_creator();
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
          .add_shader(context.shmgr.load_shader("shaders/engine/imgui/imgui.hsf:spirv(main_vs)"_rid))
          .add_shader(context.shmgr.load_shader("shaders/engine/imgui/imgui.hsf:spirv(main_fs)"_rid));
      }
      void setup(renderer::gpu_task_context& rpctx)
      {
        hctx.ppmgr.add_pipeline<shaders::blur>(hctx);

        // create the pipeline
        hctx.ppmgr.add_pipeline("imgui::pipeline"_rid, [this](auto& p) { make_imgui_pipeline(hctx, related_context, p); });

        // if (related_context.font_texture_ds._get_vk_descritpor_set() == nullptr)
        // related_context.font_texture_ds = context.ppmgr.allocate_descriptor_set("imgui::pipeline"_rid);
      }

      bool is_enabled() const
      {
        const ImDrawData* draw_data = nullptr;
        if (imgui_viewport->RendererUserData)
          draw_data = &(((draw_data_t*)imgui_viewport->RendererUserData)->draw_data);
        if (!draw_data)
          return false;
        const int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
        const int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
          return false;
        const size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        const size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

        // nothing to do
        if (!vertex_size || !index_size)
          return false;

        return true;
      }

      prepare_state_t prepare(renderer::gpu_task_context& gtctx)
      {
        check::on_vulkan_error::n_assert(related_context.is_current_context(), "Trying to draw an imgui context when it's not the current one. There might be an error somewhere before.");

        const ImDrawData* draw_data = &(((draw_data_t*)imgui_viewport->RendererUserData)->draw_data);

        // Create or resize the vertex/index buffers
        const size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        const size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

        buffer_holder vertex_buffer(hctx.allocator, vk::buffer(hctx.device, vertex_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), hydra::allocation_type::short_lived);
        vertex_buffer.buffer._set_debug_name("imgui::vertex-buffer");

        buffer_holder index_buffer(hctx.allocator, vk::buffer(hctx.device, index_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT), hydra::allocation_type::short_lived);
        index_buffer.buffer._set_debug_name("imgui::index-buffer");

        // Upload data to the buffers:
        {
          raw_data vtx_dst = raw_data::allocate(vertex_size);
          raw_data idx_dst = raw_data::allocate(index_size);
          {
            ImDrawVert* it_vtx_dst = (ImDrawVert*)vtx_dst.get();
            ImDrawIdx* it_idx_dst = (ImDrawIdx*)idx_dst.get();

            for (int n = 0; n < draw_data->CmdListsCount; n++)
            {
              const ImDrawList* cmd_list = draw_data->CmdLists[n];
              memcpy(it_vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
              memcpy(it_idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
              it_vtx_dst += cmd_list->VtxBuffer.Size;
              it_idx_dst += cmd_list->IdxBuffer.Size;
            }
          }

          gtctx.transfers.transfer(vertex_buffer.buffer, std::move(vtx_dst));
          gtctx.transfers.release(vertex_buffer.buffer, hctx.gqueue);

          gtctx.transfers.transfer(index_buffer.buffer, std::move(idx_dst));
          gtctx.transfers.release(index_buffer.buffer, hctx.gqueue);
        }

        return
        {
          .vertex_buffer = std::move(vertex_buffer),
          .index_buffer = std::move(index_buffer),

          .backbuffer = import_image(renderer::k_context_final_output, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
        };
      }

      void submit(renderer::gpu_task_context& gtctx, vk::submit_info& si, prepare_state_t& ps)
      {
        const ImDrawData* draw_data = &(((draw_data_t*)imgui_viewport->RendererUserData)->draw_data);

        const int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
        const int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);

        vk::command_buffer cmd_buf = hctx.gcpm.get_pool().create_command_buffer();
        cmd_buf._set_debug_name("imgui::command_buffer");
        neam::hydra::vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        {
          neam::hydra::vk::cbr_debug_marker _dm(cbr, "imgui");

          // Create the descriptor set for imgui:
          imgui_shader_params imgui_descriptor_set;
          imgui_descriptor_set.s_sampler = related_context.font_sampler;


          std::map<uint32_t, uint32_t> im_to_tm_index;
          {
            uint32_t im_texture_index = 0;

            for (int n = 0; n < draw_data->CmdListsCount; n++)
            {
              const ImDrawList* cmd_list = draw_data->CmdLists[n];
              for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
              {
                ++im_texture_index;

                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                ImTextureID current_texture_id = pcmd->GetTexID();
                if (current_texture_id == nullptr)
                  current_texture_id = &related_context.font_texture->view;


                std::visit([&, this]<typename T>(const T& v)
                {
                  if constexpr (std::is_same_v<T, const vk::image_view*>) // we were provided an image view directly:
                  {
                    imgui_descriptor_set.s_textures.push_back(*v);
                    return;
                  }
                  else
                  {
                    uint32_t texture_index;
                    if constexpr (std::is_same_v<T, id_t>) // we were provided a resource id
                      texture_index = hctx.textures.request_texture_index(string_id::_from_id_t(v));
                    else if (std::is_same_v<T, unsigned>)
                      texture_index = v;

                    im_to_tm_index.emplace(im_texture_index - 1, hctx.textures.texture_index_to_gpu_index(texture_index));
                    // prevent the texture from being streamed-out
                    hctx.textures.indicate_texture_usage(texture_index, 0);

                    // will not use this, but must contain a valid value:
                    imgui_descriptor_set.s_textures.push_back(related_context.font_texture->view);
                  }
                }, current_texture_id.variant);
              }
            }
          }
          imgui_descriptor_set.s_textures.push_back(related_context.font_texture->view);
          imgui_descriptor_set.update_descriptor_set(hctx);

          // Will project scissor/clipping rectangles into framebuffer space
          const ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
          const ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

          // Render command lists
          // (Because we merged all buffers into a single one, we maintain our own offset into them)
          int global_vtx_offset = 0;
          int global_idx_offset = 0;

          uint32_t im_texture_index = 0;
          pipeline_barrier(cbr, ps.backbuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
          begin_rendering(cbr, ps.backbuffer, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
          for (int n = 0; n < draw_data->CmdListsCount; n++)
          {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
              uint32_t texture_index = im_texture_index;
              ++im_texture_index;

              if (auto it = im_to_tm_index.find(texture_index); it != im_to_tm_index.end())
              {
                texture_index = it->second | 0x80000000; // marker that the resource is from the texture manager
              }
              {
                const glm::vec2 scale = glm::vec2(2.0f / draw_data->DisplaySize.x, 2.0f / draw_data->DisplaySize.y);
                setup_renderstate(cbr, ps, scale, -1.0f - glm::vec2(draw_data->DisplayPos.x, draw_data->DisplayPos.y) * scale, {fb_width, fb_height}, cmd_i == 0, texture_index);
              }

              const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
              if (pcmd->UserCallback != NULL)
              {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                {
                  const glm::vec2 scale = glm::vec2(2.0f / draw_data->DisplaySize.x, 2.0f / draw_data->DisplaySize.y);
                  setup_renderstate(cbr, ps, scale, -1.0f - glm::vec2(draw_data->DisplayPos.x, draw_data->DisplayPos.y) * scale, {fb_width, fb_height}, cmd_i == 0, texture_index);
                  // last_texture_id = nullptr;
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


                cbr.bind_descriptor_set(hctx, imgui_descriptor_set);
                cbr.bind_descriptor_set(hctx, hctx.textures.get_descriptor_set());

                cbr.set_scissor(scissor);
                cbr.draw_indexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
              }
            }
            global_idx_offset += cmd_list->IdxBuffer.Size;
            global_vtx_offset += cmd_list->VtxBuffer.Size;
          }
          cbr.end_rendering();

          hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.gqueue), std::move(imgui_descriptor_set.reset()));
        }
        cmd_buf.end_recording();

        si.on(hctx.gqueue).execute(cmd_buf);
        hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.gqueue), std::move(cmd_buf));
      }
      void setup_renderstate(vk::command_buffer_recorder& cbr, prepare_state_t& ps, glm::vec2 scale, glm::vec2 translate, glm::ivec2 fb_size, bool do_sample_back, uint32_t texture_index)
      {
        TRACY_SCOPED_ZONE;
        cbr.bind_graphics_pipeline(hctx.ppmgr, "imgui::pipeline"_rid, vk::specialization
          {{
#ifdef N_IMGUI_BLUR
            { "sample_backbuffer"_rid, (uint32_t)do_sample_back },
#else
            { "sample_backbuffer"_rid, (uint32_t)false },
#endif
          }}
        );
        const vk::pipeline_layout& pipeline_layout = hctx.ppmgr.get_pipeline_layout("imgui::pipeline"_rid);
        //         cbr.bind_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, { &related_context.font_texture_ds });

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
        cbr.bind_vertex_buffer(ps.vertex_buffer.buffer, 0);
        cbr.bind_index_buffer(ps.index_buffer.buffer, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

        cbr.push_constants(pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, 0, imgui_push_constants
        {
          .scale = scale,
          .translate = translate,
          .font_texture_index = texture_index,
        });
      }
    private:
      imgui_context& related_context;
      ImGuiViewport* imgui_viewport = nullptr;

      friend gpu_task_producer_provider_t;
  };
}

