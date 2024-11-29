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
#include <hydra/renderer/generic_shaders/blur.hpp>
#include <hydra/engine/hydra_context.hpp>
#include <hydra/utilities/holders.hpp>

#include "imgui_context.hpp"
#include "shader_structs.hpp"
#include "imgui_drawdata.hpp"
#include <ntools/container_utils.hpp>
#include <ntools/tracy.hpp>

// #define N_IMGUI_BLUR 1 // FIXME: validation errors
// #define N_FULLSIZE_BLUR 1 // FIXME: validation errors

namespace neam::hydra::imgui
{
  class render_pass : public neam::hydra::render_pass
  {
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

    public:
      render_pass(imgui_context& _related_context, hydra_context& _context, ImGuiViewport* _imgui_viewport)
        : hydra::render_pass(_context),
          related_context(_related_context),
          imgui_viewport(_imgui_viewport)
      {
      }

      void setup_dependencies()
      {
#if 0
        // Illegal dependencies: a pass that has been depended-on as global that then depend on something local
        // Global passes have no render-context (no group or local context).
        // All the prepare() / cleanup() calls are done in dependency order, parallelizing what can be, all the submit calls are done in no specific order
        // (but the result is then rearranged to fit the dependency order. This is so we can run them in parallel)
        //
        // Dependency management is simple in nature, we have three active deps context: global, group, local.
        // Global: executed once for the full engine. Group: executed once for a group of render-contexts. Local: per render-context.
        // If a pass depend on a global/group/local one, and if that pass is not already in the global/group/local context
        // it is created, and setup_dependencies is called on it and then added at the end of the global/group/local context.

        depend_on<imgui_font_setup>(render_pass_rate_t::group);
        depend_on<imgui_vertex_upload>(render_pass_rate_t::local);

        add_dependency<imgui_vertex_upload, imgui_font_setup>(render_pass_rate_t::local);
#endif
      }

      void setup(render_pass_context& rpctx) override
      {
        TRACY_SCOPED_ZONE;
        context.ppmgr.add_pipeline<shaders::blur>(context);

        // create the pipeline
        context.ppmgr.add_pipeline("imgui::pipeline"_rid, [this](auto& p) { make_imgui_pipeline(context, related_context, p); });

       // if (related_context.font_texture_ds._get_vk_descritpor_set() == nullptr)
         // related_context.font_texture_ds = context.ppmgr.allocate_descriptor_set("imgui::pipeline"_rid);
      }

      void prepare(render_pass_context& rpctx) override
      {
        TRACY_SCOPED_ZONE;
        check::on_vulkan_error::n_assert(related_context.is_current_context(), "Trying to draw an imgui context when it's not the current one. There might be an error somewhere before.");

        {
          static spinlock lock;
          std::lock_guard _lg(lock); // FIXME
          if (related_context.should_regenerate_fonts())
            setup_font_texture(rpctx);
          related_context.set_font_as_regenerated();
        }

        const ImDrawData* draw_data = nullptr;
        if (imgui_viewport->RendererUserData)
          draw_data = &(((draw_data_t*)imgui_viewport->RendererUserData)->draw_data);
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

        state.vertex_buffer.emplace(context.allocator, vk::buffer(context.device, vertex_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), hydra::allocation_type::short_lived);
        state.vertex_buffer->buffer._set_debug_name("imgui::vertex-buffer");
        state.index_buffer.emplace(context.allocator, vk::buffer(context.device, index_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT), hydra::allocation_type::short_lived);
        state.index_buffer->buffer._set_debug_name("imgui::index-buffer");

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

          rpctx.transfers.transfer(state.vertex_buffer->buffer, std::move(vtx_dst));
          rpctx.transfers.release(state.vertex_buffer->buffer, context.gqueue);

          rpctx.transfers.transfer(state.index_buffer->buffer, std::move(idx_dst));
          rpctx.transfers.release(state.index_buffer->buffer, context.gqueue);
        }

#ifdef N_IMGUI_BLUR
        // allocate the backbuffer_copy textures:
        {
          state.backbuffer_copy.emplace
          (
            context, vk::image::create_image_arg
            (
              context.device,
              neam::hydra::vk::image_2d
              (
                {rpctx.output_size.x / 2, rpctx.output_size.y / 2}, rpctx.output_image(0).get_image_format(), VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
              )
            ),
            hydra::allocation_type::scoped_optimal_image
          );
          state.backbuffer_copy_temp.emplace
          (
            context, vk::image::create_image_arg
            (
              context.device,
              neam::hydra::vk::image_2d
              (
                {rpctx.output_size.x / 2, rpctx.output_size.y / 2}, rpctx.output_image(0).get_image_format(), VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
              )
            ),
            hydra::allocation_type::scoped_optimal_image
          );
        }
#endif
      }

      render_pass_output submit(render_pass_context& rpctx) override
      {
        return draw(rpctx);
      }

