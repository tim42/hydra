//
// created by : Timothée Feuillet
// date: 2023-9-8
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

#include "deferred_queue_execution.hpp"

#include <ntools/tracy.hpp>

namespace neam::hydra
{
  static constexpr bool dqe_immediate_mode = true;

  uint32_t deferred_queue_execution::get_index(id_t qid)
  {
    std::lock_guard _lgs {spinlock_shared_adapter::adapt(queue_id_lock) };
    const uint32_t current_next_index = next_index;
    for (const auto& it : id_to_index)
    {
      [[likely]] if (it.first == qid)
        return it.second;
    }

    std::lock_guard _lge {spinlock_shared_to_exclusive_adapter::adapt(queue_id_lock) };
    if (current_next_index != next_index) [[unlikely]]
    {
      // search again (TODO: only search the new indices)
      for (const auto& it : id_to_index)
      {
        [[likely]] if (it.first == qid)
          return it.second;
      }
    }

    check::debug::n_assert(next_index < k_max_queues, "deferred_queue_execution::get_index: reached the max number of supported queues ({})", k_max_queues);

    id_to_index[next_index] = { qid, next_index };
    ++next_index;
    return next_index - 1;
  }

  void deferred_queue_execution::defer_execution_unlocked(id_t queue_id, threading::function_t&& fnc)
  {
    if (dqe_immediate_mode)
    {
      fnc();
      return;
    }
    check::debug::n_assert(queue_id != id_t::invalid, "defer_execution: queue_id is invalid");

    N_MTC_WRITER_SCOPE;
    // ensure we have somewhere to push:
    if (to_submit.empty())
      to_submit.emplace_back();

    const uint32_t index = get_index(queue_id);
    to_submit.back().lists[index].emplace_back(std::move(fnc));
    to_submit.back().has_any_submissions = true;
  }

  void deferred_queue_execution::defer_sync_unlocked()
  {
    if (dqe_immediate_mode)
      return;
    N_MTC_WRITER_SCOPE;
    if (to_submit.empty() || to_submit.back().has_any_submissions)
      to_submit.emplace_back();
  }

  void deferred_queue_execution::_execute_deferred_tasks_synchronously_single_threaded()
  {
    if (dqe_immediate_mode)
      return;
    TRACY_SCOPED_ZONE;
    decltype(to_submit) submissions;
    {
      // minimal lock:
      std::lock_guard _lg(lock);
      N_MTC_WRITER_SCOPE;
      std::swap(to_submit, submissions);
    }
    {
      const uint32_t queue_count = next_index;
      for (auto& it : submissions)
      {
        if (!it.has_any_submissions) continue;

        for (uint32_t i = 0; i < queue_count; ++i)
        {
          if (it.lists[i].empty()) continue;

          for (auto& fnc : it.lists[i])
            fnc();
        }
      }
    }
  }

  void deferred_queue_execution::execute_deferred_tasks(threading::group_t group)
  {
    if (dqe_immediate_mode)
      return;
    TRACY_SCOPED_ZONE;

    decltype(to_submit) submissions;
    {
      // minimal lock:
      std::lock_guard _lg(lock);
      N_MTC_WRITER_SCOPE;
      std::swap(to_submit, submissions);
    }

    if (group == threading::k_invalid_task_group)
      group = tm.get_current_group();

    // force deferred execution (and early exit of the current function)
    tm.get_task(group, [submissions = std::move(submissions), group, &tm = tm, queue_count = next_index] mutable
    {
      TRACY_SCOPED_ZONE;
      // We use a dependency scheme that allows us to both prepare and run tasks at the same time
      threading::task_wrapper previous_sync_task = tm.get_task(group, []{});
      for (auto& it : submissions)
      {
        if (!it.has_any_submissions) continue;

        threading::task_wrapper next_sync_task = tm.get_task(group, [] {});

        for (uint32_t i = 0; i < queue_count; ++i)
        {
          if (it.lists[i].empty()) continue;

          threading::task_wrapper task = tm.get_task(group, [list = std::move(it.lists[i])] mutable
          {
            TRACY_SCOPED_ZONE;
            for (auto& fnc : list)
            {
              TRACY_SCOPED_ZONE;
              fnc();
            }
          });

          // add the dependencies:
          task->add_dependency_to(previous_sync_task);
          next_sync_task->add_dependency_to(task);
        }

        // allow the execution of the previously scheduled tasks
        // (this will release previous_sync_task at the end of this scope, which will allow it to run,
        //  thus also allowing the dependent tasks to run as well)
        std::swap(next_sync_task, previous_sync_task);
      }

      // at the scope exit, the last batch will be allowed to run
    });
  }
}

