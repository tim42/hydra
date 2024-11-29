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
  struct texture_indirection_t : hydra::shaders::block_struct<texture_indirection_t>
  {
    shaders::uint32_t texture_count;
    shaders::unbound_array<uint32_t> indirection;

    static constexpr neam::ct::string glsl_type_name = "texture_indirection_t";
  };

  struct texture_manager_descriptor_set_t : hydra::shaders::descriptor_set_struct<texture_manager_descriptor_set_t>
  {
    // remaining entries goes after the vectors
    shaders::buffer<texture_indirection_t, shaders::readonly> texture_manager_indirection;

    // NOTE: must be the last entries
    std::vector<shaders::combined_image_sampler<shaders::float_1d>> texture_manager_texture_float_1d = {};

#define HYDRA_TEXTURE_DS_DECLARE(x) [[no_unique_address]] shaders::noop<std::vector<shaders::combined_image_sampler<shaders::x>>> texture_manager_texture_##x
    // HYDRA_TEXTURE_DS_DECLARE(float_1d); // NOTE: 1d is the "main" one, used for descriptor gen
    HYDRA_TEXTURE_DS_DECLARE(float_1d_array);
    HYDRA_TEXTURE_DS_DECLARE(float_2d);
    HYDRA_TEXTURE_DS_DECLARE(float_2d_array);
    HYDRA_TEXTURE_DS_DECLARE(float_2d_ms);
    HYDRA_TEXTURE_DS_DECLARE(float_2d_ms_array);
    HYDRA_TEXTURE_DS_DECLARE(float_3d);
    HYDRA_TEXTURE_DS_DECLARE(float_cube);
    HYDRA_TEXTURE_DS_DECLARE(float_cube_array);
    HYDRA_TEXTURE_DS_DECLARE(int_1d);
    HYDRA_TEXTURE_DS_DECLARE(int_1d_array);
    HYDRA_TEXTURE_DS_DECLARE(int_2d);
    HYDRA_TEXTURE_DS_DECLARE(int_2d_array);
    HYDRA_TEXTURE_DS_DECLARE(int_2d_ms);
    HYDRA_TEXTURE_DS_DECLARE(int_2d_ms_array);
    HYDRA_TEXTURE_DS_DECLARE(int_3d);
    HYDRA_TEXTURE_DS_DECLARE(int_cube);
    HYDRA_TEXTURE_DS_DECLARE(int_cube_array);
    HYDRA_TEXTURE_DS_DECLARE(uint_1d);
    HYDRA_TEXTURE_DS_DECLARE(uint_1d_array);
    HYDRA_TEXTURE_DS_DECLARE(uint_2d);
    HYDRA_TEXTURE_DS_DECLARE(uint_2d_array);
    HYDRA_TEXTURE_DS_DECLARE(uint_2d_ms);
    HYDRA_TEXTURE_DS_DECLARE(uint_2d_ms_array);
    HYDRA_TEXTURE_DS_DECLARE(uint_3d);
    HYDRA_TEXTURE_DS_DECLARE(uint_cube);
    HYDRA_TEXTURE_DS_DECLARE(uint_cube_array);
#undef HYDRA_TEXTURE_DS_DECLARE
  };
}

N_METADATA_STRUCT(neam::hydra::shader_structs::texture_indirection_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(texture_count),
    N_MEMBER_DEF(indirection)
  >;
};
N_METADATA_STRUCT(neam::hydra::shader_structs::texture_manager_descriptor_set_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(texture_manager_indirection),

    N_MEMBER_DEF(texture_manager_texture_float_1d),
#define HYDRA_TEXTURE_DS_DECLARE(x)   N_MEMBER_DEF(texture_manager_texture_##x, neam::hydra::shaders::alias_of_previous_entry{})
    // HYDRA_TEXTURE_DS_DECLARE(float_1d), // NOTE: 1d is the "main" one, used for descriptor gen
    HYDRA_TEXTURE_DS_DECLARE(float_1d_array),
    HYDRA_TEXTURE_DS_DECLARE(float_2d),
    HYDRA_TEXTURE_DS_DECLARE(float_2d_array),
    HYDRA_TEXTURE_DS_DECLARE(float_2d_ms),
    HYDRA_TEXTURE_DS_DECLARE(float_2d_ms_array),
    HYDRA_TEXTURE_DS_DECLARE(float_3d),
    HYDRA_TEXTURE_DS_DECLARE(float_cube),
    HYDRA_TEXTURE_DS_DECLARE(float_cube_array),
    HYDRA_TEXTURE_DS_DECLARE(int_1d),
    HYDRA_TEXTURE_DS_DECLARE(int_1d_array),
    HYDRA_TEXTURE_DS_DECLARE(int_2d),
    HYDRA_TEXTURE_DS_DECLARE(int_2d_array),
    HYDRA_TEXTURE_DS_DECLARE(int_2d_ms),
    HYDRA_TEXTURE_DS_DECLARE(int_2d_ms_array),
    HYDRA_TEXTURE_DS_DECLARE(int_3d),
    HYDRA_TEXTURE_DS_DECLARE(int_cube),
    HYDRA_TEXTURE_DS_DECLARE(int_cube_array),
    HYDRA_TEXTURE_DS_DECLARE(uint_1d),
    HYDRA_TEXTURE_DS_DECLARE(uint_1d_array),
    HYDRA_TEXTURE_DS_DECLARE(uint_2d),
    HYDRA_TEXTURE_DS_DECLARE(uint_2d_array),
    HYDRA_TEXTURE_DS_DECLARE(uint_2d_ms),
    HYDRA_TEXTURE_DS_DECLARE(uint_2d_ms_array),
    HYDRA_TEXTURE_DS_DECLARE(uint_3d),
    HYDRA_TEXTURE_DS_DECLARE(uint_cube),
    HYDRA_TEXTURE_DS_DECLARE(uint_cube_array)
#undef HYDRA_TEXTURE_DS_DECLARE
  >;
};
