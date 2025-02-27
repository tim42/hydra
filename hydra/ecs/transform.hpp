//
// created by : Timothée Feuillet
// date: 2023-10-15
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

#include "types.hpp"
#include "hierarchy.hpp"

#include <hydra_glm.hpp>

namespace neam::hydra
{
  /// \brief Serializable transform
  /// \note lossy, but within reason
  struct packed_transform
  {
    glm::ivec3 grid_translation;
    glm::u16vec3 fine_translation; // waste of 16b here
    float scale;
    glm::i8vec4 packed_quaternion; // snorm8[4]
  };

  /// \brief Handle transforms
  /// \warning Non-uniform scale is not supported
  class transform
  {
    public: // api
      static constexpr transform identity() { return {}; }

      static transform compute_inverse(const transform& a);

      /// \brief a * b
      /// (transform b by a)
      static transform multiply(const transform& a, const transform& b);

      glm::dvec3 transform_position(glm::dvec3 pos) const; // rotation, scale, translation
      glm::vec3 transform_vector(glm::vec3 vector) const; // rotation, scale, no translation

      packed_transform pack() const;
      static transform unpack(const packed_transform& pt);

    public: // state
      glm::dvec3 translation {0, 0, 0};
      glm::quat rotation {1.0f, 0.0f, 0.0f, 0.0f};
      float scale = 1;
      // 32 bits lost to padding
  };
}

N_METADATA_STRUCT(neam::hydra::transform)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(translation),
    N_MEMBER_DEF(rotation),
    N_MEMBER_DEF(scale)
  >;
};

namespace neam::hydra::ecs::components
{
  /// \brief Transform for entities.
  /// \note positions are in double, rotations, scales are in float
  ///       packed positions are (uint+unorm16)[3], packed scales are float3, rotations are unorm8[3] + packed-signs (uint8)
  ///
  /// \note If the local-transform is identity, this component is not needed and should be removed.
  ///       Entitites without transforms simply use the transform of their parent
  ///       Do not require<> this component, rather, require< hierarchy > and call
  ///       hierarchy.get< transform > when needed. It will return the closest transform component / nullptr if none are present.
  class transform : public internal_component<transform>,
    public serializable::concept_provider<transform>,
    public concepts::hierarchical::concept_provider<transform>
  {
    public:
      transform(param_t p);

      hydra::transform& update_local_transform() { set_dirty(); return local_state; }
      const hydra::transform& get_local_transform() const { return local_state; }

      /// \note if a parent is dirty, will not accound for the upcoming change.
      void set_local_position_from_world_position(glm::dvec3 _world_position);

      const hydra::transform& get_local_to_world_transform() const { return world_state; }
      const hydra::transform& get_world_to_local_transform() const { return world_state_inverse; }

      /// \brief yield a world to parent-local transform
      /// Usefull for gizmo and other manipulator, as it's an inverse transform that excludes the local state
      const hydra::transform& get_world_to_parent_transform() const;

    private: // hierarchical
      void update_from_hierarchy();

    private: // serialization
      void refresh_from_deserialization()
      {
        update_local_transform() = get_persistent_data();
      }

      // we only serialize the local data, as everything else can be easily reconstructed from it
      hydra::transform get_data_to_serialize() const { return local_state; }

    private: // data
      hydra::transform local_state;
      hydra::transform world_state;
      hydra::transform world_state_inverse;
      // FIXME: Store the local-inverse?

      friend serializable_t;
      friend hierarchical_t;
  };
}

