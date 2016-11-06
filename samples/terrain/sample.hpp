//
// file : sample.hpp
// in : file:///home/tim/projects/hydra/samples/terrain/sample.hpp
//
// created by : Timothée Feuillet
// date: Sat Nov 05 2016 18:39:18 GMT-0400 (EDT)
//
//
// Copyright (c) 2016 Timothée Feuillet
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

#ifndef __N_94954772477713296_2016124446_SAMPLE_HPP__
#define __N_94954772477713296_2016124446_SAMPLE_HPP__

#include <memory>

#include "app.hpp"

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
    {{-0.5f, -0.5f}, {0x09 / 255.f, 1.0f, 0.0f}, {0.f, 0.f}},
    {{0.5f,  -0.5f}, {0.0f, 0x89 / 255.f, 1.0f}, {1.f, 0.f}},
    {{0.5f,   0.5f}, {0xF6 / 255.f, 0.0f, 1.0f}, {1.f, 1.f}},
    {{-0.5f,  0.5f}, {1.0f, 0x76 / 255.f, 0.0f}, {0.f, 1.f}}
};
const std::vector<uint16_t> indices =
{
    0, 1, 2, 2, 3, 0
};

namespace neam
{
  class sample_app : public application
  {
    private:
      static constexpr unsigned int logo_size = 1024;

    public:
      sample_app(const glm::uvec2 &window_size, const std::string &window_name)
       : application(window_size, window_name),
         mesh(device),
         sampler_dslb(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
         sampler_ds_layout(device, {sampler_dslb}),
         ds_pool(device, 1, {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}}),
         descriptor_set(ds_pool.allocate_descriptor_set(sampler_ds_layout)),
         hydra_logo_img
         (
           neam::hydra::vk::image::create_image_arg
           (
             device,
             neam::hydra::vk::image_2d
             (
               {logo_size, logo_size}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
             )
           )
         ),
         sampler(device, VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.f, 0.f, 0.f, 16.f),
         pipeline_layout(device, { &sampler_ds_layout })
      {
        //////////////////////////////////////////////////////////////////////////////
        // create the render pass
        render_pass.create_subpass().add_attachment(neam::hydra::vk::subpass::attachment_type::color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
        render_pass.create_subpass_dependency(VK_SUBPASS_EXTERNAL, 0)
          .dest_subpass_masks(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT)
          .source_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

        render_pass.create_attachment().set_swapchain(&swapchain).set_samples(VK_SAMPLE_COUNT_1_BIT)
          .set_load_op(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
          .set_store_op(VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
          .set_layouts(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        render_pass.refresh();

        //////////////////////////////////////////////////////////////////////////////
        // setup the mesh
        mesh.add_buffer(sizeof(indices[0]) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        mesh.add_buffer(sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        mesh.get_vertex_input_state() = dummy_vertex::get_vertex_input_state();
        mesh.allocate_memory(mem_alloc);

        mesh.transfer_data(btransfers, 0, sizeof(indices[0]) * indices.size(), indices.data());
        mesh.transfer_data(btransfers, 1, sizeof(vertices[0]) * vertices.size(), vertices.data());

        //////////////////////////////////////////////////////////////////////////////
        // allocate memory for the image (+ transfer data to it)
        {
          neam::hydra::memory_allocation ma = mem_alloc.allocate_memory(hydra_logo_img.get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, neam::hydra::allocation_type::optimal_image);
          hydra_logo_img.bind_memory(*ma.mem(), ma.offset());

          uint8_t *pixels = new uint8_t[logo_size * logo_size * 4];
          neam::hydra::generate_rgba_logo(pixels, logo_size, 5);
          btransfers.add_transfer(hydra_logo_img, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, logo_size * logo_size * 4, pixels);
          delete[] pixels;

          btransfers.start(); // We can transfer buffers while the other things initialize...
        }

        hydra_logo_img_view = decltype(hydra_logo_img_view)(new neam::hydra::vk::image_view(device, hydra_logo_img, VK_IMAGE_VIEW_TYPE_2D));

        // update ds
        descriptor_set.write_descriptor_set(0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {{ sampler, *hydra_logo_img_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }});

        //////////////////////////////////////////////////////////////////////////////
        // create the pipeline
        pcr.get_pipeline_shader_stage()
          .add_shader(shmgr.load_shader("data/shaders/2d_plane.vert.spv"), VK_SHADER_STAGE_VERTEX_BIT)
          .add_shader(shmgr.load_shader("data/shaders/2d_plane.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);

        // yep.
        mesh.setup_vertex_description(pcr);

        pcr.get_viewport_state().add_viewport(&swapchain.get_full_viewport())
          .add_scissor(&swapchain.get_full_rect2D());

        pcr.get_pipeline_color_blending_state().add_attachment_color_blending(&acb);

        pcr.set_pipeline_layout(pipeline_layout);
        pcr.set_render_pass(render_pass);
        pcr.set_subpass_index(0);
        pcr.allow_derivate_pipelines(true);

        ppmgr.add_pipeline("hydra-logo", pcr);
      }

    protected: // hooks
      virtual void init_command_buffer(neam::hydra::vk::command_buffer_recorder &cbr, neam::hydra::vk::framebuffer &fb, size_t /*index*/) override
      {
        cbr.begin_render_pass(render_pass, fb, swapchain.get_full_rect2D(), VK_SUBPASS_CONTENTS_INLINE, { glm::vec4(0.0f, 0x89 / 255.f, 1.0f, 1.f) });
        cbr.bind_pipeline(ppmgr.get_pipeline("hydra-logo"));
        cbr.bind_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, pcr.get_pipeline_layout(), 0, { &descriptor_set });
        mesh.bind(cbr);
        cbr.draw_indexed(indices.size(), 1, 0, 0, 0); // TODO: a draw state, like in YägGLer
        cbr.end_render_pass();
      }

      virtual void render_loop_hook() override
      {
        
      }

    protected:
      neam::hydra::mesh mesh;

      neam::hydra::vk::descriptor_set_layout_binding sampler_dslb;
      neam::hydra::vk::descriptor_set_layout sampler_ds_layout;

      neam::hydra::vk::descriptor_pool ds_pool;
      neam::hydra::vk::descriptor_set descriptor_set;

      neam::hydra::vk::image hydra_logo_img;
      neam::hydra::vk::sampler sampler;

      neam::hydra::vk::attachment_color_blending acb = neam::hydra::vk::attachment_color_blending::create_alpha_blending();
      neam::hydra::vk::pipeline_layout pipeline_layout;
      std::unique_ptr<neam::hydra::vk::image_view> hydra_logo_img_view;

      neam::hydra::vk::pipeline_creator pcr;
  };
} // namespace neam

#endif // __N_94954772477713296_2016124446_SAMPLE_HPP__

