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

#pragma once

#include <ntools/id/string_id.hpp>
#include <ntools/raw_data.hpp>
#include <resources/asset.hpp>
#include <resources/packer.hpp>

#include <hydra_glm.hpp>


namespace neam::hydra::assets
{
  namespace packed_data
  {
    struct meshlet_data
    {
      uint32_t vertex_offset;
      uint32_t triangle_offset;
      uint16_t vertex_count;
      uint16_t triangle_count;
    };
    struct meshlet_culling_data
    {
      glm::vec4 bounding_sphere;
      glm::vec4 cone_apex; // w is unused
      glm::vec4 cone_axis_and_cutoff; // could be packed in cone_apex.w ?
    };
    struct lod_data
    {
      uint32_t meshlet_count;
    };
    struct vertex_data
    {
      glm::uvec4 position_tbn; // xyz: f32 position, w: packed TBN
      static constexpr uint32_t k_data_size = 4;
      glm::u16vec4 data[k_data_size];    // extra data, compressed as f16, packed tightly.
      // For data, the last component of the last entry is reserved for the material data. This means that you can store:
      //  - up to 3 f16 rgba color channels (and one uv channel)
      //  - up to 7 UV channels
    };
    struct mesh_data
    {
      uint32_t color_channel_count; // color channels are RGBA packed as f16, UV are RG packed as unorm16. They both share vertex_data::data.
    };
  }

  struct static_submesh : public resources::rle_data_asset<"submesh", static_submesh>
  {
    // handle versioning:
    static constexpr uint32_t min_supported_version = 0;
    static constexpr uint32_t current_version = 0;
    using version_list = ct::type_list< static_submesh >;


  };

  struct static_mesh_lod : public resources::rle_data_asset<"lod", static_mesh_lod>
  {
    // handle versioning:
    static constexpr uint32_t min_supported_version = 0;
    static constexpr uint32_t current_version = 0;
    using version_list = ct::type_list< static_mesh_lod >;

    // We only handle raw-data in this stage as it's way faster to decode (a single memcopy)
    raw_data vertex_data;
    raw_data vertex_indirection_data;
    raw_data meshlet_index_data;
    raw_data meshlet_data;
    raw_data meshlet_culling_data;
    raw_data lod_data;

    /// \brief Return the total memory size (very close (~32bytes) to uncompressed asset size)
    size_t total_memory_size() const
    {
      return vertex_data.size + vertex_indirection_data.size + meshlet_index_data.size + meshlet_data.size + meshlet_culling_data.size + lod_data.size;
    }
  };

  /// \brief Mesh that has no skeletal component
  /// \note All static meshes are cut into meshlets
  struct static_mesh : public resources::rle_data_asset<"static_mesh", static_mesh>
  {
    // handle versioning:
    static constexpr uint32_t min_supported_version = 0;
    static constexpr uint32_t current_version = 0;
    using version_list = ct::type_list< static_mesh >;

    std::vector<id_t> lods; // LOD resource ID

    glm::vec4 bounding_sphere; // xyz: center, w: radius
  };
}

N_METADATA_STRUCT(neam::hydra::assets::static_mesh)
{
  using member_list = neam::ct::type_list
  <
  >;
};
N_METADATA_STRUCT(neam::hydra::assets::static_mesh_lod)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(vertex_data),
    N_MEMBER_DEF(vertex_indirection_data),
    N_MEMBER_DEF(meshlet_index_data),
    N_MEMBER_DEF(meshlet_data),
    N_MEMBER_DEF(lod_data)
  >;
};
N_METADATA_STRUCT(neam::hydra::assets::static_submesh)
{
  using member_list = neam::ct::type_list
  <
  >;
};

