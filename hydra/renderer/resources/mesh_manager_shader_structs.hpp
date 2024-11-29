//
// created by : Timothée Feuillet
// date: 2024-3-8
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

#include <hydra/utilities/shader_gen/block.hpp>
#include <hydra/utilities/shader_gen/descriptor_sets.hpp>

namespace neam::hydra::shader_structs
{
  struct mesh_indirection_t : hydra::shaders::block_struct<mesh_indirection_t>
  {
    shaders::uint32_t mesh_count;
    shaders::unbound_array<uint32_t> indirection;

    static constexpr neam::ct::string glsl_type_name = "mesh_indirection_t";
  };

  struct mesh_manager_descriptor_set_t : hydra::shaders::descriptor_set_struct<mesh_manager_descriptor_set_t>
  {
    // remaining entries goes after the vectors
    shaders::buffer<mesh_indirection_t, shaders::readonly> mesh_manager_indirection;
  };
}

N_METADATA_STRUCT(neam::hydra::shader_structs::mesh_indirection_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(mesh_count),
    N_MEMBER_DEF(indirection)
  >;
};
N_METADATA_STRUCT(neam::hydra::shader_structs::mesh_manager_descriptor_set_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(mesh_manager_indirection)
  >;
};
