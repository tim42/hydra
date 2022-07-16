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
    hydra::vk::descriptor_set_layout ds_layout =
    {
      context.device,
      {
        neam::hydra::vk::descriptor_set_layout_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
        neam::hydra::vk::descriptor_set_layout_binding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
      }
    };
    hydra::vk::pipeline_layout p_layout =
    {
      context.device, { &ds_layout }
    };

    prs.create_pipeline_info
    (
      std::move(ds_layout),
      hydra::vk::descriptor_pool
      {
        context.device, 100,
        {
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        }
      },
      std::move(p_layout)
    );

    hydra::vk::pipeline_creator& pcr = prs.get_pipeline_creator();
    pcr.get_pipeline_shader_stage()
      .add_shader(context.shmgr.load_shader("shaders/2d_plane.hsf:spirv(main_vs)"_rid), VK_SHADER_STAGE_VERTEX_BIT)
      .add_shader(context.shmgr.load_shader("shaders/2d_plane.hsf:spirv(main_fs)"_rid), VK_SHADER_STAGE_FRAGMENT_BIT);

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
          vk_render_pass(context.device),
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
           uniform_buffer(context.device, neam::hydra::vk::buffer(context.device, 100, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
      {
      }

      void setup(hydra::render_pass_context& rpctx) override
      {
        screen_resolution = (glm::vec2)rpctx.output_size;

        //////////////////////////////////////////////////////////////////////////////
        // create the render pass
        vk_render_pass.create_subpass().add_attachment(neam::hydra::vk::subpass::attachment_type::color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
        vk_render_pass.create_subpass_dependency(VK_SUBPASS_EXTERNAL, 0)
          .dest_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
          .source_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0);

        vk_render_pass.create_attachment().set_format(rpctx.output_format).set_samples(VK_SAMPLE_COUNT_1_BIT)
          .set_load_op(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
          .set_store_op(VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
          .set_layouts(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        vk_render_pass.refresh();

        //////////////////////////////////////////////////////////////////////////////
        // setup the mesh
        mesh.add_buffer(sizeof(indices[0]) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        mesh.add_buffer(sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        mesh.vertex_input_state() = dummy_vertex::get_vertex_input_state();
        mesh.allocate_memory(context.allocator);

        mesh.transfer_data(context.transfers, 0, sizeof(indices[0]) * indices.size(), indices.data(), context.gqueue.get_queue_familly_index());
        mesh.transfer_data(context.transfers, 1, sizeof(vertices[0]) * vertices.size(), vertices.data(), context.gqueue.get_queue_familly_index());

        //////////////////////////////////////////////////////////////////////////////
        // allocate memory for the image (+ transfer data to it)
        {
          hydra_logo_img_allocation = context.allocator.allocate_memory(hydra_logo_img.get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, neam::hydra::allocation_type::persistent_optimal_image);
          hydra_logo_img.bind_memory(*hydra_logo_img_allocation.mem(), hydra_logo_img_allocation.offset());

          uint8_t *pixels = new uint8_t[logo_size * logo_size * 4];
          neam::hydra::generate_rgba_logo(pixels, logo_size, 5);
          context.transfers.add_transfer(hydra_logo_img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, logo_size * logo_size * 4, pixels);
          delete[] pixels;
        }
        //////////////////////////////////////////////////////////////////////////////
        // allocate memory for the uniform buffer (+ transfer data to it)
        {
          uniform_buffer.set_transfer_info(context.transfers, context.tqueue, context.vrd);
          uniform_buffer_allocation = context.allocator.allocate_memory(uniform_buffer.get_buffer().get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, neam::hydra::allocation_type::persistent);

          uniform_buffer.get_buffer().bind_memory(*uniform_buffer_allocation.mem(), uniform_buffer_allocation.offset());

          size_t offset = 0;
          offset = uniform_buffer.watch(time, offset, neam::hydra::buffer_layout::std140);
          offset = uniform_buffer.watch(screen_resolution, offset, neam::hydra::buffer_layout::std140);
        }

        hydra_logo_img_view = decltype(hydra_logo_img_view)(new neam::hydra::vk::image_view(context.device, hydra_logo_img, VK_IMAGE_VIEW_TYPE_2D));


        // create the pipeline
        context.ppmgr.add_pipeline("hydra-logo"_rid, [this](auto& p) { make_fs_quad_pipeline(context, p); });
        descriptor_set = context.ppmgr.allocate_descriptor_set("hydra-logo"_rid);

        // update ds
        descriptor_set.write_descriptor_set(0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {{ sampler, *hydra_logo_img_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }});
        descriptor_set.write_descriptor_set(1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {{ uniform_buffer, uniform_buffer.get_buffer_offset(), uniform_buffer.get_area_size() }});
      }

      void prepare(hydra::render_pass_context& rpctx) override
      {
        time = cr::chrono::now_relative();
        screen_resolution = (glm::vec2)rpctx.output_size;

        uniform_buffer.sync();
      }

      hydra::render_pass_output submit(hydra::render_pass_context& rpctx) override
      {
        hydra::vk::command_buffer cmd_buf = graphic_transient_cmd_pool.create_command_buffer();
        hydra::vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        cbr.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
        {
          neam::hydra::vk::buffer_memory_barrier::queue_transfer(uniform_buffer.get_buffer(), context.transfers.get_queue_familly_index(), context.gqueue.get_queue_familly_index()),
        });

        cbr.begin_render_pass(vk_render_pass, rpctx.final_fb, hydra::vk::rect2D(glm::ivec2(0, 0), (glm::ivec2)screen_resolution), VK_SUBPASS_CONTENTS_INLINE, { glm::vec4(0.0f, 0x89 / 255.f, 1.0f, 0.f) });
        cbr.bind_pipeline(context.ppmgr.get_pipeline("hydra-logo"_rid, vk_render_pass, 0, mesh));
        cbr.set_viewport({rpctx.viewport}, 0, 1);
        cbr.set_scissor(rpctx.viewport_rect);

        cbr.bind_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, context.ppmgr.get_pipeline_layout("hydra-logo"_rid), 0, { &descriptor_set });
        mesh.bind(cbr);
        // TODO: a draw state, like in YägGLer
        cbr.draw_indexed(indices.size(), 1, 0, 0, 0);
        cbr.end_render_pass();
        cmd_buf.end_recording();

        return { .graphic = cr::construct<std::vector>(std::move(cmd_buf)) };
      }

    private:
      neam::hydra::vk::render_pass vk_render_pass;

      neam::hydra::mesh mesh;

      neam::hydra::vk::descriptor_set descriptor_set { context.device, VkDescriptorSet(nullptr) };

      neam::hydra::vk::image hydra_logo_img;
      neam::hydra::memory_allocation hydra_logo_img_allocation;
      neam::hydra::vk::sampler sampler;

      std::unique_ptr<neam::hydra::vk::image_view> hydra_logo_img_view;

      neam::hydra::auto_buffer uniform_buffer;
      neam::hydra::memory_allocation uniform_buffer_allocation;

      // uniforms
      float time = 0.5f;
      glm::vec2 screen_resolution = 900_vec2_xy;

  };
}
