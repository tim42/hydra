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

#include <hydra_glm.hpp>
#include <glm/gtc/packing.hpp>
#include <ntools/ct_string.hpp>
#include <ntools/id/id.hpp>
#include <ntools/struct_metadata/struct_metadata.hpp>

#include <hydra/vulkan/buffer.hpp>
#include <hydra/vulkan/image_view.hpp>
#include <hydra/vulkan/sampler.hpp>

#ifndef N_HYDRA_SHADERS_ALLOW_GENERATION
#define N_HYDRA_SHADERS_ALLOW_GENERATION false
#endif
#if N_STRIP_DEBUG
#undef N_HYDRA_SHADERS_ALLOW_GENERATION
#define N_HYDRA_SHADERS_ALLOW_GENERATION false
#endif

/// \brief Implem of shader types that are guaranteed to work as-is between glsl and C++ and are supported for generating structures, blocks and descriptors
namespace neam::hydra::shaders
{
  template<typename T> inline constexpr auto glsl_type_name = nullptr;
  template<metadata::concepts::StructWithMetadata T> inline constexpr ct::string glsl_type_name<T> = T::glsl_type_name;

  using float32_t = float;
  using int8_t = ::int8_t;
  using int16_t = ::int16_t;
  using int32_t = ::int32_t;
  using uint8_t = ::uint8_t;
  using uint16_t = ::uint16_t;
  using uint32_t = ::uint32_t;
  template<> inline constexpr ct::string glsl_type_name<float> = c_string_t<"float">;
  template<> inline constexpr ct::string glsl_type_name<int8_t> = c_string_t<"int8_t">;
  template<> inline constexpr ct::string glsl_type_name<int16_t> = c_string_t<"int16_t">;
  template<> inline constexpr ct::string glsl_type_name<int32_t> = c_string_t<"int">;
  template<> inline constexpr ct::string glsl_type_name<uint8_t> = c_string_t<"uint8_t">;
  template<> inline constexpr ct::string glsl_type_name<uint16_t> = c_string_t<"uint16_t">;
  template<> inline constexpr ct::string glsl_type_name<uint32_t> = c_string_t<"uint">;


  using vec4 = glm::tvec4<float, glm::packed_highp>;
  using vec3 = glm::tvec3<float, glm::packed_highp>;
  using vec2 = glm::tvec2<float, glm::packed_highp>;

  template<> inline constexpr ct::string glsl_type_name<vec4> = c_string_t<"vec4">;
  template<> inline constexpr ct::string glsl_type_name<vec3> = c_string_t<"vec3">;
  template<> inline constexpr ct::string glsl_type_name<vec2> = c_string_t<"vec2">;

  using ivec4 = glm::tvec4<int32_t, glm::packed_highp>;
  using ivec3 = glm::tvec3<int32_t, glm::packed_highp>;
  using ivec2 = glm::tvec2<int32_t, glm::packed_highp>;

  template<> inline constexpr ct::string glsl_type_name<ivec4> = c_string_t<"ivec4">;
  template<> inline constexpr ct::string glsl_type_name<ivec3> = c_string_t<"ivec3">;
  template<> inline constexpr ct::string glsl_type_name<ivec2> = c_string_t<"ivec2">;

  using uvec4 = glm::tvec4<uint32_t, glm::packed_highp>;
  using uvec3 = glm::tvec3<uint32_t, glm::packed_highp>;
  using uvec2 = glm::tvec2<uint32_t, glm::packed_highp>;

  template<> inline constexpr ct::string glsl_type_name<uvec4> = c_string_t<"uvec4">;
  template<> inline constexpr ct::string glsl_type_name<uvec3> = c_string_t<"uvec3">;
  template<> inline constexpr ct::string glsl_type_name<uvec2> = c_string_t<"uvec2">;


  struct alignas(2) float16_t
  {
    float16_t() = default;
    float16_t(const float16_t&) = default;
    float16_t& operator = (const float16_t&) = default;

    // implicit
    float16_t(float in) : data(glm::packHalf1x16((in))) {}
    float16_t& operator = (float in) { data = glm::packHalf1x16((in)); return *this; }