    public: // imgui related:
      void setup_font_texture(render_pass_context& rpctx)
      {
        TRACY_SCOPED_ZONE;
        // unalloc the texture and its mem allocation
        if (related_context.font_texture)
        {
          context.dfe.defer_destruction(std::move(*related_context.font_texture));
          related_context.font_texture.reset();
        }

        unsigned char* pixels;
        int width, height;
        related_context.get_io().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        size_t upload_size = width * height * 4 * sizeof(char);
        raw_data pixel_data = raw_data::allocate(upload_size);
        memcpy(pixel_data.get(), pixels, pixel_data.size);

        cr::out().debug("imgui: generated a {} x {} font texture", width, height);

        // create the texture:
        related_context.font_texture.emplace
        (
          context.allocator, context.device, vk::image::create_image_arg
          (
            context.device,
            neam::hydra::vk::image_2d
            (
              {width, height}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            )
          )
        );
        related_context.font_texture->image._set_debug_name("imgui::font-texture");
        related_context.font_texture->view._set_debug_name("imgui::font-texture[view]");

        // upload the data to the texture:
        rpctx.transfers.acquire(related_context.font_texture->image, VK_IMAGE_LAYOUT_UNDEFINED);
        rpctx.transfers.transfer(related_context.font_texture->image, std::move(pixel_data));
        rpctx.transfers.release(related_context.font_texture->image, context.gqueue, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // update ds
//        rpctx.vrd.postpone_destruction_to_next_fence(context.gqueue, std::move(related_context.font_texture_ds));
//         related_context.font_texture_ds = context.ppmgr.allocate_descriptor_set("imgui::pipeline"_rid);
//
//         related_context.font_texture_ds.write_descriptor_set(0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//         {
//           { related_context.font_texture->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
//           { related_context.font_texture->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
//         });
      }

      render_pass_output draw(render_pass_context& rpctx)
      {
        TRACY_SCOPED_ZONE;
        // nothing to do
        if (!state.vertex_buffer || !state.index_buffer)
          return {};

        const ImDrawData* draw_data = nullptr;
        if (imgui_viewport->RendererUserData)
          draw_data = &(((draw_data_t*)imgui_viewport->RendererUserData)->draw_data);
        if (!draw_data)
          return {};

        const int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
        const int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
          return {};

        vk::command_buffer cmd_buf = context.gcpm.get_pool().create_command_buffer();
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
                      texture_index = context.textures.request_texture_index(string_id::_from_id_t(v));
                    else if (std::is_same_v<T, unsigned>)
                      texture_index = v;

                    im_to_tm_index.emplace(im_texture_index - 1, context.textures.texture_index_to_gpu_index(texture_index));
                    // prevent the texture from being streamed-out
                    context.textures.indicate_texture_usage(texture_index, 0);

                    // will not use this, but must contain a valid value:
                    imgui_descriptor_set.s_textures.push_back(related_context.font_texture->view);
                  }
                }, current_texture_id.variant);
              }
            }
          }
          imgui_descriptor_set.s_textures.push_back(related_context.font_texture->view);
          imgui_descriptor_set.update_descriptor_set(context);

          // Will project scissor/clipping rectangles into framebuffer space
          const ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
          const ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

          // Render command lists
          // (Because we merged all buffers into a single one, we maintain our own offset into them)
          int global_vtx_offset = 0;
          int global_idx_offset = 0;

          uint32_t im_texture_index = 0;
          rpctx.begin_rendering(cbr, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
          for (int n = 0; n < draw_data->CmdListsCount; n++)
          {
#ifdef N_IMGUI_BLUR
            cbr.pipeline_barrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            {
              vk::image_memory_barrier{rpctx.output_image(0), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT },
              vk::image_memory_barrier{state.backbuffer_copy->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT },
            });

  //           cbr.copy_image(rpctx.output_image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, state.backbuffer_copy->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
  //           {
  //             {{0, 0}, {rpctx.output_size}, {0, 0}}
  //           });
            cbr.blit_image(rpctx.output_image(0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, state.backbuffer_copy->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            {
              {{0, 0}, {rpctx.output_size}, {0, 0}, {state.backbuffer_copy->image.get_size().x, state.backbuffer_copy->image.get_size().y},},
            });

            shaders::blur::image_memory_barrier_pre(cbr, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                    state.backbuffer_copy->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
                                                    state.backbuffer_copy_temp->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_NONE,
                                                    state.backbuffer_copy->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT // ignored
                                                  );

            const float blur_strength = 8 * related_context.get_content_scale();
            shaders::blur::blur_image(context, rpctx, cbr, state.backbuffer_copy->view, state.backbuffer_copy_temp->view,
                                      {state.backbuffer_copy->image.get_size().x, state.backbuffer_copy->image.get_size().y}, blur_strength, true);

            shaders::blur::image_memory_barrier_internal(cbr, state.backbuffer_copy->image, state.backbuffer_copy_temp->image, state.backbuffer_copy->image);

            shaders::blur::blur_image(context, rpctx, cbr, state.backbuffer_copy_temp->view, state.backbuffer_copy->view,
                                      {state.backbuffer_copy->image.get_size().x, state.backbuffer_copy->image.get_size().y}, blur_strength, false);

            shaders::blur::image_memory_barrier_post(cbr, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                    state.backbuffer_copy->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
                                                    state.backbuffer_copy->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, // ignored
                                                    state.backbuffer_copy->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT  // ignored
                                                  );
            cbr.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
            {
              vk::image_memory_barrier{rpctx.output_image(0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT },
  //             vk::image_memory_barrier{backbuffer_copy->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT },
            });

#ifdef N_FULLSIZE_BLUR
            shaders::blur::image_memory_barrier_pre(cbr, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                    rpctx.output_image(0), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                    state.backbuffer_copy_temp->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_NONE,
                                                    state.backbuffer_copy->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_NONE
                                                  );

            shaders::blur::blur_image(context, rpctx, cbr, *rpctx.output().get_image_view_vector()[0], state.backbuffer_copy_temp->view,
                                      rpctx.output_size, 4, true);

            shaders::blur::image_memory_barrier_internal(cbr, rpctx.output_image(0), state.backbuffer_copy_temp->image, state.backbuffer_copy->image);

            shaders::blur::blur_image(context, rpctx, cbr, state.backbuffer_copy_temp->view, state.backbuffer_copy->view,
                                      rpctx.output_size, 4, false);

            shaders::blur::image_memory_barrier_post(cbr, (VkPipelineStageFlagBits)(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT),
                                                    rpctx.output_image(0), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                    rpctx.output_image(0), VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_NONE, // ignored
                                                    state.backbuffer_copy->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT
                                                  );
#endif
#endif


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
                setup_renderstate(cbr, scale, -1.0f - glm::vec2(draw_data->DisplayPos.x, draw_data->DisplayPos.y) * scale, {fb_width, fb_height}, cmd_i == 0, texture_index);
              }

              const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
              if (pcmd->UserCallback != NULL)
              {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                {
                  const glm::vec2 scale = glm::vec2(2.0f / draw_data->DisplaySize.x, 2.0f / draw_data->DisplaySize.y);
                  setup_renderstate(cbr, scale, -1.0f - glm::vec2(draw_data->DisplayPos.x, draw_data->DisplayPos.y) * scale, {fb_width, fb_height}, cmd_i == 0, texture_index);
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


                cbr.bind_descriptor_set(context, imgui_descriptor_set);
                cbr.bind_descriptor_set(context, context.textures.get_descriptor_set());

                cbr.set_scissor(scissor);
                cbr.draw_indexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
              }
            }
            global_idx_offset += cmd_list->IdxBuffer.Size;
            global_vtx_offset += cmd_list->VtxBuffer.Size;
          }
          cbr.end_rendering();

          context.dfe.defer_destruction(context.dfe.queue_mask(context.gqueue), std::move(imgui_descriptor_set.reset()));
        }
        cmd_buf.end_recording();

        return { .graphic = cr::construct<std::vector>(std::move(cmd_buf)) };
      }

      void cleanup(render_pass_context& rpctx) override
      {
        TRACY_SCOPED_ZONE;

        // Flush the state:
        context.dfe.defer_destruction(context.dfe.queue_mask(context.gqueue), std::move(state));
      }

    private: // rendering stuff
      void setup_renderstate(vk::command_buffer_recorder& cbr, glm::vec2 scale, glm::vec2 translate, glm::ivec2 fb_size, bool do_sample_back, uint32_t texture_index)
      {
        TRACY_SCOPED_ZONE;
        cbr.bind_graphics_pipeline(context.ppmgr, "imgui::pipeline"_rid, vk::specialization
          {{
#ifdef N_IMGUI_BLUR
            { "sample_backbuffer"_rid, (uint32_t)do_sample_back },
#else
            { "sample_backbuffer"_rid, (uint32_t)false },
#endif
          }}
        );
        const vk::pipeline_layout& pipeline_layout = context.ppmgr.get_pipeline_layout("imgui::pipeline"_rid);
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
        cbr.bind_vertex_buffer(state.vertex_buffer->buffer, 0);
        cbr.bind_index_buffer(state.index_buffer->buffer, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

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

      struct state_t
      {
        std::optional<buffer_holder> vertex_buffer;
        std::optional<buffer_holder> index_buffer;
#ifdef N_IMGUI_BLUR
        std::optional<image_holder> backbuffer_copy;
        std::optional<image_holder> backbuffer_copy_temp;
#endif
        // Temporary cache of texture ds, so we can simply pass image_view around,
        // without having to deal with full-on descriptor sets
        std::map<ImTextureID, vk::descriptor_set> textures_ds_cache;
      } state;
  };
}
