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

#pragma once

#include <vector>

#include <ntools/mt_check/mt_check_base.hpp>
#include <ntools/threading/threading.hpp>

namespace neam::hydra
{
  /// \brief Handle deferred/parallel queue execution
  /// \warning Must be externally synchronised (via the lock provided w/ get_lock())
  /// \note: used by the vulkan layer of hydra, so it should not depend on any hydra:: stuff
  class deferred_queue_execution : public neam::cr::mt_checked<deferred_queue_execution>
  {
    public:
      deferred_queue_execution(threading::task_manager& _tm) : tm(_tm) {}

      /// \brief Execute all the deferred tasks
      void execute_deferred_tasks(threading::group_t group = threading::k_invalid_task_group);

      void defer_execution_unlocked(id_t queue_id, threading::function_t&& fnc);
      void defer_execution_unlocked(threading::function_t&& fnc) { return defer_execution_unlocked(id_t::none, std::move(fnc)); }

      void defer_execution(id_t queue_id, threading::function_t&& fnc)
      {
        std::lock_guard _l{lock};
        defer_execution_unlocked(queue_id, std::move(fnc));
      }
      void defer_execution(threading::function_t&& fnc) { return defer_execution(id_t::none, std::move(fnc)); }


      /// \note Require the lock to be held!
      void defer_sync_unlocked();

    public:
      /// \brief SLOW!
      void _execute_deferred_tasks_synchronously_single_threaded();
    public:
      spinlock lock;

    private:
      uint32_t get_index(id_t qid);

    private:
      threading::task_manager& tm;

      shared_spinlock queue_id_lock;
      static constexpr uint32_t k_max_queues = 5;
      std::pair<id_t, uint32_t> id_to_index[k_max_queues];
      uint32_t next_index = 0;

      struct submission_list_t
      {
        std::vector<threading::function_t> lists[k_max_queues];
        bool has_any_submissions = false;
      };

      std::vector<submission_list_t> to_submit;
  };
}