    uint16_t data;
  };
  template<> inline constexpr ct::string glsl_type_name<float16_t> = c_string_t<"float16_t">;

  using f16vec4 = glm::tvec4<float16_t, glm::packed_highp>;
  using f16vec3 = glm::tvec3<float16_t, glm::packed_highp>;
  using f16vec2 = glm::tvec2<float16_t, glm::packed_highp>;
  template<> inline constexpr ct::string glsl_type_name<f16vec4> = c_string_t<"f16vec4">;
  template<> inline constexpr ct::string glsl_type_name<f16vec3> = c_string_t<"f16vec3">;
  template<> inline constexpr ct::string glsl_type_name<f16vec2> = c_string_t<"f16vec2">;

  using i16vec4 = glm::tvec4<int16_t, glm::packed_highp>;
  using i16vec3 = glm::tvec3<int16_t, glm::packed_highp>;
  using i16vec2 = glm::tvec2<int16_t, glm::packed_highp>;
  template<> inline constexpr ct::string glsl_type_name<i16vec4> = c_string_t<"i16vec4">;
  template<> inline constexpr ct::string glsl_type_name<i16vec3> = c_string_t<"i16vec3">;
  template<> inline constexpr ct::string glsl_type_name<i16vec2> = c_string_t<"i16vec2">;

  using u16vec4 = glm::tvec4<uint16_t, glm::packed_highp>;
  using u16vec3 = glm::tvec3<uint16_t, glm::packed_highp>;
  using u16vec2 = glm::tvec2<uint16_t, glm::packed_highp>;
  template<> inline constexpr ct::string glsl_type_name<u16vec4> = c_string_t<"u16vec4">;
  template<> inline constexpr ct::string glsl_type_name<u16vec3> = c_string_t<"u16vec3">;
  template<> inline constexpr ct::string glsl_type_name<u16vec2> = c_string_t<"u16vec2">;

  using i8vec4 = glm::tvec4<int8_t, glm::packed_highp>;
  using i8vec3 = glm::tvec3<int8_t, glm::packed_highp>;
  using i8vec2 = glm::tvec2<int8_t, glm::packed_highp>;
  template<> inline constexpr ct::string glsl_type_name<i8vec4> = c_string_t<"i8vec4">;
  template<> inline constexpr ct::string glsl_type_name<i8vec3> = c_string_t<"i8vec3">;
  template<> inline constexpr ct::string glsl_type_name<i8vec2> = c_string_t<"i8vec2">;

  using u8vec4 = glm::tvec4<uint8_t, glm::packed_highp>;
  using u8vec3 = glm::tvec3<uint8_t, glm::packed_highp>;
  using u8vec2 = glm::tvec2<uint8_t, glm::packed_highp>;
  template<> inline constexpr ct::string glsl_type_name<u8vec4> = c_string_t<"u8vec4">;
  template<> inline constexpr ct::string glsl_type_name<u8vec3> = c_string_t<"u8vec3">;
  template<> inline constexpr ct::string glsl_type_name<u8vec2> = c_string_t<"u8vec2">;


  using mat4x4 = glm::tmat4x4<float, glm::packed_highp>;
  using mat4x3 = glm::tmat4x3<float, glm::packed_highp>;
  using mat4x2 = glm::tmat4x2<float, glm::packed_highp>;
  using mat3x4 = glm::tmat3x4<float, glm::packed_highp>;
  using mat3x3 = glm::tmat3x3<float, glm::packed_highp>;
  using mat3x2 = glm::tmat3x2<float, glm::packed_highp>;
  using mat2x4 = glm::tmat2x4<float, glm::packed_highp>;
  using mat2x3 = glm::tmat2x3<float, glm::packed_highp>;
  using mat2x2 = glm::tmat2x2<float, glm::packed_highp>;
  template<> inline constexpr ct::string glsl_type_name<mat4x4> = c_string_t<"mat4x4">;
  template<> inline constexpr ct::string glsl_type_name<mat4x3> = c_string_t<"mat4x3">;
  template<> inline constexpr ct::string glsl_type_name<mat4x2> = c_string_t<"mat4x2">;
  template<> inline constexpr ct::string glsl_type_name<mat3x4> = c_string_t<"mat3x4">;
  template<> inline constexpr ct::string glsl_type_name<mat3x3> = c_string_t<"mat3x3">;
  template<> inline constexpr ct::string glsl_type_name<mat3x2> = c_string_t<"mat3x2">;
  template<> inline constexpr ct::string glsl_type_name<mat2x4> = c_string_t<"mat2x4">;
  template<> inline constexpr ct::string glsl_type_name<mat2x3> = c_string_t<"mat2x3">;
  template<> inline constexpr ct::string glsl_type_name<mat2x2> = c_string_t<"mat2x2">;

