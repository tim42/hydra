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

#include <ntools/container_utils.hpp>
#include <hydra/hydra.hpp>          // the main header of hydra

#include <hydra/renderer/ecs/gpu_task_producer.hpp>

#include "shader_struct.hpp"
// #include "app.hpp"

/// \brief A dummy vertex, just for fun
struct dummy_vertex
{
  glm::vec2 pos;
  glm::vec3 color;
  glm::vec2 uv;

  static neam::hydra::vk::pipeline_vertex_input_state get_vertex_input_state()
  {
    neam::hydra::vk::pipeline_vertex_input_state pvis;
    pvis.add_binding_description(0, sizeof(dummy_vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    pvis.add_attribute_description(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(dummy_vertex, pos));
    pvis.add_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(dummy_vertex, color));
    pvis.add_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(dummy_vertex, uv));
    return pvis;
  }
};
const std::vector<dummy_vertex> vertices =
{
    {{-1.0f, -1.0f}, {0x09 / 255.f, 1.0f, 0.0f}, {0.f, 0.f}},
    {{1.0f,  -1.0f}, {0.0f, 0x89 / 255.f, 1.0f}, {1.f, 0.f}},
    {{1.0f,   1.0f}, {0xF6 / 255.f, 0.0f, 1.0f}, {1.f, 1.f}},
    {{-1.0f,  1.0f}, {1.0f, 0x76 / 255.f, 0.0f}, {0.f, 1.f}}
};
const std::vector<uint16_t> indices =
{
    0, 1, 2, 2, 3, 0
};

namespace neam::components
{
  inline void make_fs_quad_pipeline(hydra::hydra_context& hctx, hydra::pipeline_render_state& prs)
  {
    hydra::vk::graphics_pipeline_creator& pcr = prs.get_graphics_pipeline_creator();
    pcr.get_pipeline_shader_stage()
      .add_shader(hctx.shmgr.load_shader("shaders/2d_plane.hsf:spirv(main_vs)"_rid))
      .add_shader(hctx.shmgr.load_shader("shaders/2d_plane.hsf:spirv(main_fs)"_rid));

    pcr.get_viewport_state()
      .set_dynamic_viewports_count(1)
      .set_dynamic_scissors_count(1);

    pcr.get_pipeline_color_blending_state().add_attachment_color_blending(neam::hydra::vk::attachment_color_blending{});
    // pcr.get_pipeline_color_blending_state().add_attachment_color_blending(neam::hydra::vk::attachment_color_blending::create_alpha_blending());
  }

  class fs_quad_pass : public hydra::ecs::internal_component<fs_quad_pass>, hydra::renderer::concepts::gpu_task_producer::concept_provider<fs_quad_pass>
  {
    public:
      static constexpr unsigned int logo_size = 1024;
      fs_quad_pass(param_t p, hydra::hydra_context& _hctx)
        : internal_component_t(p)
        , gpu_task_producer_provider_t(*this, _hctx)
      {
      }

    protected:
      struct setup_state_t
      {
        hydra::mesh mesh;

        hydra::vk::image hydra_logo_img;
        hydra::memory_allocation hydra_logo_img_allocation;
        hydra::vk::image_view hydra_logo_img_view;
        hydra::vk::sampler sampler;

        hydra::vk::buffer uniform_buffer;
        hydra::memory_allocation uniform_buffer_allocation;

        fs_quad_shader_params descriptor_set;

        hydra::texture_index_t logo_index;
      };

      struct prepare_state_t
      {
        hydra::renderer::exported_image backbuffer;
      };

