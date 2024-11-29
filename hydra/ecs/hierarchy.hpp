//
// created by : Timothée Feuillet
// date: 2023-10-7
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
#include <enfield/concept/serializable.hpp>

#include <ntools/rle/rle.hpp>

#include "types.hpp"
#include "universe.hpp"

namespace neam::hydra::ecs::internal
{
  struct serialized_hierarchy
  {
    // We only serialize the parent, not any children.
    // After scene deserialization, children register to their parents
    // (this is done so the serialized data is easier to keep consistent AND is slightly smaller)
    // NOTE: Only the universe root is allowed to have entity_id_t::none parent
    // (scene root must have a parent, either another scene or the universe root)
    entity_id_t parent;
    entity_id_t self;
  };
}

N_METADATA_STRUCT(neam::hydra::ecs::internal::serialized_hierarchy)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(parent),
    N_MEMBER_DEF(self)
  >;
};


namespace neam::hydra::ecs
{
  class universe;

  namespace concepts
  {
    class hierarchical;
  }

  namespace components
  {
    /// \brief Handle everything hierarchy related in entities. Completed by the hierarchical concept.
    /// \note Any entity part of a universe must have this component.
    ///
    /// entity_id_t::none is the universe root.
    class hierarchy : public component<hierarchy>, public serializable::concept_provider<hierarchy>
    {
      // NOTE: hierarchy update process:
      //        - update the current entity
      //          - unrequire all the hierarchical components
      //          - copy attached_objects from the parent (if any), and update it with its components
      //          - update the remaining ones
      //          - require all the hierarchical components (prevent their destruction until next update)
      //        - for every children:
      //          - push them in the to-update list
      //        - set the current entity to the first entry in the to-update list
      //          - optional: dispatch tasks to help with the update process (up-to a max number of update tasks)
      public:
        hierarchy(param_t p);

        [[nodiscard]] entity_weak_ref create_parent_tracking_reference() const { return parent.duplicate_tracking_reference(); }

        /// \brief Return a tracked attached-object, searching in the parents if the current entity doesn't contain it
        /// \note Might return nullptr if no parent ever contain that attached-object
        /// \note AttachedObject cannot be a concept (must be requirable)
        /// \warning require the ao_unsafe_getable access right (component&concept -> classof(AttachedObject))
        template<typename AttachedObject>
        AttachedObject* get(bool include_self = true)
        {
          return const_cast<AttachedObject*>(static_cast<const hierarchy*>(this)->get<AttachedObject>(include_self));
        }

        template<typename AttachedObject>
        const AttachedObject* get(bool include_self = true) const
        {
          // Check if the current entity has the attached-object
          if (include_self)
          {
            if (const AttachedObject* ptr = component_t::get_unsafe<AttachedObject>(); ptr != nullptr)
              return ptr;
          }
          if (const base_t* ptr = get_attached_object_in_parents(type_id<AttachedObject>::id()); ptr != nullptr)
            return static_cast<const AttachedObject*>(ptr);
          return nullptr;
        }

        /// \brief Create a children entity, and add the hierarchy component as externally-added
        [[nodiscard]] entity_weak_ref create_child();

        void add_orphaned_child(entity&& child);

        /// \brief Remove a child given a weak-ref to it
        /// \note the weak ref might become invalid afterward
        void remove_child(const entity_weak_ref& child);

        /// \brief Remove a child given a strong-ref to it
        /// \note the ex-child entity and its hierarchy is in an invalid state,
        ///       but operations on it might still work until the next hierarchical update
        ///       if kept orphaned after an universe update, crash or memory corruption might happens
        /// \note reparenting is remove then add
        void remove_child(const entity& child);

        /// \brief Remove all children from the hierarchy
        void remove_all_children();

      private: // serialization
        void refresh_from_deserialization();
        internal::serialized_hierarchy get_data_to_serialize() const;

      private: // internal:
        const base_t* get_attached_object_in_parents(enfield::type_t type) const;

        void clear_child_for_removal(entity& child);

        /// \brief Update the hierarchical components and perform some of the update process
        /// (does not perform any recusrion)
        /// \warning If called, children must be updated as well
        void update();

        void update_children(universe::update_queue_t& update_queue);
        bool update_children(universe::update_queue_t& update_queue, uint32_t start, uint32_t count);
        void update_children(std::deque<components::hierarchy*>& update_queue);
        bool update_children(std::deque<components::hierarchy*>& update_queue, uint32_t start, uint32_t count);

