//
// created by : Timothée Feuillet
// date: 2023-10-22
//
//
// Copyright (c) 2023 Timothée Feuillet
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

#include <hydra/hydra.hpp>          // the main header of hydra
#include <hydra/engine/hydra_context.hpp>
#include <hydra/renderer/render_pass.hpp>
#include <hydra/ecs/ecs.hpp>
#include <hydra/ecs/transform.hpp>
#include <hydra/utilities/holders.hpp>

struct mesh_vertex
{
  glm::vec3 pos;
  glm::vec2 uv;

  static neam::hydra::vk::pipeline_vertex_input_state get_vertex_input_state()
  {
    neam::hydra::vk::pipeline_vertex_input_state pvis;
    pvis.add_binding_description(0, sizeof(mesh_vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    pvis.add_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(mesh_vertex, pos));
    pvis.add_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(mesh_vertex, uv));
    return pvis;
  }
};

const std::vector<mesh_vertex> cube_vertices =
{
  {{-1.f, -1.f, -1.f}, {0.f, 0.f}},
  {{+1.f, -1.f, -1.f}, {1.f, 0.f}},
  {{+1.f, +1.f, -1.f}, {1.f, 1.f}},
  {{-1.f, +1.f, -1.f}, {0.f, 1.f}},
  {{-1.f, +1.f, +1.f}, {0.f, 0.f}},
  {{+1.f, +1.f, +1.f}, {1.f, 0.f}},
  {{+1.f, -1.f, +1.f}, {1.f, 1.f}},
  {{-1.f, -1.f, +1.f}, {0.f, 1.f}},
};
const std::vector<uint16_t> cube_indices =
{
  0, 2, 1, //face front
  0, 3, 2,
  2, 3, 4, //face top
  2, 4, 5,
  1, 2, 5, //face right
  1, 5, 6,
  0, 7, 4, //face left
  0, 4, 3,
  5, 4, 7, //face back
  5, 7, 6,
  0, 6, 7, //face bottom
  0, 1, 6
};

namespace neam::hydra
{
  inline void make_cube_mesh_pipeline(hydra_context& context, pipeline_render_state& prs)
  {
    // vk::descriptor_set_layout ds_layout =
    // {
    //   context.device,
    //   {
    //     vk::descriptor_set_layout_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT),
    //    // vk::descriptor_set_layout_binding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
    //   }
    // };
    // vk::pipeline_layout p_layout =
    // {
    //   context.device, { &ds_layout },
    //   {
    //     {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16 + sizeof(uint32_t) * 3 + sizeof(uint16_t) * 4}
    //   }
    // };
    //
    // prs.create_pipeline_info
    // (
    //   std::move(ds_layout),
    //   vk::descriptor_pool
    //   {
    //     context.device, 20,
    //     {
    //       {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 20},
    //     //  {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
    //     },
    //     VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
    //   },
    //   std::move(p_layout)
    // );

    vk::graphics_pipeline_creator& pcr = prs.get_graphics_pipeline_creator();
    pcr.get_pipeline_shader_stage()
      .add_shader(context.shmgr.load_shader("shaders/3d_default.hsf:spirv(main_vs)"_rid))
      .add_shader(context.shmgr.load_shader("shaders/3d_default.hsf:spirv(main_fs)"_rid));

    pcr.get_viewport_state()
      .set_dynamic_viewports_count(1)
      .set_dynamic_scissors_count(1);

    pcr.get_pipeline_color_blending_state().add_attachment_color_blending(vk::attachment_color_blending::create_alpha_blending());
  }

  class mesh_pass : public render_pass
  {
    public:
      mesh_pass(hydra::hydra_context& _context, ecs::database& _db)
        : hydra::render_pass(_context),
          db(_db),
          mesh(context.device)
      {
      }

