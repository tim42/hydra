//
// created by : Timothée Feuillet
// date: 2023-10-20
//
//
// Copyright (c) 2023 Timothée Feuillet
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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SWIZZLE

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/ulp.hpp>

// hydra extensions to glm
namespace glm
{
  static constexpr uint64_t k_nearly_equal_max_distance_double = 3;
  static constexpr uint32_t k_nearly_equal_max_distance_float = 2;

  constexpr bool is_nearly_equal(float x, float y)
  {
    return (uint32_t)glm::abs(std::bit_cast<int32_t>(x) - std::bit_cast<int32_t>(y)) <= k_nearly_equal_max_distance_float;
  }
  constexpr bool is_nearly_equal(double x, double y)
  {
    return (uint64_t)glm::abs(std::bit_cast<int64_t>(x) - std::bit_cast<int64_t>(y)) <= k_nearly_equal_max_distance_double;
  }

  template<length_t L, typename T, qualifier Q>
  constexpr vec<L, bool, Q> is_nearly_equal(vec<L, T, Q> const& x, vec<L, T, Q> const& y)
  {
    vec<L, bool, Q> result;
    for (length_t i = 0; i < L; ++i)
      result[i] = is_nearly_equal(x[i], y[i]);
    return result;
  }

  inline constexpr glm::i8vec4 pack_quaternion(glm::quat q)
  {
    const glm::vec4 r = glm::clamp(glm::normalize(glm::vec4(q.x, q.y, q.z, q.w)), -1.f, 1.f);
    return (glm::i8vec4)glm::floor(r * (float)0x7F);
  }
  inline constexpr glm::quat unpack_quaternion(glm::i8vec4 p)
  {
    return glm::quat(((glm::vec4)p) / (float)0x7F);
  }

  constexpr glm::quat tbn_to_quat(glm::vec3 t, glm::vec3 b, glm::vec3 n)
  {
    return glm::quat_cast(glm::mat3{ t, b, n });
  }
  constexpr glm::i8vec4 pack_tbn(glm::vec3 t, glm::vec3 b, glm::vec3 n)
  {
    return pack_quaternion(tbn_to_quat(t, b, n));
  }
}

