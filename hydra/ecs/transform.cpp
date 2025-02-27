//
// created by : Timothée Feuillet
// date: 2023-10-19
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

#include "transform.hpp"

#include <glm/gtc/packing.hpp>

namespace neam::hydra
{
  static constexpr transform global_identity {};

  transform transform::compute_inverse(const transform& a)
  {
    transform ret
    {
      {},
      glm::inverse(a.rotation),
      1.0f / a.scale,
    };
    ret.translation = ret.transform_position(-a.translation);
    return ret;
  }

  transform transform::multiply(const transform& a, const transform& b)
  {

    transform ret;
    // scale (uniform scaling is easy to handle)
    ret.scale = a.scale * b.scale;

    // rotation
    ret.rotation = glm::normalize(a.rotation * b.rotation);

    // translation
    ret.translation = a.transform_position(b.translation);
    return ret;
  }

  glm::dvec3 transform::transform_position(glm::dvec3 pos) const
  {
    pos *= scale;
    pos = (glm::dquat)rotation * pos;
    pos += translation;
    return pos;
  }

  glm::vec3 transform::transform_vector(glm::vec3 vector) const
  {
    vector *= scale;
    vector = rotation * vector;
    return vector;
  }

  packed_transform transform::pack() const
  {
    packed_transform ret;
    ret.scale = scale;
    ret.grid_translation = (glm::ivec3)glm::floor(translation * 0.5);
    ret.fine_translation = (glm::u16vec3)glm::floor(glm::fract(translation * 0.5) * (double)0xFFFF);
    ret.packed_quaternion = glm::pack_quaternion(rotation);
    return ret;
  }

  static transform unpack(const packed_transform& pt)
  {
    transform ret;
    ret.scale = pt.scale;
    ret.translation = ((glm::dvec3)pt.grid_translation) * 2.0 + (glm::dvec3)pt.fine_translation * (2.0 * 0xFFFF);
    ret.rotation = glm::unpack_quaternion(pt.packed_quaternion);
    return ret;
  }

  namespace ecs::components
  {
    transform::transform(param_t p)
      : internal_component_t(p), serializable_t(*this), hierarchical_t(*this)
    {
    }

    const hydra::transform& transform::get_world_to_parent_transform() const
    {
      if (const transform* parent = get_parent(); parent != nullptr)
        return parent->world_state_inverse;
      return global_identity;
    }

    void transform::update_from_hierarchy()
    {
      local_state.rotation = glm::normalize(local_state.rotation);

      if (transform* parent = get_parent(); parent != nullptr)
        world_state = hydra::transform::multiply(parent->world_state, local_state);
      else
        world_state = local_state;

      world_state_inverse = hydra::transform::compute_inverse(world_state);
    }
  }
}