      void setup(hydra::render_pass_context& rpctx) override
      {
        //////////////////////////////////////////////////////////////////////////////
        // setup the mesh
        mesh.add_buffer(sizeof(cube_indices[0]) * cube_indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        mesh.add_buffer(sizeof(cube_vertices[0]) * cube_vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        mesh.vertex_input_state() = mesh_vertex::get_vertex_input_state();
        mesh.allocate_memory(context.allocator);

        mesh.transfer_data(rpctx.transfers, 0, raw_data::allocate_from(cube_indices), context.gqueue);
        mesh.transfer_data(rpctx.transfers, 1, raw_data::allocate_from(cube_vertices), context.gqueue);

        //////////////////////////////////////////////////////////////////////////////
        // create the pipeline
        context.ppmgr.add_pipeline("cube-mesh"_rid, [this](auto& p) { make_cube_mesh_pipeline(context, p); });
      }

      void prepare(hydra::render_pass_context& rpctx) override
      {
        std::vector<packed_transform> transforms;
        // FIXME: Should be done in a system, should use a "renderable" concept
        const size_t count = db.get_attached_object_count<ecs::components::transform>();
        transforms.reserve(count);
        db.for_each([&transforms](ecs::components::transform& tr)
        {
          packed_transform pt = tr.get_local_to_world_transform().pack();
          // cr::out().debug("pt: tr[{},{},{} | {},{},{}], sc:{}",
          //                 pt.grid_translation.x, pt.grid_translation.y, pt.grid_translation.z,
          //                 pt.fine_translation.x, pt.fine_translation.y, pt.fine_translation.z,
          //                 pt.scale);
          // cr::out().debug("pt: tr[{},{},{}], sc:{}",
          //                 tr.get_local_to_world_transform().translation.x, tr.get_local_to_world_transform().translation.y, tr.get_local_to_world_transform().translation.z,
          //                 pt.scale);
          transforms.push_back(pt);
        });

        if (state.instance_count != count)
          cr::out().debug("mesh-pass: instance count: {}", count);
        state.instance_count = count;

        state.transform_buffer.emplace(context.allocator, vk::buffer(context.device, std::max<uint32_t>(1u, transforms.size()) * sizeof(transforms[0]), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), hydra::allocation_type::short_lived);

        // upload to buffer:
        if (count > 0)
        {
          rpctx.transfers.acquire(state.transform_buffer->buffer, context.gqueue);
          rpctx.transfers.transfer(state.transform_buffer->buffer, raw_data::allocate_from(transforms)); // FIXME: Use raw-data directly instead of a vector
          rpctx.transfers.release(state.transform_buffer->buffer, context.gqueue);
        }

        // create the descriptor set:
        // state.descriptor_set = context.ppmgr.allocate_descriptor_set("cube-mesh"_rid, true /* allow free */);

        // update ds
        state.descriptor_set->write_descriptor_set(0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, {{ state.transform_buffer->buffer, 0, state.transform_buffer->buffer.size() }});
        //descriptor_set->write_descriptor_set(1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {{ uniform_buffer, uniform_buffer.get_buffer_offset(), uniform_buffer.get_area_size() }});
      }

      hydra::render_pass_output submit(hydra::render_pass_context& rpctx) override
      {
        hydra::vk::command_buffer cmd_buf = context.gcpm.get_pool().create_command_buffer();
        cmd_buf._set_debug_name("mesh-pass::command_buffer");
        hydra::vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        neam::hydra::vk::cbr_debug_marker _dm(cbr, "mesh-pass");

        rpctx.begin_rendering(cbr, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
        cbr.bind_graphics_pipeline(context.ppmgr, "cube-mesh"_rid, mesh,
                                                     hydra::vk::specialization
                                                     {/*{
                                                       { "loop_count_factor"_rid, float(2) },
                                                     }*/});
        cbr.set_viewport({rpctx.viewport}, 0, 1);
        cbr.set_scissor(rpctx.viewport_rect);

        const glm::mat4 projection = glm::perspective(glm::radians(90.0f), rpctx.viewport.get_aspect_ratio(), 0.001f, 5000.0f);
        cbr.push_constants(context.ppmgr.get_pipeline_layout("cube-mesh"_rid), VK_SHADER_STAGE_VERTEX_BIT, 0, projection);
        cbr.push_constants(context.ppmgr.get_pipeline_layout("cube-mesh"_rid), VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 16, glm::ivec3(0, 0, 0));
        cbr.push_constants(context.ppmgr.get_pipeline_layout("cube-mesh"_rid), VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 16 + sizeof(glm::ivec3), glm::u16vec4(0, 0, 0, 0));

        cbr.bind_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, context.ppmgr.get_pipeline_layout("cube-mesh"_rid), 0, { &*state.descriptor_set });
        mesh.bind(cbr);
        cbr.draw_indexed(cube_indices.size(), state.instance_count, 0, 0, 0);
        cbr.end_rendering();
        cmd_buf.end_recording();

        return { .graphic = cr::construct<std::vector>(std::move(cmd_buf)) };
      }

      void cleanup(render_pass_context& rpctx) override
      {
        TRACY_SCOPED_ZONE;

        // Flush the state:
        context.dfe.defer_destruction(context.dfe.queue_mask(context.gqueue), std::move(state));
      }

    private:
      ecs::database& db;
      neam::hydra::mesh mesh;

      struct state_t
      {
        std::optional<buffer_holder> transform_buffer;
        std::optional<neam::hydra::vk::descriptor_set> descriptor_set;
        size_t instance_count = 0;
      } state;
  };
}