  using f16mat4x4 = glm::tmat4x4<float16_t, glm::packed_highp>;
  using f16mat4x3 = glm::tmat4x3<float16_t, glm::packed_highp>;
  using f16mat4x2 = glm::tmat4x2<float16_t, glm::packed_highp>;
  using f16mat3x4 = glm::tmat3x4<float16_t, glm::packed_highp>;
  using f16mat3x3 = glm::tmat3x3<float16_t, glm::packed_highp>;
  using f16mat3x2 = glm::tmat3x2<float16_t, glm::packed_highp>;
  using f16mat2x4 = glm::tmat2x4<float16_t, glm::packed_highp>;
  using f16mat2x3 = glm::tmat2x3<float16_t, glm::packed_highp>;
  using f16mat2x2 = glm::tmat2x2<float16_t, glm::packed_highp>;
  template<> inline constexpr ct::string glsl_type_name<f16mat4x4> = c_string_t<"f16mat4x4">;
  template<> inline constexpr ct::string glsl_type_name<f16mat4x3> = c_string_t<"f16mat4x3">;
  template<> inline constexpr ct::string glsl_type_name<f16mat4x2> = c_string_t<"f16mat4x2">;
  template<> inline constexpr ct::string glsl_type_name<f16mat3x4> = c_string_t<"f16mat3x4">;
  template<> inline constexpr ct::string glsl_type_name<f16mat3x3> = c_string_t<"f16mat3x3">;
  template<> inline constexpr ct::string glsl_type_name<f16mat3x2> = c_string_t<"f16mat3x2">;
  template<> inline constexpr ct::string glsl_type_name<f16mat2x4> = c_string_t<"f16mat2x4">;
  template<> inline constexpr ct::string glsl_type_name<f16mat2x3> = c_string_t<"f16mat2x3">;
  template<> inline constexpr ct::string glsl_type_name<f16mat2x2> = c_string_t<"f16mat2x2">;


  /// \brief Reference a type that only exists in glsl (will not generate anything, and requiring to generate this type will trigger an error)
  /// \note Use this when the CPU side doesn't have to care about the actual type (not visible cpu-side *at all*)
  template<ct::string_holder GlslTypeName>
  struct pure_glsl_type
  {
    static constexpr neam::ct::string glsl_type_name { GlslTypeName };
  };

  /// \brief In a glsl structure, generates `InnerType name[];`
  /// \note Must be the very last element
  /// \warning Use this with [[no_unique_address]], otherwise it may affect struct size / ...
  template<typename ValueType>
  struct alignas(alignof(ValueType)) unbound_array
  {
    using value_type = ValueType;

    ValueType* data() { reinterpret_cast<ValueType*>(this); }

    ValueType& operator[](uint32_t index) { return reinterpret_cast<ValueType*>(this)[index]; }
    const ValueType& operator[](uint32_t index) const { return reinterpret_cast<const ValueType*>(this)[index]; }
  };

  // NOTE: To generate arrays of elements, use std::array<blah, xyz>
}


template<neam::ct::string_holder GlslTypeName>
N_METADATA_STRUCT_TPL(neam::hydra::shaders::pure_glsl_type, GlslTypeName)
{
  using member_list = neam::ct::type_list
  <
  >;
};
