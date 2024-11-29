//
// file : ct_point_udl.hpp
// in : file:///home/tim/projects/yaggler/yaggler/ct_point_udl.hpp
//
// created by : Timothée Feuillet on linux-coincoin.tim
// date: 27/01/2014 11:49:13
//
//
// Copyright (c) 2016 Timothée Feuillet
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
# define __N_1814989461818831215_234927224__GLM_UDL_HPP__

#include <hydra_glm.hpp>


// this is SO MUCH EASIER than hitting your keyboard for the same things, over and over... (poor keyboard :/ )
// 2D glm::vec
static inline glm::vec2 operator "" _vec2_x(long double val)
{
  return glm::vec2((val), (0));
}
static inline glm::vec2 operator "" _vec2_x(unsigned long long val)
{
  return glm::vec2((val), (0));
}

static inline glm::vec2 operator "" _vec2_y(long double val)
{
  return glm::vec2((0), (val));
}
static inline glm::vec2 operator "" _vec2_y(unsigned long long val)
{
  return glm::vec2((0), (val));
}

static inline glm::vec2 operator "" _vec2_xy(long double val)
{
  return glm::vec2((val), (val));
}
static inline glm::vec2 operator "" _vec2_xy(unsigned long long val)
{
  return glm::vec2((val), (val));
}

// 3D glm::vec
static inline glm::vec3 operator "" _vec3_x(long double val)
{
  return glm::vec3((val), (0), (0));
}
static inline glm::vec3 operator "" _vec3_x(unsigned long long val)
{
  return glm::vec3((val), (0), (0));
}

static inline glm::vec3 operator "" _vec3_y(long double val)
{
  return glm::vec3((0), (val), (0));
}
static inline glm::vec3 operator "" _vec3_y(unsigned long long val)
{
  return glm::vec3((0), (val), (0));
}

static inline glm::vec3 operator "" _vec3_z(long double val)
{
  return glm::vec3((0), (0), (val));
}
static inline glm::vec3 operator "" _vec3_z(unsigned long long val)
{
  return glm::vec3((0), (0), (val));
}

static inline glm::vec3 operator "" _vec3_xy(long double val)
{
  return glm::vec3((val), (val), (0));
}
static inline glm::vec3 operator "" _vec3_xy(unsigned long long val)
{
  return glm::vec3((val), (val), (0));
}

static inline glm::vec3 operator "" _vec3_xz(long double val)
{
  return glm::vec3((val), (0), (val));
}
static inline glm::vec3 operator "" _vec3_xz(unsigned long long val)
{
  return glm::vec3((val), (0), (val));
}

static inline glm::vec3 operator "" _vec3_yz(long double val)
{
  return glm::vec3((0), (val), (val));
}
static inline glm::vec3 operator "" _vec3_yz(unsigned long long val)
{
  return glm::vec3((0), (val), (val));
}

static inline glm::vec3 operator "" _vec3_xyz(long double val)
{
  return glm::vec3((val), (val), (val));
}
static inline glm::vec3 operator "" _vec3_xyz(unsigned long long val)
{
  return glm::vec3((val), (val), (val));
}

// 4D glm::vec
static inline glm::vec4 operator "" _vec4_x(long double val)
{
  return glm::vec4((val), (0), (0), 0);
}
static inline glm::vec4 operator "" _vec4_x(unsigned long long val)
{
  return glm::vec4((val), (0), (0), 0);
}

static inline glm::vec4 operator "" _vec4_y(long double val)
{
  return glm::vec4((0), (val), (0), 0);
}
static inline glm::vec4 operator "" _vec4_y(unsigned long long val)
{
  return glm::vec4((0), (val), (0), 0);
}

static inline glm::vec4 operator "" _vec4_z(long double val)
{
  return glm::vec4((0), (0), (val), 0);
}
static inline glm::vec4 operator "" _vec4_z(unsigned long long val)
{
  return glm::vec4((0), (0), (val), 0);
}

static inline glm::vec4 operator "" _vec4_w(long double val)
{
  return glm::vec4((0), (0), (0), (val));
}
static inline glm::vec4 operator "" _vec4_w(unsigned long long val)
{
  return glm::vec4((0), (0), (0), (val));
}

///

