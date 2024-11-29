//
// created by : Timothée Feuillet
// date: 2024-2-2
//
//
// Copyright (c) 2024 Timothée Feuillet
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

#include "static_mesh_packer.hpp"

#include <meshoptimizer/src/meshoptimizer.h>
#include <glm/gtc/packing.hpp>

namespace neam::hydra::packer
{
  struct static_mesh_packer : resources::packer::packer<assets::static_mesh, static_mesh_packer>
  {
    static constexpr id_t packer_hash = "neam/static-mesh-packer:0.0.1"_rid;

    static resources::packer::chain pack_resource(hydra::core_context& /*ctx*/, resources::processor::data&& data)
    {
      TRACY_SCOPED_ZONE;
      const id_t root_id = get_root_id(data.resource_id);
      data.db.resource_name(root_id, get_root_name(data.db, data.resource_id));

      // final resources:
      assets::static_mesh root;
      std::vector<assets::static_mesh_lod> lods;
      resources::status status = resources::status::success;

      // generate the lods + meshlets:
      {
        static_mesh_packer_input in;
        const rle::status rst = rle::in_place_deserialize(std::move(data.data), in);
        if (rst == rle::status::failure)
        {
          data.db.error<static_mesh_packer>(root_id, "failed to deserialize processor data");
          return resources::packer::chain::create_and_complete({}, id_t::invalid, resources::status::failure);
        }

        root.bounding_sphere = in.bounding_sphere,

        data.db.message<static_mesh_packer>(root_id, "LOD {}: {} tri", 0, in.indices.size() / 3);

        std::vector<std::vector<uint32_t>> lod_indices;

        // push LOD 0
        lod_indices.emplace_back(std::move(in.indices));

        // generate LODs:
        constexpr size_t k_lod_count = 10;
        bool use_sloppy_simplifier = false;
        size_t previous_index_count = lod_indices[0].size();
        for (uint32_t lod_index = 1; lod_index < k_lod_count; ++lod_index)
        {
          std::vector<uint32_t>& new_indices = lod_indices.emplace_back();
          new_indices.resize(lod_indices[0].size(), 0);
          const size_t target_index_count = lod_indices[0].size() - (lod_indices[0].size() / k_lod_count) * lod_index;
          const float target_error = 0.1f;
          size_t ret = 0;
          if (!use_sloppy_simplifier)
          {
            ret = meshopt_simplify(new_indices.data(), lod_indices[0].data(), lod_indices[0].size(),
                                          (const float*)in.vertices.data(), in.vertices.size(), sizeof(in.vertices[0]),
                                          target_index_count, target_error,
                                          0 /* flags */,
                                          nullptr);
            if (ret == previous_index_count)
            {
              data.db.warning<static_mesh_packer>(root_id, "LOD {}: failed to generate LOD, switching to sloppy simplifier", lod_index);
              use_sloppy_simplifier = true;
            }
            previous_index_count = ret;
          }
          if (use_sloppy_simplifier)
          {
            ret = meshopt_simplifySloppy(new_indices.data(), lod_indices[0].data(), lod_indices[0].size(),
                                          (const float*)in.vertices.data(), in.vertices.size(), sizeof(in.vertices[0]),
                                          target_index_count, target_error,
                                          nullptr);
          }

          new_indices.resize(ret);
          data.db.message<static_mesh_packer>(root_id, "LOD {}: {} tri (target: {})", lod_index, ret / 3, target_index_count / 3);
        }

        // vertex data optim: (TODO!)
        // might not be necessary, but should help a bit the meshlets
        {
          // std::vector<uint32_t> com;
          // for (uint32_t rev_lod_index = 0; rev_lod_index < k_lod_count; ++rev_lod_index)
          // {
          //   const uint32_t lod_index = rev_lod_index - k_lod_count - 1;
          //   meshopt_optimizeVertexCache(lod_indices[lod_index].data(), lod_indices[lod_index].data(), lod_indices[lod_index].size(), in.vertices.size());
          //
          // }
        }

        std::vector<uint32_t> vertex_indirection;
        vertex_indirection.resize(in.vertices.size(), ~0u);
        for (uint32_t rev_lod_index = 0; rev_lod_index < k_lod_count; ++rev_lod_index)
        {
          const uint32_t lod_index = k_lod_count - rev_lod_index - 1;
          // skip invalid LODs
          if (lod_indices[lod_index].size() == 0)
            continue;

          constexpr size_t max_vertices = 64;
          constexpr size_t max_triangles = 124;
          constexpr float cone_weight = 0.25f;

          // generate meshlets
          const size_t max_meshlets = meshopt_buildMeshletsBound(lod_indices[lod_index].size(), max_vertices, max_triangles);
          std::vector<meshopt_Meshlet> meshlets(max_meshlets);
          std::vector<uint32_t> meshlet_vertices(max_meshlets * max_vertices);
          std::vector<uint8_t> meshlet_triangles(max_meshlets * max_triangles * 3);

          const size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), lod_indices[lod_index].data(),
          lod_indices[lod_index].size(), (const float*)in.vertices.data(), in.vertices.size(), sizeof(in.vertices[0]), max_vertices, max_triangles, cone_weight);

          const meshopt_Meshlet& last = meshlets[meshlet_count - 1];

          meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
          meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
          meshlets.resize(meshlet_count);


          // Push/create LOD entry:
          const id_t lod_id = parametrize(specialize(root_id, assets::static_mesh_lod::type_name), fmt::to_string(lod_index));
          data.db.resource_name(lod_id, fmt::format("{}:{}({})", data.db.resource_name(root_id), assets::static_mesh_lod::type_name.str, lod_index));

          assets::static_mesh_lod& lod = lods.emplace_back();
          root.lods.push_back(lod_id);

          // compute meshlet info:
          {
            std::vector<assets::packed_data::meshlet_culling_data> meshlet_culling_data;
            std::vector<assets::packed_data::meshlet_data> meshlet_data;
            for (uint32_t meshlet_index = 0; meshlet_index < meshlet_count; ++meshlet_index)
            {
              const meshopt_Meshlet& meshlet = meshlets[meshlet_index];
              const meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshlet_vertices[meshlet.vertex_offset], &meshlet_triangles[meshlet.triangle_offset],
                                                meshlet.triangle_count, (float*)in.vertices.data(), in.vertices.size(), sizeof(in.vertices[0]));
              meshlet_data.push_back(
              {
                .vertex_offset = meshlet.vertex_offset,
                .triangle_offset = meshlet.triangle_offset,
                .vertex_count = (uint16_t)meshlet.vertex_count,
                .triangle_count = (uint16_t)meshlet.triangle_count,
              });
              meshlet_culling_data.push_back(
              {
                .bounding_sphere = glm::vec4(bounds.center[0], bounds.center[1], bounds.center[2], bounds.radius),
                .cone_apex = glm::vec4(bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2], 0),
                .cone_axis_and_cutoff = glm::vec4(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2], bounds.cone_cutoff),
              });
            }
            lod.meshlet_culling_data = raw_data::allocate_from(meshlet_culling_data);
            lod.meshlet_data = raw_data::allocate_from(meshlet_data);
          }

          // repack vertex buffers (only store vertices that aren't present in any other LOD), repack index buffer (reference to vertices from other LODs)
          // (done last, so we can use the meshlet_vertices where it's needed)
          std::vector<vertex_data> lod_vertices;
          std::vector<vertex_data_stream> lod_data;
          lod_data.resize(in.data.size());
          for (uint32_t i = 0; i < meshlet_vertices.size(); ++i)
          {
            const uint32_t vertex_index = meshlet_vertices[i];
            // We are the first to claim this vertex
            if (vertex_indirection[vertex_index] == ~0u)
            {
              // Claim the vertex
              vertex_indirection[vertex_index] = lod_index << 24 | (uint32_t)lod_vertices.size();

              // Add the vertex to this LOD
              lod_vertices.emplace_back(in.vertices[vertex_index]);
              for (uint32_t data_index = 0; data_index < lod_data.size(); ++data_index)
                lod_data[data_index].data.emplace_back(in.data[data_index].data[vertex_index]);
            }

            // use the generic vertex indirection
            meshlet_vertices[i] = vertex_indirection[vertex_index];
          }

          // TODO: cpu-side add counts (in root), so we can pre-size the buffers correctly while the data is loading?

          lod.meshlet_index_data = raw_data::allocate_from(meshlet_triangles);
          lod.vertex_indirection_data = raw_data::allocate_from(meshlet_vertices);

          lod.lod_data = raw_data::duplicate(assets::packed_data::lod_data
          {
            .meshlet_count = (uint32_t)meshlet_count
          });

          // compress the vertex data:
          {
            std::vector<assets::packed_data::vertex_data> packed_vertex_data;
            packed_vertex_data.reserve(lod_vertices.size());
            for (uint32_t vertex_index = 0; vertex_index < lod_vertices.size(); ++vertex_index)
            {
              const vertex_data& vertex = lod_vertices[vertex_index];
              packed_vertex_data.push_back(
              {
                .position_tbn =
                {
                  std::bit_cast<glm::uvec3>(lod_vertices[vertex_index].position),
                  std::bit_cast<uint32_t>(glm::pack_tbn(vertex.tangent, vertex.bitangent, vertex.normal))
                },
              });
              assets::packed_data::vertex_data& packed_vertex = packed_vertex_data.back();
              // write extra colors/uv:
              uint32_t offset = 0;
              uint32_t sub_offset = 0;
              for (uint32_t data_index = 0; data_index < in.data.size() && offset < assets::packed_data::vertex_data::k_data_size; ++data_index)
              {
                if (!in.data[data_index].is_vec2)
                {
                  // FIXME: expects sub_offset to be 0.
                  packed_vertex.data[offset] = glm::packHalf(lod_data[data_index].data[vertex_index]);
                  ++offset;
                }
                else
                {
                  glm::u16vec2 d = glm::packUnorm<uint16_t>(glm::vec2{lod_data[data_index].data[vertex_index]});
                  if (sub_offset == 0)
                  {
                    packed_vertex.data[offset].z = d.x;
                    packed_vertex.data[offset].w = d.y;
                    sub_offset += 2;
                  }
                  else
                  {
                    packed_vertex.data[offset].x = d.x;
                    packed_vertex.data[offset].y = d.y;
                    sub_offset = 0;
                    ++offset;
                  }
                }
              }

              // write material data:
              packed_vertex.data[assets::packed_data::vertex_data::k_data_size - 1].w = vertex.material_index;
            }
            lod.vertex_data = raw_data::allocate_from(packed_vertex_data);
          }

          data.db.message<static_mesh_packer>(root_id, "LOD {}: {} meshlets, {} vertices, memory size: {:.3f}Mib",
                                              lod_index, meshlet_count, lod_vertices.size(),
                                              lod.total_memory_size() / 1024.0f / 1024.0f);
        }

        // all the temp data is freed there
      }

      // serialize everything + create sub-resources
      std::vector<resources::packer::data> ret;
      {
        resources::status st = resources::status::success;
        ret.emplace_back(resources::packer::data
        {
          .id = root_id,
          .data = assets::static_mesh::to_raw_data(root, st),
          .metadata = std::move(data.metadata),
        });
        status = resources::worst(status, st);
      }
      for (uint32_t i = 0; i < lods.size(); ++i)
      {
        resources::status st = resources::status::success;
        ret.emplace_back(resources::packer::data
        {
          .id = root.lods[i],
          .data = assets::static_mesh_lod::to_raw_data(lods[i], st),
          .metadata = {},
        });
        lods[i] = {}; // free-up the memory a bit
        status = resources::worst(status, st);
      }
      return resources::packer::chain::create_and_complete(std::move(ret), root_id, status);
    }
  };
}

