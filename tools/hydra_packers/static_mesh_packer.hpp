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

#include <ntools/raw_data.hpp>
#include <ntools/struct_metadata/struct_metadata.hpp>
#include <hydra/assets/static_mesh.hpp>

#include <hydra_glm.hpp>

namespace neam::hydra::packer
{
  struct vertex_data
  {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    // material index, per vertex (tho it should be indentical per face, there's no support for material blending)
    uint32_t material_index;
  };

  struct vertex_data_stream
  {
    std::vector<glm::vec4> data;

    bool is_vec2;
    bool must_be_strictly_different; // FIXME: don't ignore anymore
  };

  struct static_mesh_packer_input
  {
    std::vector<vertex_data> vertices;
    std::vector<vertex_data_stream> data;
    std::vector<uint32_t> indices;

    uint32_t material_count;

    std::map<id_t, uint32_t> material_indices;
    std::map<id_t, std::string> material_names;

    glm::vec4 bounding_sphere;
  };
}
N_METADATA_STRUCT(neam::hydra::packer::vertex_data)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(position),
    N_MEMBER_DEF(normal),
    N_MEMBER_DEF(tangent),
    N_MEMBER_DEF(bitangent),

    N_MEMBER_DEF(material_index)
  >;
};
N_METADATA_STRUCT(neam::hydra::packer::vertex_data_stream)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(data),
    N_MEMBER_DEF(is_vec2),
    N_MEMBER_DEF(must_be_strictly_different)
  >;
};
N_METADATA_STRUCT(neam::hydra::packer::static_mesh_packer_input)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(vertices),
    N_MEMBER_DEF(data),
    N_MEMBER_DEF(indices),
    N_MEMBER_DEF(material_count),
    N_MEMBER_DEF(material_indices),
    N_MEMBER_DEF(material_names)
  >;
};

