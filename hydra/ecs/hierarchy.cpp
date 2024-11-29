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

#include "hierarchy.hpp"
#include "universe.hpp"

namespace neam::hydra::ecs
{
  namespace components
  {
    hierarchy::hierarchy(param_t p): component_t(p), serializable_t(*this)
    {
      if (has_persistent_data())
        refresh_from_deserialization();
    }

    const hierarchy::base_t* hierarchy::get_attached_object_in_parents(enfield::type_t type) const
    {
      for (const auto& it : attached_objects)
      {
        if (it.first == type)
          return it.second;
      }
      return nullptr;
    }

    void hierarchy::update()
    {
      concepts::hierarchical* const hierarchical_con = get_unsafe<concepts::hierarchical>();

      // release any attached objects that are un-necessary anymore
      // if (hierarchical_con)
        // hierarchical_con->unrequire_components();

      // update attached_objects:
      if (parent.is_valid())
      {
        enfield::inline_mask<db_conf> mask;
        decltype(attached_objects) new_attached_objects;
        if (concepts::hierarchical* const parent_hierarchical_con = parent.get<concepts::hierarchical>(); parent_hierarchical_con != nullptr)
        {
          new_attached_objects.reserve(parent_hierarchical_con->get_concept_providers_count());
          parent_hierarchical_con->for_each_concept_provider([&](concepts::hierarchical::concept_logic& hlgc)
          {
            mask.set(hlgc.get_base().object_type_id);
            new_attached_objects.emplace_back(hlgc.get_base().object_type_id, &hlgc.get_base());
          });
        }

        if (const hierarchy* const parent_hc = parent.get<hierarchy>(); parent_hc)
        {
          new_attached_objects.reserve(parent_hc->attached_objects.size());
          for (const auto& it : parent_hc->attached_objects)
          {
            if (mask.is_set(it.first)) continue;
            new_attached_objects.emplace_back(it);
          }
        }

        attached_objects = std::move(new_attached_objects);
      }

      // update the components
      if (hierarchical_con)
        hierarchical_con->update();

      // lock any components that are remaining so that pointers are valid until next update
      // if (hierarchical_con)
        // hierarchical_con->require_components();
    }

    void hierarchy::update_children(universe::update_queue_t& update_queue)
    {
      update_children(update_queue, 0, children.size());
    }

    void hierarchy::update_children(std::deque<components::hierarchy*>& update_queue)
    {
      update_children(update_queue, 0, children.size());
    }

    bool hierarchy::update_children(std::deque<components::hierarchy*>& update_queue, uint32_t start, uint32_t count)
    {
      // queue children to the update queue:
      const uint32_t end = (uint32_t)children.size();
      for (uint32_t i = 0; start + i < end && i < count; ++i)
      {
        hierarchy* ptr = children_hierarchy[start + i];
        update_queue.push_back(std::move(ptr));
      }
      if (start + count < end)
        return true;
      return false;
    }

    bool hierarchy::update_children(universe::update_queue_t& update_queue, uint32_t start, uint32_t count)
    {
      // queue children to the update queue:
      const uint32_t end = (uint32_t)children.size();
      for (uint32_t i = 0; start + i < end && i < count; ++i)
      {
        hierarchy* ptr = children_hierarchy[start + i];
        update_queue.push_back(std::move(ptr));
      }
      if (start + count < end)
        return true;
      return false;
    }

    entity_weak_ref hierarchy::create_child()
    {
      entity child = get_database().create_entity();
      entity_weak_ref wref = child.weak_reference();
      add_orphaned_child(std::move(child));
      return wref;
    }

    void hierarchy::remove_child(const entity_weak_ref& child)
    {
      for (uint32_t i = 0; i < (uint32_t)children.size(); ++i)
      {
        if (children[i].is_tracking_same_entity(child))
        {
          clear_child_for_removal(children[i]);
          children.erase(children.begin() + i);
          children_hierarchy.erase(children_hierarchy.begin() + i);
          return;
        }
      }
    }

    void hierarchy::remove_child(const entity& child)
    {
      for (uint32_t i = 0; i < (uint32_t)children.size(); ++i)
      {
        if (children[i].is_tracking_same_entity(child))
        {
          clear_child_for_removal(children[i]);
          children.erase(children.begin() + i);
          children_hierarchy.erase(children_hierarchy.begin() + i);
          return;
        }
      }
    }

    void hierarchy::add_orphaned_child(entity&& child)
    {
      hierarchy* ptr = child.get<hierarchy>();
      if (ptr == nullptr)
        ptr = &child.add<hierarchy>();

      {
        check::debug::n_assert(!ptr->parent.is_valid(), "hierarchy::update: entity to add is not orphaned");
        ptr->uni = uni;
        ptr->parent = create_entity_weak_reference_tracking();
        ptr->parent_id = self_id;
      }
      children_hierarchy.push_back(child.get<hierarchy>());
      children.push_back(std::move(child));
    }

    void hierarchy::remove_all_children()
    {
      for (uint32_t i = 0; i < (uint32_t)children.size(); ++i)
        clear_child_for_removal(children[i]);
      children.clear();
      children_hierarchy.clear();
    }

    void hierarchy::clear_child_for_removal(entity& child)
    {
      if (hierarchy* ptr = child.get<hierarchy>(); ptr != nullptr)
      {
        ptr->uni = nullptr;
        ptr->parent.release();
        ptr->parent_id = entity_id_t::invalid;
      }
    }

    void hierarchy::refresh_from_deserialization()
    {
      const internal::serialized_hierarchy sh = get_persistent_data();

      if (parent_id != sh.parent)
      {
        const entity_id_t old_parent_id = parent_id;
        if (old_parent_id != entity_id_t::invalid)
        {
          // TODO: unregister from parent if universe is setup
        }

        parent_id = sh.parent;
        self_id = sh.self;

        // FIXME: FINISH
      }
    }

    internal::serialized_hierarchy hierarchy::get_data_to_serialize() const
    {
      return { .parent = parent_id, .self = self_id };
    }
  }

  namespace concepts
  {
    bool hierarchical::concept_logic::is_dirty(bool recursive_check) const
    {
      if (last_update_token == 0) return true;
      if (parent != nullptr)
      {
        if (!recursive_check)
          return parent->last_update_token != last_update_token;
        else
          return parent->is_dirty(true);
      }
      return false;
    }

    void hierarchical::concept_logic::do_update_dirty_flag()
    {
      // update the parent (and overwrite the update token with the value from the parent)
      if (parent != nullptr)
      {
        last_update_token = parent->last_update_token;
      }
      else
      {
        // update the token
        last_update_token += 1;
        if (last_update_token == 0)
          last_update_token = 1;
      }
    }

    void hierarchical::require_components()
    {
      for_each_concept_provider([](concept_logic& lg)
      {
        lg.concept_require();
      });
    }

    void hierarchical::unrequire_components()
    {
      for_each_concept_provider([](concept_logic& lg)
      {
        lg.concept_try_unrequire();
      });
    }

    void hierarchical::update()
    {
      for_each_concept_provider([this](concept_logic& lg)
      {
        lg.update_logic(hierarchy_component);
        if (lg.is_dirty() || everything_is_dirty || true)
        {
          lg.update_provider();
          lg.update_dirty_flag();
        }
      });
      everything_is_dirty = false;
    }
  }
}