static inline glm::vec4 operator "" _vec4_xy(long double val)
{
  return glm::vec4((val), (val), (0), 0);
}
static inline glm::vec4 operator "" _vec4_xy(unsigned long long val)
{
  return glm::vec4((val), (val), (0), 0);
}

static inline glm::vec4 operator "" _vec4_xz(long double val)
{
  return glm::vec4((val), (0), (val), 0);
}
static inline glm::vec4 operator "" _vec4_xz(unsigned long long val)
{
  return glm::vec4((val), (0), (val), 0);
}

static inline glm::vec4 operator "" _vec4_yz(long double val)
{
  return glm::vec4((0), (val), (val), 0);
}
static inline glm::vec4 operator "" _vec4_yz(unsigned long long val)
{
  return glm::vec4((0), (val), (val), 0);
}

static inline glm::vec4 operator "" _vec4_xw(long double val)
{
  return glm::vec4((val), (0), (0), (val));
}
static inline glm::vec4 operator "" _vec4_xw(unsigned long long val)
{
  return glm::vec4((val), (0), (0), (val));
}

static inline glm::vec4 operator "" _vec4_yw(long double val)
{
  return glm::vec4((0), (val), (0), (val));
}
static inline glm::vec4 operator "" _vec4_yw(unsigned long long val)
{
  return glm::vec4((0), (val), (0), (val));
}

static inline glm::vec4 operator "" _vec4_zw(long double val)
{
  return glm::vec4((0), (0), (val), (val));
}
static inline glm::vec4 operator "" _vec4_zw(unsigned long long val)
{
  return glm::vec4((0), (0), (val), (val));
}

///
static inline glm::vec4 operator "" _vec4_xyz(long double val)
{
  return glm::vec4((val), (val), (val), 0);
}
static inline glm::vec4 operator "" _vec4_xyz(unsigned long long val)
{
  return glm::vec4((val), (val), (val), 0);
}

static inline glm::vec4 operator "" _vec4_yzw(long double val)
{
  return glm::vec4((0), (val), (val), (val));
}
static inline glm::vec4 operator "" _vec4_yzw(unsigned long long val)
{
  return glm::vec4((0), (val), (val), (val));
}

static inline glm::vec4 operator "" _vec4_xzw(long double val)
{
  return glm::vec4((val), (0), (val), (val));
}
static inline glm::vec4 operator "" _vec4_xzw(unsigned long long val)
{
  return glm::vec4((val), (0), (val), (val));
}

static inline glm::vec4 operator "" _vec4_xyw(long double val)
{
  return glm::vec4((val), (val), (0), (val));
}
static inline glm::vec4 operator "" _vec4_xyw(unsigned long long val)
{
  return glm::vec4((val), (val), (0), (val));
}

///

static inline glm::vec4 operator "" _vec4_xyzw(long double val)
{
  return glm::vec4((val), (val), (val), (val));
}
static inline glm::vec4 operator "" _vec4_xyzw(unsigned long long val)
{
  return glm::vec4((val), (val), (val), (val));
}

///

static inline glm::ivec2 operator "" _ivec2_x(long double val)
{
  return glm::ivec2((val), (0));
}
static inline glm::ivec2 operator "" _ivec2_x(unsigned long long val)
{
  return glm::ivec2((val), (0));
}

static inline glm::ivec2 operator "" _ivec2_y(long double val)
{
  return glm::ivec2((0), (val));
}
static inline glm::ivec2 operator "" _ivec2_y(unsigned long long val)
{
  return glm::ivec2((0), (val));
}

static inline glm::ivec2 operator "" _ivec2_xy(long double val)
{
  return glm::ivec2((val), (val));
}
static inline glm::ivec2 operator "" _ivec2_xy(unsigned long long val)
{
  return glm::ivec2((val), (val));
}

// 3D glm::ivec
static inline glm::ivec3 operator "" _ivec3_x(long double val)
{
  return glm::ivec3((val), (0), (0));
}
static inline glm::ivec3 operator "" _ivec3_x(unsigned long long val)
{
  return glm::ivec3((val), (0), (0));
}

static inline glm::ivec3 operator "" _ivec3_y(long double val)
{
  return glm::ivec3((0), (val), (0));
}
static inline glm::ivec3 operator "" _ivec3_y(unsigned long long val)
{
  return glm::ivec3((0), (val), (0));
}

