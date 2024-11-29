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

namespace neam
{
  inline void make_fs_quad_pipeline(hydra::hydra_context& context, hydra::pipeline_render_state& prs)
  {
    hydra::vk::graphics_pipeline_creator& pcr = prs.get_graphics_pipeline_creator();
    pcr.get_pipeline_shader_stage()
      .add_shader(context.shmgr.load_shader("shaders/2d_plane.hsf:spirv(main_vs)"_rid))
      .add_shader(context.shmgr.load_shader("shaders/2d_plane.hsf:spirv(main_fs)"_rid));

    pcr.get_viewport_state()
      .set_dynamic_viewports_count(1)
      .set_dynamic_scissors_count(1);

    pcr.get_pipeline_color_blending_state().add_attachment_color_blending(neam::hydra::vk::attachment_color_blending::create_alpha_blending());
  }

  class fs_quad_pass : public hydra::render_pass
  {
    public:
      static constexpr unsigned int logo_size = 1024;
      fs_quad_pass(hydra::hydra_context& _context)
        : hydra::render_pass(_context),
          mesh(context.device),

          hydra_logo_img
          (
            neam::hydra::vk::image::create_image_arg
            (
              context.device,
              neam::hydra::vk::image_2d
              (
                {logo_size, logo_size}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
              )
            )
          ),
          sampler(context.device, VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.f, 0.f, 0.f),
          uniform_buffer(context.device, sizeof(fs_quad_ubo), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
      {
      }

      void setup(hydra::render_pass_context& rpctx) override
      {
        //////////////////////////////////////////////////////////////////////////////
        // setup the mesh
        mesh.add_buffer(sizeof(indices[0]) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        mesh.add_buffer(sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        mesh.vertex_input_state() = dummy_vertex::get_vertex_input_state();
        mesh.allocate_memory(context.allocator);

        mesh.transfer_data(rpctx.transfers, 0, raw_data::allocate_from(indices), context.gqueue);
        mesh.transfer_data(rpctx.transfers, 1, raw_data::allocate_from(vertices), context.gqueue);

        //////////////////////////////////////////////////////////////////////////////
        // allocate memory for the image (+ transfer data to it)
        {
          hydra_logo_img_allocation = context.allocator.allocate_memory(hydra_logo_img.get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, neam::hydra::allocation_type::persistent_optimal_image);
          hydra_logo_img.bind_memory(*hydra_logo_img_allocation.mem(), hydra_logo_img_allocation.offset());
          hydra_logo_img._set_debug_name("fs-quad/hydra_logo_img");

          raw_data pixels = raw_data::allocate(logo_size * logo_size * 4);
          neam::hydra::generate_rgba_logo((uint8_t*)pixels.get(), logo_size, 5, 0xFFFFFF);
          rpctx.transfers.acquire(hydra_logo_img);
          rpctx.transfers.transfer(hydra_logo_img, std::move(pixels));
          rpctx.transfers.release(hydra_logo_img, context.gqueue, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        //////////////////////////////////////////////////////////////////////////////
        // allocate memory for the uniform buffer (+ transfer data to it)
        {
          uniform_buffer_allocation = context.allocator.allocate_memory(uniform_buffer.get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, neam::hydra::allocation_type::persistent);

          uniform_buffer.bind_memory(*uniform_buffer_allocation.mem(), uniform_buffer_allocation.offset());
          uniform_buffer._set_debug_name("fs-quad/uniform_buffer");
        }

        hydra_logo_img_view = decltype(hydra_logo_img_view)(new neam::hydra::vk::image_view(context.device, hydra_logo_img, VK_IMAGE_VIEW_TYPE_2D));


        // create the pipeline
        context.ppmgr.add_pipeline("hydra-logo"_rid, [this](auto& p) { make_fs_quad_pipeline(context, p); });

        // logo_index = context.textures.request_texture_index("images/hydra-logo.png.xz/hydra-logo.png:image"_rid);
        logo_index = context.textures.request_texture_index("images/hydra-logo-square.png:image"_rid);
      }

      void prepare(hydra::render_pass_context& rpctx) override
      {
        // prevent the logo from being streamed-out
        context.textures.indicate_texture_usage(logo_index, 0);

        rpctx.transfers.acquire(uniform_buffer, context.gqueue);
        rpctx.transfers.transfer(uniform_buffer, raw_data::duplicate(
        fs_quad_ubo
        {
          .time = (float)cr::chrono::now_relative(),
          .screen_resolution = (glm::vec2)rpctx.output_size,
          .logo_index = context.textures.texture_index_to_gpu_index(logo_index),
        }));
        rpctx.transfers.release(uniform_buffer, context.gqueue);

        {
          descriptor_set.tex_sampler = {*hydra_logo_img_view, sampler};
          descriptor_set.ubo = uniform_buffer;
          descriptor_set.update_descriptor_set(context);
        }
      }

      hydra::render_pass_output submit(hydra::render_pass_context& rpctx) override
      {
        hydra::vk::command_buffer cmd_buf = context.gcpm.get_pool().create_command_buffer();
        cmd_buf._set_debug_name("fs-quad::command_buffer");
        hydra::vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        // if (0)
        {
          neam::hydra::vk::cbr_debug_marker _dm(cbr, "fsquad");

  //         cbr.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
  //         {
  //           neam::hydra::vk::buffer_memory_barrier::queue_transfer(uniform_buffer.get_buffer(), rpctx.transfers.get_queue_familly_index(), context.gqueue.get_queue_familly_index()),
  //         });

          rpctx.begin_rendering(cbr, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

          cbr.bind_graphics_pipeline(context.ppmgr, "hydra-logo"_rid, mesh,
                                                      hydra::vk::specialization
                                                      {{
                                                        { "loop_count_factor"_rid, float(2) },
                                                      }});
          cbr.set_viewport({rpctx.viewport}, 0, 1);
          cbr.set_scissor(rpctx.viewport_rect);

          // cbr.bind_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, context.ppmgr.get_pipeline_layout("hydra-logo"_rid), 0, { &descriptor_set });
          cbr.bind_descriptor_set(context, descriptor_set);
          cbr.bind_descriptor_set(context, context.textures.get_descriptor_set());
          mesh.bind(cbr);
          cbr.draw_indexed(indices.size(), 1, 0, 0, 0);
          cbr.end_rendering();
        }
        cmd_buf.end_recording();

        return { .graphic = cr::construct<std::vector>(std::move(cmd_buf)) };
      }

      void cleanup(hydra::render_pass_context& rpctx) override
      {
        TRACY_SCOPED_ZONE;

        context.dfe.defer_destruction(context.dfe.queue_mask(context.gqueue), descriptor_set.reset());
      }

      neam::hydra::vk::image_view* get_hydra_logo_img_view() { return hydra_logo_img_view.get(); }
    private:
      neam::hydra::mesh mesh;

      fs_quad_shader_params descriptor_set;
      // neam::hydra::vk::descriptor_set descriptor_set { context.device, VkDescriptorSet(nullptr) };

      neam::hydra::vk::image hydra_logo_img;
      neam::hydra::memory_allocation hydra_logo_img_allocation;
      neam::hydra::vk::sampler sampler;

      std::unique_ptr<neam::hydra::vk::image_view> hydra_logo_img_view;

      neam::hydra::vk::buffer uniform_buffer;
      neam::hydra::memory_allocation uniform_buffer_allocation;
      neam::hydra::texture_index_t logo_index;
  };
}
