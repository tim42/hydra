//
// created by : Timothée Feuillet
// date: 2024-2-11
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
#include <hydra/renderer/shader_structs.hpp>

namespace neam
{
  struct fs_quad_ubo : hydra::shaders::block_struct<fs_quad_ubo>
  {
    float time;
    hydra::shaders::vec2 screen_resolution;
    uint32_t logo_index;

    static constexpr neam::ct::string glsl_type_name = "fs_quad_ubo";
  };

  struct cube_push_constants : hydra::shaders::block_struct<cube_push_constants>
  {
    hydra::shaders::mat4x4 view_projection;
    hydra::shader_structs::packed_translation_t view_translation;

    static constexpr neam::ct::string glsl_type_name = "cube_push_constant";
  };

  struct cube_buffer_pos : hydra::shaders::block_struct<cube_buffer_pos>
  {
    [[no_unique_address]] hydra::shaders::unbound_array<hydra::shader_structs::packed_transform_t> packed_transforms;

    static constexpr neam::ct::string glsl_type_name = "cube_buffer_pos";
  };

  struct fs_quad_shader_params : hydra::shaders::descriptor_set_struct<fs_quad_shader_params>
  {
    hydra::shaders::combined_image_sampler<> tex_sampler;
    hydra::shaders::ubo<fs_quad_ubo> ubo;
  };
}

N_METADATA_STRUCT(neam::fs_quad_ubo)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(time),
    N_MEMBER_DEF(screen_resolution),
    N_MEMBER_DEF(logo_index)
  >;
};
N_METADATA_STRUCT(neam::cube_push_constants)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(view_projection),
    N_MEMBER_DEF(view_translation)
  >;
};
N_METADATA_STRUCT(neam::cube_buffer_pos)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(packed_transforms)
  >;
};
N_METADATA_STRUCT(neam::fs_quad_shader_params)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(tex_sampler),
    N_MEMBER_DEF(ubo)
  >;
};