static inline glm::ivec3 operator "" _ivec3_z(long double val)
{
  return glm::ivec3((0), (0), (val));
}
static inline glm::ivec3 operator "" _ivec3_z(unsigned long long val)
{
  return glm::ivec3((0), (0), (val));
}

static inline glm::ivec3 operator "" _ivec3_xy(long double val)
{
  return glm::ivec3((val), (val), (0));
}
static inline glm::ivec3 operator "" _ivec3_xy(unsigned long long val)
{
  return glm::ivec3((val), (val), (0));
}

static inline glm::ivec3 operator "" _ivec3_xz(long double val)
{
  return glm::ivec3((val), (0), (val));
}
static inline glm::ivec3 operator "" _ivec3_xz(unsigned long long val)
{
  return glm::ivec3((val), (0), (val));
}

static inline glm::ivec3 operator "" _ivec3_yz(long double val)
{
  return glm::ivec3((0), (val), (val));
}
static inline glm::ivec3 operator "" _ivec3_yz(unsigned long long val)
{
  return glm::ivec3((0), (val), (val));
}

static inline glm::ivec3 operator "" _ivec3_xyz(long double val)
{
  return glm::ivec3((val), (val), (val));
}
static inline glm::ivec3 operator "" _ivec3_xyz(unsigned long long val)
{
  return glm::ivec3((val), (val), (val));
}

// 4D glm::ivec
static inline glm::ivec4 operator "" _ivec4_x(long double val)
{
  return glm::ivec4((val), (0), (0), 0);
}
static inline glm::ivec4 operator "" _ivec4_x(unsigned long long val)
{
  return glm::ivec4((val), (0), (0), 0);
}

static inline glm::ivec4 operator "" _ivec4_y(long double val)
{
  return glm::ivec4((0), (val), (0), 0);
}
static inline glm::ivec4 operator "" _ivec4_y(unsigned long long val)
{
  return glm::ivec4((0), (val), (0), 0);
}

static inline glm::ivec4 operator "" _ivec4_z(long double val)
{
  return glm::ivec4((0), (0), (val), 0);
}
static inline glm::ivec4 operator "" _ivec4_z(unsigned long long val)
{
  return glm::ivec4((0), (0), (val), 0);
}

static inline glm::ivec4 operator "" _ivec4_w(long double val)
{
  return glm::ivec4((0), (0), (0), (val));
}
static inline glm::ivec4 operator "" _ivec4_w(unsigned long long val)
{
  return glm::ivec4((0), (0), (0), (val));
}

///

static inline glm::ivec4 operator "" _ivec4_xy(long double val)
{
  return glm::ivec4((val), (val), (0), 0);
}
static inline glm::ivec4 operator "" _ivec4_xy(unsigned long long val)
{
  return glm::ivec4((val), (val), (0), 0);
}

static inline glm::ivec4 operator "" _ivec4_xz(long double val)
{
  return glm::ivec4((val), (0), (val), 0);
}
static inline glm::ivec4 operator "" _ivec4_xz(unsigned long long val)
{
  return glm::ivec4((val), (0), (val), 0);
}

static inline glm::ivec4 operator "" _ivec4_yz(long double val)
{
  return glm::ivec4((0), (val), (val), 0);
}
static inline glm::ivec4 operator "" _ivec4_yz(unsigned long long val)
{
  return glm::ivec4((0), (val), (val), 0);
}

static inline glm::ivec4 operator "" _ivec4_xw(long double val)
{
  return glm::ivec4((val), (0), (0), (val));
}
static inline glm::ivec4 operator "" _ivec4_xw(unsigned long long val)
{
  return glm::ivec4((val), (0), (0), (val));
}

static inline glm::ivec4 operator "" _ivec4_yw(long double val)
{
  return glm::ivec4((0), (val), (0), (val));
}
static inline glm::ivec4 operator "" _ivec4_yw(unsigned long long val)
{
  return glm::ivec4((0), (val), (0), (val));
}

static inline glm::ivec4 operator "" _ivec4_zw(long double val)
{
  return glm::ivec4((0), (0), (val), (val));
}
static inline glm::ivec4 operator "" _ivec4_zw(unsigned long long val)
{
  return glm::ivec4((0), (0), (val), (val));
}

