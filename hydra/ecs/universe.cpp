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

#include "universe.hpp"
#include "hierarchy.hpp"

#include "../engine/core_context.hpp"

namespace neam::hydra::ecs
{
  universe::universe(database& _db) : db(_db)
  {
    // so the state is always correct
    roots.emplace_back(db.create_entity());
    roots_hierarchy.emplace_back(&roots[0].add<components::hierarchy>());
    roots_hierarchy[0]->self_id = entity_id_t::none; // main root id is always none
  }

  uint32_t universe::hierarchical_update_single_thread()
  {
    std::deque<components::hierarchy*> queue;
    // prime the queue:
    for (auto& it : roots)
    {
      components::hierarchy* ptr = it.get<components::hierarchy>();
      check::debug::n_assert(ptr != nullptr, "universe::update-st: universe root doesn't have a hierarchy component");
      queue.push_back(ptr);
    }

    // update the universe:
    uint32_t count = 0;
    while (!queue.empty())
    {
      components::hierarchy* ptr = queue.front();
      queue.pop_front();
      ptr->update();
      ptr->update_children(queue);
      ++count;
    }
    return count;
  }

  uint32_t universe::hierarchical_update_tasked(core_context& cctx, uint32_t max_helper_task_count, uint32_t entity_per_task)
  {
    // FIXME:
    return hierarchical_update_single_thread();
    // for (uint64_t i = 0; i < 409114; ++i)
    // {
    //   update_queue.push_back((components::hierarchy*)(i+1));
    // }
    // while (!update_queue.empty())
    // {
    //   components::hierarchy* ptr;
    //   update_queue.try_pop_front(ptr);
    // }
    // return 409114;

//         // max_helper_task_count = 16;
//     std::atomic<uint32_t> task_count = 1;
//     std::atomic<uint32_t> ret = 0;
//     const uint64_t depth_limit = 10;
//     std::deque<uint64_t> uq;
//     spinlock uqlock;
//
//     auto try_pop_front = [&uq, &uqlock](uint64_t& x)
//     {
//       std::lock_guard _lg(uqlock);
//       if (uq.empty()) return false;
//       x = uq.front();
//       uq.pop_front();
//       return true;
//     };
//     {
//       // prime the queue:
//       uq.push_back(1);
//       ret.fetch_add(1, std::memory_order_release);
//
//       const auto task_function = [&]
//       {
//         while (!uq.empty())
//         {
//           uint64_t ptr;
//           [[likely]] if (try_pop_front(ptr))
//           {
//             uint64_t depth = (uint64_t)ptr;
//             if (depth < depth_limit)
//             for (uint32_t i = 0; i < depth; ++i)
//             {
//               {
//                 std::lock_guard _lg(uqlock);
//                 uq.push_back((depth + 1));
//               }
//               ret.fetch_add(1, std::memory_order_release);
//             }
//           }
//         }
//         task_count.fetch_sub(1, std::memory_order_release);
//       };
//
//       while (!uq.empty() || task_count.load(std::memory_order_acquire) > 1)
//       {
//         uint64_t ptr;
//         [[likely]] if (try_pop_front(ptr))
//         {
//           uint64_t depth = (uint64_t)ptr;
//           if (depth < depth_limit)
//           for (uint32_t i = 0; i < depth; ++i)
//           {
//               {
//                 std::lock_guard _lg(uqlock);
//                 uq.push_back((depth + 1));
//               }
//             ret.fetch_add(1, std::memory_order_release);
//           }
//         }
//
//         // dispatch tasks:
//         const uint32_t remaining_count = uq.size();
//         const uint32_t expected_task_count = std::min(max_helper_task_count, remaining_count / entity_per_task);
//         uint32_t actual_task_count = task_count.load(std::memory_order_acquire);
//         while (actual_task_count < expected_task_count)
//         {
//           ++actual_task_count;
//           task_count.fetch_add(1, std::memory_order_release);
//           cctx.tm.get_task(task_function);
//         }
//       }
//       task_count.fetch_sub(1, std::memory_order_release);
//
//       // spin (FIXME: Should not happen)
//       check::debug::n_check(task_count == 0, "still has tasks");
//       check::debug::n_assert(uq.empty(), "not empty :(");
//     }
//     return ret;
// #if 0
    max_helper_task_count = 12;
    std::atomic<uint32_t> task_count = 1;
    std::atomic<uint32_t> ret = 0;
    const uint64_t depth_limit = 10;

    {
      // prime the queue:
      update_queue.push_back((components::hierarchy*)1);
      ret.fetch_add(1, std::memory_order_release);

      const auto task_function = [&]
      {
        while (!update_queue.empty())
        {
          components::hierarchy* ptr;
          [[likely]] if (update_queue.try_pop_front(ptr))
          {
            uint64_t depth = (uint64_t)ptr;
            if (depth < depth_limit)
            for (uint32_t i = 0; i < depth; ++i)
            {
              update_queue.push_back((components::hierarchy*)(depth + 1));
              ret.fetch_add(1, std::memory_order_release);
            }
          }
        }
        task_count.fetch_sub(1, std::memory_order_release);
      };

      while (!update_queue.empty() || task_count.load(std::memory_order_acquire) > 1)
      {
        components::hierarchy* ptr;
        [[likely]] if (update_queue.try_pop_front(ptr))
        {
          uint64_t depth = (uint64_t)ptr;
          if (depth < depth_limit)
          for (uint32_t i = 0; i < depth; ++i)
          {
            update_queue.push_back((components::hierarchy*)(depth + 1));
            ret.fetch_add(1, std::memory_order_release);
          }
        }

        // dispatch tasks:
        const uint32_t remaining_count = update_queue.size();
        const uint32_t expected_task_count = std::min(max_helper_task_count, remaining_count / entity_per_task);
        uint32_t actual_task_count = task_count.load(std::memory_order_acquire);
        while (actual_task_count < expected_task_count)
        {
          ++actual_task_count;
          task_count.fetch_add(1, std::memory_order_release);
          cctx.tm.get_task(task_function);
        }
      }
      task_count.fetch_sub(1, std::memory_order_release);

      // spin (FIXME: Should not happen)
      check::debug::n_check(task_count == 0, "still has tasks");
      check::debug::n_assert(update_queue.empty(), "not empty :(");
    }
    return ret;
// #endif
#if 0
    // max_helper_task_count = 16;
    std::atomic<uint32_t> task_count = 1;

    {
      // prime the queue:
      for (auto& it : roots)
      {
        components::hierarchy* ptr = it.get<components::hierarchy>();
        check::debug::n_assert(ptr != nullptr, "universe::update-st: universe root doesn't have a hierarchy component");
        update_queue.push_back(std::move(ptr));
      }

      const auto task_function = [&]
      {
        while (!update_queue.empty())
        {
          components::hierarchy* ptr;
          [[likely]] if (update_queue.try_pop_front(ptr))
          {
            ptr->update();
            ptr->update_children(update_queue);
          }
        }
        task_count.fetch_sub(1, std::memory_order_release);
      };

      while (!update_queue.empty() && task_count.load(std::memory_order_relaxed) >= 1)
      {
        components::hierarchy* ptr;
        [[likely]] if (update_queue.try_pop_front(ptr))
        {
          ptr->update();
          constexpr uint32_t k_children_batch = 1024;
          uint32_t index = 0;
          for (index = 0; ptr->update_children(update_queue, index, k_children_batch); index += k_children_batch)
          {
            // dispatch tasks:
            const uint32_t remaining_count = update_queue.size();
            const uint32_t expected_task_count = std::min(max_helper_task_count, remaining_count / entity_per_task);
            uint32_t actual_task_count = task_count.load(std::memory_order_acquire);
            while (actual_task_count < expected_task_count)
            {
              ++actual_task_count;
              task_count.fetch_add(1, std::memory_order_release);
              cctx.tm.get_task(task_function);
            }
          }
        }

        // dispatch tasks:
        const uint32_t remaining_count = update_queue.size();
        const uint32_t expected_task_count = std::min(max_helper_task_count, remaining_count / entity_per_task);
        uint32_t actual_task_count = task_count.load(std::memory_order_acquire);
        while (actual_task_count < expected_task_count)
        {
          ++actual_task_count;
          task_count.fetch_add(1, std::memory_order_release);
          cctx.tm.get_task(task_function);
        }
      }
      task_count.fetch_sub(1, std::memory_order_release);

      // spin (FIXME: Should not happen)
      while (task_count.load(std::memory_order_acquire))
      {
        while (task_count.load(std::memory_order_relaxed)) {}
      }
      while (!update_queue.empty())
      {
        components::hierarchy* ptr;
        update_queue.try_pop_front(ptr);
      }
    }
#endif
  }
}

