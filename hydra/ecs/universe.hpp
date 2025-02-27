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

#include <vector>

#include "types.hpp"

namespace neam::hydra
{
  class core_context;
}
namespace neam::hydra::ecs
{
  namespace components
  {
    class hierarchy;
  }

  /// \brief Self-contained entity context
  /// enfield:::database / enfield::entity don't have a concept of entity_id, and entities don't have a fixed pointer
  /// This class handles a self-contained "universe" of entities, providing entity-ids, entity-refs
  /// The database can contain more than one universe, as long as systems can work on all universes at the same time
  /// The database can also contain out-of-universe entities, usually those contains rendering state,
  /// but referencing those entities can be dangerous unless you own the entity object
  ///
  /// \note for an entity to belong in a universe it has to have at least the hierarchy component (or better the hierarchical concept)
  ///       If your entity has transform or any other hierarchical component, nothing else is required
  class universe
  {
    public:
      using update_queue_t = cr::queue_ts<cr::queue_ts_atomic_wrapper<components::hierarchy*>, 511*4>;

    public:
      universe(database& _db);

      /// \brief Perform hierarchical update, but on a single thread. Might be slow.
      uint32_t hierarchical_update_single_thread();

      /// \brief Perform hierarchical update on multiple threads
      /// \note Does not return until the update is done
      uint32_t hierarchical_update_tasked(core_context& cctx, uint32_t max_helper_task_count = 8, uint32_t entity_per_task = 32);

      /// \brief Return the hierarchy component of the (main) universe root
      components::hierarchy& get_universe_root() { return *roots_hierarchy[0]; }
      const components::hierarchy& get_universe_root() const { return *roots_hierarchy[0]; }

      /// \brief Return the hierarchy component of the (main) universe root
      entity& get_universe_root_entity() { return roots[0]; }
      const entity& get_universe_root_entity() const { return roots[0]; }


    private:
      /// \brief universe roots (everything is held from there)
      /// \note the first entry is always guaranteed to exist and has id_t::none
      std::vector<entity> roots;
      std::vector<components::hierarchy*> roots_hierarchy;

      database& db;

      update_queue_t update_queue;
  };
}