///
static inline glm::ivec4 operator "" _ivec4_xyz(long double val)
{
  return glm::ivec4((val), (val), (val), 0);
}
static inline glm::ivec4 operator "" _ivec4_xyz(unsigned long long val)
{
  return glm::ivec4((val), (val), (val), 0);
}

static inline glm::ivec4 operator "" _ivec4_yzw(long double val)
{
  return glm::ivec4((0), (val), (val), (val));
}
static inline glm::ivec4 operator "" _ivec4_yzw(unsigned long long val)
{
  return glm::ivec4((0), (val), (val), (val));
}

static inline glm::ivec4 operator "" _ivec4_xzw(long double val)
{
  return glm::ivec4((val), (0), (val), (val));
}
static inline glm::ivec4 operator "" _ivec4_xzw(unsigned long long val)
{
  return glm::ivec4((val), (0), (val), (val));
}

static inline glm::ivec4 operator "" _ivec4_xyw(long double val)
{
  return glm::ivec4((val), (val), (0), (val));
}
static inline glm::ivec4 operator "" _ivec4_xyw(unsigned long long val)
{
  return glm::ivec4((val), (val), (0), (val));
}

///

static inline glm::ivec4 operator "" _ivec4_xyzw(long double val)
{
  return glm::ivec4((val), (val), (val), (val));
}
static inline glm::ivec4 operator "" _ivec4_xyzw(unsigned long long val)
{
  return glm::ivec4((val), (val), (val), (val));
}

///

static inline glm::uvec2 operator "" _uvec2_x(long double val)
{
  return glm::uvec2((val), (0));
}
static inline glm::uvec2 operator "" _uvec2_x(unsigned long long val)
{
  return glm::uvec2((val), (0));
}

static inline glm::uvec2 operator "" _uvec2_y(long double val)
{
  return glm::uvec2((0), (val));
}
static inline glm::uvec2 operator "" _uvec2_y(unsigned long long val)
{
  return glm::uvec2((0), (val));
}

static inline glm::uvec2 operator "" _uvec2_xy(long double val)
{
  return glm::uvec2((val), (val));
}
static inline glm::uvec2 operator "" _uvec2_xy(unsigned long long val)
{
  return glm::uvec2((val), (val));
}

// 3D glm::uvec
static inline glm::uvec3 operator "" _uvec3_x(long double val)
{
  return glm::uvec3((val), (0), (0));
}
static inline glm::uvec3 operator "" _uvec3_x(unsigned long long val)
{
  return glm::uvec3((val), (0), (0));
}

static inline glm::uvec3 operator "" _uvec3_y(long double val)
{
  return glm::uvec3((0), (val), (0));
}
static inline glm::uvec3 operator "" _uvec3_y(unsigned long long val)
{
  return glm::uvec3((0), (val), (0));
}

static inline glm::uvec3 operator "" _uvec3_z(long double val)
{
  return glm::uvec3((0), (0), (val));
}
static inline glm::uvec3 operator "" _uvec3_z(unsigned long long val)
{
  return glm::uvec3((0), (0), (val));
}

static inline glm::uvec3 operator "" _uvec3_xy(long double val)
{
  return glm::uvec3((val), (val), (0));
}
static inline glm::uvec3 operator "" _uvec3_xy(unsigned long long val)
{
  return glm::uvec3((val), (val), (0));
}

static inline glm::uvec3 operator "" _uvec3_xz(long double val)
{
  return glm::uvec3((val), (0), (val));
}
static inline glm::uvec3 operator "" _uvec3_xz(unsigned long long val)
{
  return glm::uvec3((val), (0), (val));
}

static inline glm::uvec3 operator "" _uvec3_yz(long double val)
{
  return glm::uvec3((0), (val), (val));
}
static inline glm::uvec3 operator "" _uvec3_yz(unsigned long long val)
{
  return glm::uvec3((0), (val), (val));
}

static inline glm::uvec3 operator "" _uvec3_xyz(long double val)
{
  return glm::uvec3((val), (val), (val));
}
static inline glm::uvec3 operator "" _uvec3_xyz(unsigned long long val)
{
  return glm::uvec3((val), (val), (val));
}

