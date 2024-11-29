//
// created by : Timothée Feuillet
// date: 2024-3-4
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

namespace neam::hydra::shaders
{
  struct blur_descriptor_set : shaders::descriptor_set_struct<blur_descriptor_set>
  {
    shaders::image<readonly> u_input;
    shaders::image<writeonly> u_output;
  };

  struct blur_push_constants : hydra::shaders::block_struct<blur_push_constants>
  {
    static constexpr VkShaderStageFlagBits stage_flags = VK_SHADER_STAGE_COMPUTE_BIT;

    shaders::uvec2 image_size;
    shaders::uint32_t is_horizontal;
    shaders::uint32_t strength;

    static constexpr neam::ct::string glsl_type_name = "blur_push_constants";
  };
}

N_METADATA_STRUCT(neam::hydra::shaders::blur_descriptor_set)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(u_input),
    N_MEMBER_DEF(u_output)
  >;
};
N_METADATA_STRUCT(neam::hydra::shaders::blur_push_constants)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(image_size),
    N_MEMBER_DEF(is_horizontal),
    N_MEMBER_DEF(strength)
  >;
};

