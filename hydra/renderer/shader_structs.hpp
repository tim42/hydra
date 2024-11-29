//
// created by : Timothée Feuillet
// date: 2024-2-19
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
#include <hydra/utilities/shader_gen/descriptor_sets_types.hpp>

namespace neam::hydra::shader_structs
{
  struct quaternion : hydra::shaders::block_struct<quaternion>
  {
    shaders::vec4 data;

    static constexpr neam::ct::string glsl_type_name = "quaternion";
  };

  struct packed_translation_t : hydra::shaders::block_struct<packed_translation_t>
  {
    shaders::ivec3 grid;
    shaders::u16vec3 fine;

    static constexpr neam::ct::string glsl_type_name = "packed_translation_t";
  };

  struct packed_transform_t : hydra::shaders::block_struct<packed_transform_t>
  {
    packed_translation_t translation;
    float scale;
    shaders::i8vec4 packed_quaternion;

    static constexpr neam::ct::string glsl_type_name = "packed_transform_t";
  };
}

N_METADATA_STRUCT(neam::hydra::shader_structs::quaternion)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(data)
  >;
};
N_METADATA_STRUCT(neam::hydra::shader_structs::packed_translation_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(grid),
    N_MEMBER_DEF(fine)
  >;
};
N_METADATA_STRUCT(neam::hydra::shader_structs::packed_transform_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(translation),
    N_MEMBER_DEF(scale),
    N_MEMBER_DEF(packed_quaternion)
  >;
};