// 4D glm::uvec
static inline glm::uvec4 operator "" _uvec4_x(long double val)
{
  return glm::uvec4((val), (0), (0), 0);
}
static inline glm::uvec4 operator "" _uvec4_x(unsigned long long val)
{
  return glm::uvec4((val), (0), (0), 0);
}

static inline glm::uvec4 operator "" _uvec4_y(long double val)
{
  return glm::uvec4((0), (val), (0), 0);
}
static inline glm::uvec4 operator "" _uvec4_y(unsigned long long val)
{
  return glm::uvec4((0), (val), (0), 0);
}

static inline glm::uvec4 operator "" _uvec4_z(long double val)
{
  return glm::uvec4((0), (0), (val), 0);
}
static inline glm::uvec4 operator "" _uvec4_z(unsigned long long val)
{
  return glm::uvec4((0), (0), (val), 0);
}

static inline glm::uvec4 operator "" _uvec4_w(long double val)
{
  return glm::uvec4((0), (0), (0), (val));
}
static inline glm::uvec4 operator "" _uvec4_w(unsigned long long val)
{
  return glm::uvec4((0), (0), (0), (val));
}

///

static inline glm::uvec4 operator "" _uvec4_xy(long double val)
{
  return glm::uvec4((val), (val), (0), 0);
}
static inline glm::uvec4 operator "" _uvec4_xy(unsigned long long val)
{
  return glm::uvec4((val), (val), (0), 0);
}

static inline glm::uvec4 operator "" _uvec4_xz(long double val)
{
  return glm::uvec4((val), (0), (val), 0);
}
static inline glm::uvec4 operator "" _uvec4_xz(unsigned long long val)
{
  return glm::uvec4((val), (0), (val), 0);
}

static inline glm::uvec4 operator "" _uvec4_yz(long double val)
{
  return glm::uvec4((0), (val), (val), 0);
}
static inline glm::uvec4 operator "" _uvec4_yz(unsigned long long val)
{
  return glm::uvec4((0), (val), (val), 0);
}

static inline glm::uvec4 operator "" _uvec4_xw(long double val)
{
  return glm::uvec4((val), (0), (0), (val));
}
static inline glm::uvec4 operator "" _uvec4_xw(unsigned long long val)
{
  return glm::uvec4((val), (0), (0), (val));
}

static inline glm::uvec4 operator "" _uvec4_yw(long double val)
{
  return glm::uvec4((0), (val), (0), (val));
}
static inline glm::uvec4 operator "" _uvec4_yw(unsigned long long val)
{
  return glm::uvec4((0), (val), (0), (val));
}

static inline glm::uvec4 operator "" _uvec4_zw(long double val)
{
  return glm::uvec4((0), (0), (val), (val));
}
static inline glm::uvec4 operator "" _uvec4_zw(unsigned long long val)
{
  return glm::uvec4((0), (0), (val), (val));
}

///
static inline glm::uvec4 operator "" _uvec4_xyz(long double val)
{
  return glm::uvec4((val), (val), (val), 0);
}
static inline glm::uvec4 operator "" _uvec4_xyz(unsigned long long val)
{
  return glm::uvec4((val), (val), (val), 0);
}

static inline glm::uvec4 operator "" _uvec4_yzw(long double val)
{
  return glm::uvec4((0), (val), (val), (val));
}
static inline glm::uvec4 operator "" _uvec4_yzw(unsigned long long val)
{
  return glm::uvec4((0), (val), (val), (val));
}

static inline glm::uvec4 operator "" _uvec4_xzw(long double val)
{
  return glm::uvec4((val), (0), (val), (val));
}
static inline glm::uvec4 operator "" _uvec4_xzw(unsigned long long val)
{
  return glm::uvec4((val), (0), (val), (val));
}

static inline glm::uvec4 operator "" _uvec4_xyw(long double val)
{
  return glm::uvec4((val), (val), (0), (val));
}
static inline glm::uvec4 operator "" _uvec4_xyw(unsigned long long val)
{
  return glm::uvec4((val), (val), (0), (val));
}

///

static inline glm::uvec4 operator "" _uvec4_xyzw(long double val)
{
  return glm::uvec4((val), (val), (val), (val));
}
static inline glm::uvec4 operator "" _uvec4_xyzw(unsigned long long val)
{
  return glm::uvec4((val), (val), (val), (val));
}



// kate: indent-mode cstyle; indent-width 2; replace-tabs on; 

