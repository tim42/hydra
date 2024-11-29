//
// created by : Timothée Feuillet
// date: 2023-10-14
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

#include <enfield/enfield.hpp>
#include <ntools/type_id.hpp>

namespace neam::hydra::ecs::conf
{
  namespace internal
  {
    template<enfield::type_t ClassId, typename AttachedObject>
    struct ao_name_check
    {
      static constexpr bool value = false;
    };
  }

  struct eccs
  {
      // type markers (mandatory)
      struct attached_object_class;
      struct attached_object_type;
      struct system_type;

      // mostly data structure, few functions:
      struct component_class { static constexpr enfield::type_t id = 0; };
      // internal component is a component that will not be externally visible (and thus cannot be part of queries / systems / for-each)
      // this also mean that creating / destroying internal-components is way faster.
      // they can implement concepts (their main use case).
      struct internal_component_class { static constexpr enfield::type_t id = 1; };
      struct concept_class { static constexpr enfield::type_t id = 2; };

      // the list of classes
      using classes = ct::type_list<component_class, concept_class>;

      // "rights" configuration:
      template<enfield::type_t ClassId>
      struct class_rights
      {
        // default configuration: (must be static constexpr)

        /// \brief Define general access rights
        static constexpr enfield::attached_object_access access = enfield::attached_object_access::all_no_automanaged;
      };

      template<enfield::type_t ClassId, enfield::type_t OtherClassId>
      struct specific_class_rights : public class_rights<ClassId> {};

      template<enfield::type_t ClassId, typename AttachedObject>
      struct check_attached_object : internal::ao_name_check<ClassId, AttachedObject> {};


      static constexpr uint64_t max_attached_objects_types = 6 * 64;

      using attached_object_allocator = enfield::default_attached_object_allocator<eccs>;

      static constexpr bool use_attached_object_db = true;
      static constexpr bool use_entity_db = true;

      static constexpr bool allow_ref_counting_on_entities = true;
  };
  template<>
  struct eccs::class_rights<eccs::concept_class::id>
  {
    static constexpr enfield::attached_object_access access = enfield::attached_object_access::automanaged
                                                            | enfield::attached_object_access::ao_unsafe_getable
                                                            | enfield::attached_object_access::ext_getable;
  };
  template<>
  struct eccs::class_rights<eccs::internal_component_class::id>
  {
    static constexpr enfield::attached_object_access access = enfield::attached_object_access::ao_all_safe
                                                            // We (may) allow the external creation of the components (is it needed?)
                                                            /*
                                                            | enfield::attached_object_access::ext_creatable
                                                            | enfield::attached_object_access::ext_removable*/;
  };

  namespace internal
  {
    template<typename AttachedObject>
    struct ao_name_check<eccs::component_class::id, AttachedObject>
    {
      static constexpr bool value = ((ct::string)ct::type_name<AttachedObject>).contains("::components::");
      static_assert(value, "invalid type-name for component (must be in a `components` namespace)");
    };
    template<typename AttachedObject>
    struct ao_name_check<eccs::internal_component_class::id, AttachedObject>
    {
      static constexpr bool value = ((ct::string)ct::type_name<AttachedObject>).contains("::internals::");
      static_assert(value, "invalid type-name for internal_component (must be in a `internals` namespace)");
    };
    template<typename AttachedObject>
    struct ao_name_check<eccs::concept_class::id, AttachedObject>
    {
      static constexpr bool value = ((ct::string)ct::type_name<AttachedObject>).contains("::concepts::");
      static_assert(value, "invalid type-name for concepts (must be in a `concepts` namespace)");
    };
  }


  /// \brief This is a base component, and should be inherited by components. See comment for internal_component_class
  /// \tparam DatabaseConf is the configuration object for the database
  /// \tparam ComponentType is the final component type
  template<typename DatabaseConf, typename ComponentType>
  class internal_component : public enfield::attached_object::base_tpl<DatabaseConf, typename DatabaseConf::internal_component_class, ComponentType>
  {
    public:
      using param_t = typename enfield::attached_object::base<DatabaseConf>::param_t;
      using internal_component_t  = internal_component<DatabaseConf, ComponentType>;

      virtual ~internal_component() = default;

    protected:
      internal_component(param_t _param)
        : enfield::attached_object::base_tpl<DatabaseConf, typename DatabaseConf::internal_component_class, ComponentType>
          (
            _param
          )
      {
      }
  };
}