      private: // data:
        // NOTE: parent-id / self-id are only valid during the serialization / deserialization process.
        entity_id_t parent_id = entity_id_t::invalid;
        entity_id_t self_id = entity_id_t::invalid;

        // populated during whole scene deserialization:
        universe* uni = nullptr;

        /// \brief children list. Strongly hold them so they aren't destructed for no reason
        std::vector<entity> children;
        std::vector<hierarchy*> children_hierarchy;

        /// \brief direct reference to the parent, if any.
        entity_weak_ref parent;

        std::vector<std::pair<enfield::type_t, base_t*>> attached_objects;

        friend serializable_t;
        friend universe;
        friend concepts::hierarchical;
    };
  }

  namespace concepts
  {
    /// \brief Concept of hierarchy
    /// \note Can only be implemented by requirable stuff (components / internal_components)
    ///
    /// Attached-objects implementing the concept only need to have a update_from_hierarchy() function
    class hierarchical : public ecs_concept<hierarchical>
    {
      private:
        using ecs_concept = ecs::ecs_concept<hierarchical>;

        class concept_logic : public ecs_concept::base_concept_logic
        {
          protected:
            concept_logic(typename ecs_concept::base_t& _base) : ecs_concept::base_concept_logic(_base) {}

          public:
            /// \brief Return whether the component is dirty or not.
            /// \note \param recursive_check do recursively check all parents if they are dirty or not
            ///       If false (the default), only check if the parent has a different state than this.
            bool is_dirty(bool recursive_check = false) const;
            /// \brief Force an update (and force an update of all children)
            void set_dirty() { last_update_token = 0; }

          protected:

          private:
            /// \brief perform the actual update (update this class and the concept-provider)
            virtual void update_logic(components::hierarchy& hc) = 0;
            virtual void update_provider() = 0;
            virtual void update_dirty_flag() = 0;
            virtual void concept_require() = 0;
            virtual void concept_try_unrequire() = 0;

          private:
            void do_update_dirty_flag();

          private:
            uint16_t last_update_token = 0; // 0 means "force-update", when wrap around, will always go to one.
            concept_logic* parent = nullptr;


            friend hierarchical;
            friend components::hierarchy;
        };

      public:
        template<typename ConceptProvider>
        class concept_provider : public concept_logic
        {
          protected:
            using hierarchical_t = concept_provider<ConceptProvider>;

            concept_provider(ConceptProvider& _p) : concept_logic(static_cast<typename ecs_concept::base_t&>(_p)) {}

            /// \brief Get the parent concept provider
            ConceptProvider* get_parent()
            {
              return static_cast<ConceptProvider*>(parent);
            }

            const ConceptProvider* get_parent() const
            {
              return static_cast<const ConceptProvider*>(parent);
            }

          private:
            void update_logic(components::hierarchy& hc) final
            {
              concept_logic* new_parent = hc.get<ConceptProvider>(false);
              if (parent != new_parent)
                set_dirty();
              parent = new_parent;
            }
            void update_provider() final
            {
              static_cast<ConceptProvider*>(this)->update_from_hierarchy();
            }
            void update_dirty_flag() final
            {
              concept_logic::do_update_dirty_flag();
            }

            static constexpr bool k_can_use_require_and_unrequire =
              enfield::dbconf_can<db_conf, ConceptProvider::ao_class_id, hierarchical::ao_class_id, enfield::attached_object_access::ao_requireable>()
              && enfield::dbconf_can<db_conf, ConceptProvider::ao_class_id, hierarchical::ao_class_id, enfield::attached_object_access::ao_removable>();

            void concept_require() final
            {
              if constexpr (k_can_use_require_and_unrequire)
              {
                get_concept().template require<ConceptProvider>();
              }
            }

            void concept_try_unrequire() final
            {
              if constexpr (k_can_use_require_and_unrequire)
              {
                // avoid an assert if the thing is not required
                if (get_concept().template is_required<ConceptProvider>())
                  get_concept().template unrequire<ConceptProvider>();
              }
            }

          protected:
          private:
        };

      public:
        hierarchical(typename ecs_concept::param_t p) : ecs_concept(p) {}

        void update();
        void force_everything_dirty() { everything_is_dirty = true; }

      private:
        void unrequire_components();
        void require_components();

      private:
        components::hierarchy& hierarchy_component = require<components::hierarchy>();

        bool everything_is_dirty = true;
        friend ecs_concept;
        friend components::hierarchy;
    };
  }
}

