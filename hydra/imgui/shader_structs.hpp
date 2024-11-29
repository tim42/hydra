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

namespace neam::hydra::imgui
{
  struct imgui_push_constants : hydra::shaders::block_struct<imgui_push_constants>
  {
    shaders::vec2 scale;
    shaders::vec2 translate;

    shaders::uint32_t font_texture_index;
    shaders::uint32_t backbuffer_texture_index;

    static constexpr neam::ct::string glsl_type_name = "imgui_push_constants";
  };

  struct imgui_shader_params : hydra::shaders::descriptor_set_struct<imgui_shader_params>
  {
    shaders::sampler s_sampler;
    std::vector<shaders::image<shaders::readonly>> s_textures;
  };
}

N_METADATA_STRUCT(neam::hydra::imgui::imgui_push_constants)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(scale),
    N_MEMBER_DEF(translate),
    N_MEMBER_DEF(font_texture_index),
    N_MEMBER_DEF(backbuffer_texture_index)
  >;
};

N_METADATA_STRUCT(neam::hydra::imgui::imgui_shader_params)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(s_sampler),
    N_MEMBER_DEF(s_textures)
  >;
};

