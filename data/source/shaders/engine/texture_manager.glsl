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

#include "hydra_global.glsl"

// FIXME: Cannot set a specific set number...
hydra::descriptor_set(_, neam::hydra::shader_structs::texture_manager_descriptor_set_t);




#define HYDRA_TEXTURE_TYPE_none         0
#define HYDRA_TEXTURE_TYPE_float        1
#define HYDRA_TEXTURE_TYPE_int          2
#define HYDRA_TEXTURE_TYPE_uint         3

#define _HYDRA_TEXTURE_TYPE_BIT_SHIFT   2

#define HYDRA_TEXTURE_KIND_1d           (0 << _HYDRA_TEXTURE_TYPE_BIT_SHIFT)
#define HYDRA_TEXTURE_KIND_1d_array     (1 << _HYDRA_TEXTURE_TYPE_BIT_SHIFT)
#define HYDRA_TEXTURE_KIND_2d           (2 << _HYDRA_TEXTURE_TYPE_BIT_SHIFT)
#define HYDRA_TEXTURE_KIND_2d_array     (3 << _HYDRA_TEXTURE_TYPE_BIT_SHIFT)
#define HYDRA_TEXTURE_KIND_2d_ms        (4 << _HYDRA_TEXTURE_TYPE_BIT_SHIFT)
#define HYDRA_TEXTURE_KIND_2d_ms_array  (5 << _HYDRA_TEXTURE_TYPE_BIT_SHIFT)
#define HYDRA_TEXTURE_KIND_3d           (6 << _HYDRA_TEXTURE_TYPE_BIT_SHIFT)
#define HYDRA_TEXTURE_KIND_cube         (7 << _HYDRA_TEXTURE_TYPE_BIT_SHIFT)
#define HYDRA_TEXTURE_KIND_cube_array   (8 << _HYDRA_TEXTURE_TYPE_BIT_SHIFT)


#define float_1d          (HYDRA_TEXTURE_KIND_1d          | HYDRA_TEXTURE_TYPE_float)
#define float_1d_array    (HYDRA_TEXTURE_KIND_1d_array    | HYDRA_TEXTURE_TYPE_float)
#define float_2d          (HYDRA_TEXTURE_KIND_2d          | HYDRA_TEXTURE_TYPE_float)
#define float_2d_array    (HYDRA_TEXTURE_KIND_2d_array    | HYDRA_TEXTURE_TYPE_float)
#define float_2d_ms       (HYDRA_TEXTURE_KIND_2d_ms       | HYDRA_TEXTURE_TYPE_float)
#define float_2d_ms_array (HYDRA_TEXTURE_KIND_2d_ms_array | HYDRA_TEXTURE_TYPE_float)
#define float_3d          (HYDRA_TEXTURE_KIND_3d          | HYDRA_TEXTURE_TYPE_float)
#define float_cube        (HYDRA_TEXTURE_KIND_cube        | HYDRA_TEXTURE_TYPE_float)
#define float_cube_array  (HYDRA_TEXTURE_KIND_cube_array  | HYDRA_TEXTURE_TYPE_float)

#define int_1d            (HYDRA_TEXTURE_KIND_1d          | HYDRA_TEXTURE_TYPE_int)
#define int_1d_array      (HYDRA_TEXTURE_KIND_1d_array    | HYDRA_TEXTURE_TYPE_int)
#define int_2d            (HYDRA_TEXTURE_KIND_2d          | HYDRA_TEXTURE_TYPE_int)
#define int_2d_array      (HYDRA_TEXTURE_KIND_2d_array    | HYDRA_TEXTURE_TYPE_int)
#define int_2d_ms         (HYDRA_TEXTURE_KIND_2d_ms       | HYDRA_TEXTURE_TYPE_int)
#define int_2d_ms_array   (HYDRA_TEXTURE_KIND_2d_ms_array | HYDRA_TEXTURE_TYPE_int)
#define int_3d            (HYDRA_TEXTURE_KIND_3d          | HYDRA_TEXTURE_TYPE_int)
#define int_cube          (HYDRA_TEXTURE_KIND_cube        | HYDRA_TEXTURE_TYPE_int)
#define int_cube_array    (HYDRA_TEXTURE_KIND_cube_array  | HYDRA_TEXTURE_TYPE_int)

#define uint_1d           (HYDRA_TEXTURE_KIND_1d          | HYDRA_TEXTURE_TYPE_uint)
#define uint_1d_array     (HYDRA_TEXTURE_KIND_1d_array    | HYDRA_TEXTURE_TYPE_uint)
#define uint_2d           (HYDRA_TEXTURE_KIND_2d          | HYDRA_TEXTURE_TYPE_uint)
#define uint_2d_array     (HYDRA_TEXTURE_KIND_2d_array    | HYDRA_TEXTURE_TYPE_uint)
#define uint_2d_ms        (HYDRA_TEXTURE_KIND_2d_ms       | HYDRA_TEXTURE_TYPE_uint)
#define uint_2d_ms_array  (HYDRA_TEXTURE_KIND_2d_ms_array | HYDRA_TEXTURE_TYPE_uint)
#define uint_3d           (HYDRA_TEXTURE_KIND_3d          | HYDRA_TEXTURE_TYPE_uint)
#define uint_cube         (HYDRA_TEXTURE_KIND_cube        | HYDRA_TEXTURE_TYPE_uint)
#define uint_cube_array   (HYDRA_TEXTURE_KIND_cube_array  | HYDRA_TEXTURE_TYPE_uint)


#define HYDRA_TEXTURE_ENUM_TO_INDEX(x)  (((x) - 1) - ((x) >> _HYDRA_TEXTURE_TYPE_BIT_SHIFT))
#define HYDRA_TEXTURE_INDEX_TO_ENUM(x)  ((((x) * 4) / 3) + 1)


/// \brief Convert the return value of texture_manager::request_texture_index to something that can be used by texture_manager_get_texture &co.
uint texture_manager_get_texture_index(uint texture_type, uint texture_id)
{
  if (texture_id == 0 || texture_id >= texture_manager_indirection.texture_count) return 0;
  // FIXME: Check texture type!
  const uint indirection_data = texture_manager_indirection.indirection[texture_id];
  return indirection_data;
}
/// \brief Return whether the return value of texture_manager_get_texture_index is a valid texture index
bool texture_manager_is_texture_index_valid(uint texture_index)
{
  return texture_index != 0;
}

/// \brief Return the gsamplerXX object given a type/kind
/// \warning Does not perform any check
#define texture_manager_get_texture(type, index)  texture_manager_texture_##type[index - 1]
#define texture_manager_get_texture_nonuniform(type, index)  texture_manager_texture_##type[nonuniform(index - 1)]

#define texture_manager_read_texture(type, kind, operation, index, ...)  \
    (texture_manager_is_texture_index_valid(index) ? operation(texture_manager_get_texture(type ## _ ## kind, index) __VA_OPT__(,) __VA_ARGS__) : type##4(0))

#define texture_manager_nonuniform_read_texture(type, kind, operation, index, ...)  \
    (texture_manager_is_texture_index_valid(index) ? operation(texture_manager_get_texture_nonuniform(type ## _ ## kind, index) __VA_OPT__(,) __VA_ARGS__) : type##4(0))

// shorthand for some very comon operations:
// htm: hydra::texture_manager::xyz

float4 htm_texture_flt_2d(uint index, float2 uv)
{
  return texture_manager_read_texture(float, 2d, texture, index, uv);
}