    protected:
      setup_state_t setup(hydra::renderer::gpu_task_context& gtctx)
      {
        //////////////////////////////////////////////////////////////////////////////
        // setup the mesh
        hydra::mesh mesh(hctx.device);
        mesh.add_buffer(sizeof(indices[0]) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        mesh.add_buffer(sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        mesh.vertex_input_state() = dummy_vertex::get_vertex_input_state();
        mesh.allocate_memory(hctx.allocator);

        mesh.transfer_data(gtctx.transfers, 0, raw_data::allocate_from(indices), hctx.gqueue);
        mesh.transfer_data(gtctx.transfers, 1, raw_data::allocate_from(vertices), hctx.gqueue);

        //////////////////////////////////////////////////////////////////////////////
        // allocate memory for the image (+ transfer data to it)
        hydra::vk::image hydra_logo_img
        (
          neam::hydra::vk::image::create_image_arg
          (
            hctx.device,
            neam::hydra::vk::image_2d
            (
              {logo_size, logo_size}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            )
          )
        );
        hydra::memory_allocation hydra_logo_img_allocation;
        {
          hydra_logo_img_allocation = hctx.allocator.allocate_memory(hydra_logo_img.get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, neam::hydra::allocation_type::persistent_optimal_image);
          hydra_logo_img.bind_memory(*hydra_logo_img_allocation.mem(), hydra_logo_img_allocation.offset());

          raw_data pixels = raw_data::allocate(logo_size * logo_size * 4);
          neam::hydra::generate_rgba_logo((uint8_t*)pixels.get(), logo_size, 5, 0xFFFFFF);
          gtctx.transfers.acquire(hydra_logo_img);
          gtctx.transfers.transfer(hydra_logo_img, std::move(pixels));
          gtctx.transfers.release(hydra_logo_img, hctx.gqueue, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        hydra::vk::image_view hydra_logo_img_view(hctx.device, hydra_logo_img, VK_IMAGE_VIEW_TYPE_2D);
        hydra::vk::sampler sampler(hctx.device, VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.f, 0.f, 0.f);

        //////////////////////////////////////////////////////////////////////////////
        // allocate memory for the uniform buffer (+ transfer data to it)
        //////////////////////////////////////////////////////////////////////////////
        // allocate memory for the uniform buffer (+ transfer data to it)
        neam::hydra::memory_allocation uniform_buffer_allocation;
        neam::hydra::vk::buffer uniform_buffer(hctx.device, sizeof(fs_quad_ubo), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        {
          uniform_buffer_allocation = hctx.allocator.allocate_memory(uniform_buffer.get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, neam::hydra::allocation_type::persistent);

          uniform_buffer.bind_memory(*uniform_buffer_allocation.mem(), uniform_buffer_allocation.offset());
          uniform_buffer._set_debug_name("fs-quad/uniform_buffer");
        }

        // create the pipeline
        hctx.ppmgr.add_pipeline("hydra-logo"_rid, [this](auto& p) { make_fs_quad_pipeline(hctx, p); });
        hydra::texture_index_t logo_index = hctx.textures.request_texture_index("images/hydra-logo-square.png:image"_rid);

        return
        {
          .mesh = std::move(mesh),
          .hydra_logo_img = std::move(hydra_logo_img),
          .hydra_logo_img_allocation = std::move(hydra_logo_img_allocation),
          .hydra_logo_img_view = std::move(hydra_logo_img_view),
          .sampler = std::move(sampler),
          .uniform_buffer = std::move(uniform_buffer),
          .uniform_buffer_allocation = std::move(uniform_buffer_allocation),
          .logo_index = logo_index,
        };
      }

      prepare_state_t prepare(hydra::renderer::gpu_task_context& gtctx, setup_state_t& st)
      {
        hctx.textures.indicate_texture_usage(st.logo_index, 0);

        const hydra::renderer::viewport_context& vc = get_viewport_context();

        gtctx.transfers.acquire(st.uniform_buffer, hctx.gqueue);
        gtctx.transfers.transfer(st.uniform_buffer, raw_data::duplicate(fs_quad_ubo
        {
          .time = (float)cr::chrono::now_relative(),
          .screen_resolution = (glm::vec2)vc.size,
          //.screen_resolution = (glm::vec2{10, 10}),
          .logo_index = hctx.textures.texture_index_to_gpu_index(st.logo_index),
        }));
        gtctx.transfers.release(st.uniform_buffer, hctx.gqueue);

        {
          st.descriptor_set.tex_sampler = {st.hydra_logo_img_view, st.sampler};
          st.descriptor_set.ubo = st.uniform_buffer;
          st.descriptor_set.update_descriptor_set(hctx);
        }

        return
        {
          .backbuffer = import_image(hydra::renderer::k_context_final_output, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
        };
      }

      void submit(hydra::renderer::gpu_task_context& gtctx, hydra::vk::submit_info& si, setup_state_t& st, prepare_state_t& pt)
      {
        hydra::vk::command_buffer cmd_buf = hctx.gcpm.get_pool().create_command_buffer();
        {
          const hydra::renderer::viewport_context& vc = get_viewport_context();
          hydra::vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

          pipeline_barrier(cbr, pt.backbuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
          begin_rendering(cbr, pt.backbuffer, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

          cbr.bind_graphics_pipeline(hctx.ppmgr, "hydra-logo"_rid, st.mesh, hydra::vk::specialization
          {{
            { "loop_count_factor"_rid, float(2) },
          }});
          cbr.set_viewport({vc.viewport}, 0, 1);
          cbr.set_scissor(vc.viewport_rect);

//          cbr.push_descriptor_set(hctx, st.descriptor_set);
          cbr.bind_descriptor_set(hctx, st.descriptor_set);
          cbr.bind_descriptor_set(hctx, hctx.textures.get_descriptor_set());

          st.mesh.bind(cbr);
          cbr.draw_indexed(indices.size(), 1, 0, 0, 0);
          cbr.end_rendering();
        }
        cmd_buf.end_recording();

        si.on(hctx.gqueue).execute(cmd_buf);
        hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.gqueue), std::move(cmd_buf));
      }

    private:
      friend gpu_task_producer_provider_t;
  };
}
