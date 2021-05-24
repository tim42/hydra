//
// file : task_scheduler.hpp
// in : file:///home/tim/projects/yaggler/yaggler/bleunw/task/task_scheduler.hpp
//
// created by : Timothée Feuillet on linux-vnd3.site
// date: 21/10/2015 17:09:36
//
//
// Copyright (C) 2014 Timothée Feuillet
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#ifndef __N_1565169418532046895_210799306__TASK_SCHEDULER_HPP__
# define __N_1565169418532046895_210799306__TASK_SCHEDULER_HPP__

#include <atomic>
#include <thread>
#include <mutex>
#include <algorithm>
#include <vector>
// #include <bits/inf.h>

#include "../tools/chrono.hpp"
#include "../tools/logger/logger.hpp"
#include "../hydra_exception.hpp"
#include "../hydra_reflective.hpp"

#include "task_control.hpp"
#include "task.hpp"

namespace neam
{
  namespace hydra
  {
    namespace task
    {
      /// \brief This is a quite simple scheduler that aim to run tasks
      /// It don't create any threads but can handle them (just call the run_some() method)
      /// \note Because it sorts the task arrays, It may be preferable to have less tasks that does a bit more.
      ///       But under 4k tasks, it won't really hurt the framerate (if a majority of task are NOT to be executed on the same thread).
      ///       You can also play with the different flags to see if this change something for you, but the more task you have,
      ///       the more your time per frame will be consumed by sorting the arrays.
      /// \todo disable array sorting with a flag.
      /// \note To remove the debug messages you can define the macro HYDRA_SCHEDULER_NO_DEBUG_MESSAGES
      /// \note On ultra light loads, the scheduler may be one frame early for some delayed tasks
      class scheduler
      {
        public:
          scheduler()
          {
            max_affinity_id = std::thread::hardware_concurrency() * 2;
            task_buffers.resize(max_affinity_id + 2);
          }

          ~scheduler()
          {
            end_frame(); // This way, there won't be any remaining thread
          }

          // // flags // //

          /// \brief Set the lateness mode
          /// (default is lateness_mode::total)
          void set_lateness_mode(lateness_mode lm = lateness_mode::total)
          {
            lateness_mode_flag = lm;
          }

          /// \brief Set the end frame mode
          /// (default is end_frame_mode::whole_time)
          void set_end_frame_mode(end_frame_mode efm = end_frame_mode::whole_time)
          {
            end_frame_mode_flag = efm;
          }

          // // settings // //

          /// \brief Sets the maximum duration of the run_some() method
          /// Default is 1/120s
          /// \note If a task is unexpectedly long, the delay could be reached and exceeded
          /// \note Some tasks may have to run at each frame, and they can slow everything / exceed the delay
          /// \note You MUST set a > 0.f maximum_run_duration.
          void set_maximum_run_duration(float duration = 1.f/120.f) { maximum_run_duration = duration; }

          /// \brief Set/change the task type of a given thread
          /// If you plan to use the task type parameter, you have to call
          /// set_current_thread_task_type() or set_thread_task_type()
          /// at least once.
          /// \param task_type The kind of task that will run on that thread
          ///                 Can have the special value neam::hydra::task::no_task_type
          ///                 If the thread don't have a task type (it will then only
          ///                 perform tasks without type)
          void set_current_thread_task_affinity(uint32_t task_type)
          {
            std::thread::id this_id = std::this_thread::get_id();
            set_thread_id_task_affinity(this_id, task_type);
          }

          /// \brief Set/change the task type of a given thread
          /// If you plan to use the task type parameter, you have to call
          /// set_current_thread_task_type() or set_thread_task_type()
          /// at least once.
          /// \param task_type The kind of task that will run on that thread
          ///                 Can have the special value neam::hydra::task::no_task_type
          ///                 If the thread don't have a task type (it will then only
          ///                 perform tasks without type)
          void set_thread_task_affinity(const std::thread &th, uint32_t task_type)
          {
            std::thread::id th_id = th.get_id();
            set_thread_id_task_affinity(th_id, task_type);
          }

          /// \brief Set the speed factor of the scheduler
          /// Default is 1.f
          /// \note The factor could not be negative nor == 0;
          /// \note It is probably a good idea to leave it at 1.f
          void set_speed_factor(float factor = 1.f) { speed_factor = factor > 0.0001 ? factor : speed_factor; }

          // // queries // //

          /// \brief true if the scheduler is late on more than 50% (or the \p lateness if setted) of the frames during the last second
          /// \note This is a good indicator on whether or not spawning a new thread !
          bool is_late(float _lateness = 0.5) const
          {
            return get_lateness() >= _lateness;
          }

          /// \brief Return the current lateness of the task scheduler (rolling average on 1s)
          float get_lateness() const
          {
            return (lateness / (this_second_frame_count + 1.f)) * this_second_advancement
                    + last_second_lateness * (1. - this_second_advancement);
          }

          /// \brief Return the number of time we hit a critical error that have laid to discarding thread safety
          /// If not 0, you could try to exit all the threads one by one, reset (calling _reset() )
          //  and continue the execution ( end_frame() on the main thread, then run_some() at some place)
          size_t get_critical_hit_count() const
          {
            return critical_hit;
          }

          // // Information // //

          /// \brief Return the remaining frame time
          /// \note Should only be used into tasks.
          /// A good usage of this is to have low_priority tasks perform some work
          /// until the frame time assigned to the scheduler is expired. This allow
          /// to perform work in advance when the CPU isn't used by more
          /// important tasks without impacting the framerate
          /// \note a remaining time of < 0 mean you've exceeded the allowed time per frame
          /// \see set_maximum_run_duration()
          /// \see is_end_frame_in_progress()
          double get_remaining_frame_time() const
          {
            return frame_chrono.get_accumulated_time() - maximum_run_duration;
          }

          /// \brief Return whether or not a end_frame() call is waiting for the current
          /// thread to exit the scheduler
          /// \note Should only be used into tasks.
          /// \note As long a get_remaining_frame_time() is > 0, this won't have a negative impact
          /// on the process / framerate / ...
          bool is_end_frame_in_progress() const
          {
            return frame_sync_lock == -1;
          }

          // TODO: average time / immediate time for this frame
          // TODO: average parallelism of the tasks: if 0, no parallelism possible, if 1, parallelism on one thread, 2, on two threads... average on 1s + immediate

          // // methods // //

          /// \brief Push a task to the scheduler
          task_control &push_task(const task_func_t &t, execution_type etype = execution_type::normal, uint32_t task_type = no_task_type)
          {
            check::on_vulkan_error::n_assert(task_type > max_affinity_id, "task_affinity is bigger than the maximum authorized");

            task_control *ctrl = nullptr;

            if (etype == execution_type::normal)
              ctrl = task_buffers[task_type + 2].push_task(task(t));
            else if (etype == execution_type::direct)
              ctrl = task_buffers[task_type + 2].push_unsorted_task(task(t));
            else if (etype == execution_type::low_priority)
              ctrl = task_buffers[0].push_unsorted_task(task(t));
            ctrl->task_execution_type = etype;
            ctrl->task_type = task_type;
            ctrl->task_scheduler = this;
            ctrl->registered = true;
            return *ctrl;
          }

          /// \brief Push a task to the scheduler
          task_control &push_task(task_control &t)
          {
            check::on_vulkan_error::n_assert(t.task_type > max_affinity_id, "task_affinity is bigger than the maximum authorized");

            if (t.task_execution_type == execution_type::normal)
              task_buffers[t.task_type + 2].push_task(task(t));
            else if (t.task_execution_type == execution_type::direct)
              task_buffers[t.task_type + 2].push_unsorted_task(task(t));
            else if (t.task_execution_type == execution_type::low_priority)
              task_buffers[0].push_unsorted_task(task(t));
            t.task_scheduler = this;
            t.registered = true;
            return t;
          }

          /// \brief Create a un-registered task_control (this is wait-free)
          /// \note the _do_not_delete property won't be set to true, every values of the object is kept as if default initialized
          task_control *create_task_control(const task_func_t &t, execution_type etype = execution_type::normal, uint32_t task_type = no_task_type)
          {
            task temp_task(t); // For the allocation + initialization of the task control

            task_control *ctrl = temp_task.ctrl; // Retrieve the task_control + remove it from the task object
            temp_task.ctrl = nullptr;
            ctrl->linked_task = nullptr;

            // Initialize it
            ctrl->task_execution_type = etype;
            ctrl->task_type = task_type;
            ctrl->task_scheduler = this;

            return ctrl;
          }

          /// \brief Clear the task scheduler, also make every thread in run_some() quit the execution as soon as
          ///        the method is called.
          /// \note To start again, you have to call end_frame()
          void clear()
          {
            NHR_MONITOR_THIS_FUNCTION(neam::hydra::task::scheduler::clear);
            // sync threads !
            frame_sync_lock = -1; // HURRY UP ! (also: run_some is forbidden)
            {
              NHR_MEASURE_POINT("wait for threads");
              while (active_thread_count.load(std::memory_order_seq_cst) != 0)
              {
                std::this_thread::yield();

                if (frame_chrono.get_accumulated_time() > maximum_run_duration * 40) // We are very late !!
                {
                  ++lateness;
                  ++critical_hit;
                  NHR_FAIL(scheduler_lateness_reason(N_REASON_INFO, "task scheduler is TOO LATE waiting some thread to terminate"));

                  if (!no_critical_messages_about_wait)
                  {
                    neam::cr::out.critical()  << "task::scheduler::end_frame(): scheduler is late of "
                                              << frame_chrono.get_accumulated_time() - maximum_run_duration << "s waiting for " << active_thread_count
                                              << " threads to terminate." << neam::cr::newline
                                              << "Please report: this is a bug." << neam::cr::newline
                                              << "Going to ignore remaining threads. This error won't be printed again." << std::endl;
                    no_critical_messages_about_wait = true;
                  }
                  break;
                }
              }
            }
            frame_sync_lock = -2;

            // prepare buffers for the clear
            for (size_t i = 0; i < task_buffers.size(); ++i)
              task_buffers[i].clear();

            frame_sync_lock = 1;
          }

          /// \brief Reset the internal state of the scheduler
          /// \warning As this function is intended to be used for recovery in case of a critical hit, it is \b NOT \b THREAD \b SAFE
          ///          You will have exit all secondary thread yourself (or make sure they will not call \b ANY of the scheduler methods in the process)
          /// \note Marked as ADVANCED 'cause this could be DANGEROUS and should be used with CAUTION
          void _reset()
          {
            NHR_MONITOR_THIS_FUNCTION(neam::hydra::task::scheduler::_reset);
            frame_sync_lock = 0; // reset the frame sync/lock
            active_thread_count = 0; // reset the number of active thread count
            frame_chrono.reset();

            lateness = 0.f;
            last_second_lateness = 0.25f;
            this_second_frame_count = 0.f;
            this_second_advancement = 0.f;
            chrono.reset();

            other_task_in_time_this_frame = false;

            critical_hit = 0; // reset the number of critical hits

            // cleanup buffers
            for (size_t i = 0; i < task_buffers.size(); ++i)
            {
              task_buffers[i].op_lock.unlock();
              task_buffers[i].clear();
            }
          }

          /// \brief Mark the end of a frame. If the sync flag is true, it will wait other threads to finish their tasks and swap,
          ///        whereas if the sync flag is false, it will just swap thus possibly accumulating delay in favor of a possibly
          ///        higher framerate for CPU limited progs.
          /// \note You \b HAVE \b TO call this method, but ONLY ONE TIME
          /// \warning you should have just one thread per frame that will call this method. Which thread doesn't particularly matters,
          ///          the thread may not be the main thread (it could be, but it better not) as the thread that end the frame will have
          ///          to perform the sort() on every task_type
          /// \todo Automatic call of end_frame()
          /// \see wait_for_frame_end()
          void end_frame()
          {
            NHR_MONITOR_THIS_FUNCTION(neam::hydra::task::scheduler::end_frame);

            if (frame_sync_lock < 0) // wait... just one thread we've said !!
            {
              NHR_FAIL(scheduler_critical_reason(N_REASON_INFO, "multiple call of end_frame detected"));
              neam::cr::out.critical() << "task::scheduler::end_frame(): more than one thread is trying to call end_frame" << cr::newline
                                                      << "> this will increment the critical hit counter as this may be a critical fault" << std::endl;
              ++critical_hit; // this may endanger the safety of that class
              return;
            }

            // sync threads !
            frame_sync_lock = -1; // HURRY UP ! (also: run_some() is forbidden)
            {
              NHR_MEASURE_POINT("wait for threads");
              while (active_thread_count.load(std::memory_order_seq_cst) != 0)
              {
                std::this_thread::yield();

                if (frame_chrono.get_accumulated_time() > maximum_run_duration * 40) // We are very late !!
                {
                  ++lateness;
                  ++critical_hit;
                  if (!no_critical_messages_about_wait)
                  {
                    NHR_FAIL(scheduler_lateness_reason(N_REASON_INFO, "task scheduler is TOO LATE waiting some thread to terminate"));
                    neam::cr::out.critical()  << "task::scheduler::end_frame(): scheduler is late of "
                                              << frame_chrono.get_accumulated_time() - maximum_run_duration << "s waiting for " << active_thread_count
                                              << " threads to terminate." << neam::cr::newline
                                              << "Please report: this is a bug." << neam::cr::newline
                                              << "Going to ignore remaining threads. This error won't be printed again." << std::endl;
                    no_critical_messages_about_wait = true;
                  }
                  break;
                }
              }
            }
            frame_sync_lock = -2;

            // Update the lateness (we begin at 1 to skip low_priority tasks)
            if (lateness_mode_flag == lateness_mode::per_task_type)
            {
              for (size_t i = 1; i < task_buffers.size(); ++i)
                lateness += task_buffers[i].work_done ? 1 : 0;
            }
            else if (lateness_mode_flag == lateness_mode::total)
            {
              bool is_late = false;
              for (size_t i = 1; i < task_buffers.size(); ++i)
                is_late |= !task_buffers[i].work_done;
              lateness += is_late ? 1 : 0;
            }

            const float acc_time = chrono.get_accumulated_time();
            if (acc_time >= (1.f - 1.f/150.f)) // end that second
            {
              last_second_lateness = lateness / (this_second_frame_count + 1);
              chrono.reset(); // reset the chrono
              lateness = 0.f;
              this_second_advancement = 0.f;
              this_second_frame_count = 0.f;
#ifndef HYDRA_NO_MESSAGES
#ifndef HYDRA_SCHEDULER_NO_DEBUG_MESSAGES
              // We print a nice debug message on whether or not we should spawn some more threads.
              if (last_second_lateness > 0.95)
                neam::cr::out.debug() << "WARNING: task::scheduler: Lateness factor for the last second: " << last_second_lateness << std::endl;
#endif /*HYDRA_SCHEDULER_NO_DEBUG_MESSAGES*/
#endif /*HYDRA_NO_MESSAGES*/
            }
            else // continue that second
            {
              this_second_advancement = acc_time;
              this_second_frame_count += 1.f;
            }

            // prepare buffers for the swap
            {
              NHR_MEASURE_POINT("task sorting");

              for (size_t i = 0; i < task_buffers.size(); ++i)
                task_buffers[i].end_frame();
            }

            frame_sync_lock = 1; // OK TO GO
          }

          /// \brief Make the thread sleep until the end of the frame, where you can call again run_some() safely
          /// An example on ow to use it:
          /// \code
          ///   { // scope of the thread
          ///     // [...]
          ///     while (is_app_working)
          ///     {
          ///       // [...]
          ///       scheduler->wait_for_frame_end();
          ///       scheduler->run_some();
          ///       // [...]
          ///     }
          ///   }
          /// \endcode
          /// \note As this method is monitored, if you lookup its duration time, you'll see if you waste
          ///       any CPU time waiting for a frame end
          bool wait_for_frame_end()
          {
            NHR_MONITOR_THIS_FUNCTION(neam::hydra::task::scheduler::wait_for_frame_end);

            // wait for a favorable sync-lock
            while (frame_sync_lock.load(std::memory_order_acquire) < 0)
              std::this_thread::yield();
            return true;
          }

          /// \brief Same as wait_for_frame_end(), but returns immediately
          bool is_frame_end()
          {
            // wait for a favorable sync-lock
            if (frame_sync_lock.load(std::memory_order_acquire) < 0)
              return false;
            return true;
          }

          /// \brief Run some of the tasks.
          /// Maximum duration is controlled by set_maximum_run_duration()
          /// \see set_maximum_run_duration()
          /// \param run_for Set a custom run duration (mostly for being used in additional threads).
          ///                Please note that a frame end will trigger the return of this method disregarding the remaining number of task
          ///                or the remaining time
          void run_some(float run_for = -1.f)
          {
            NHR_MONITOR_THIS_FUNCTION(neam::hydra::task::scheduler::run_some);
            float time_limit = std::min(run_for <= 0.0001f ? maximum_run_duration : run_for, maximum_run_duration);
            time_limit = time_limit > 0.0001f ? time_limit : 1.f; // Make the hardcoded maximum to 1s of run_time (this is HUGE for a realtime app).
            time_limit -= 0.0001f;

            try // We HAVE to decrement thread_count !
            {
              ++active_thread_count;

              int one = 1;
              if (frame_sync_lock.compare_exchange_strong(one, 0))
              {
                // We are the first thread to enter the method, so this is officialy the frame start
                frame_chrono.reset(); // reset the chrono (we are in a thread safe context !)
                frame_sync_lock = 0;
              }
              else while (frame_sync_lock.load(std::memory_order_acquire) != 0)
                  std::this_thread::yield();

              double inital_time = frame_chrono.now() - frame_chrono.get_accumulated_time();
              double end_time = inital_time + maximum_run_duration;

              // get the task_type index
              size_t task_type_index = get_thread_affinity(std::this_thread::get_id()) + 2;
              task_list_buffer *current = task_buffers[task_type_index].current;

              // the task_buffers_indexes array contain indexes of task_buffer to work on
              size_t task_buffers_indexes[] = {task_type_index, 1, 0};
              size_t current_task_buffers_index = 0;
              const size_t task_buffers_indexes_sz = sizeof(task_buffers_indexes) / sizeof(task_buffers_indexes[0]);

              bool skip_direct_tasks = task_buffers[task_type_index].work_done;

              while (true)
              {
                // exit conditions
                // frame_chrono.get_accumulated_time() wouldn't cause any thread-safety problem as the method is const.
                if (end_frame_mode_flag == end_frame_mode::whole_time && (frame_chrono.get_accumulated_time() >= time_limit))
                  break;
                else if (end_frame_mode_flag == end_frame_mode::early_exit && ((frame_chrono.get_accumulated_time() >= time_limit) || frame_sync_lock < 0))
                  break;
                else if (end_frame_mode_flag == end_frame_mode::whole_work && ((frame_chrono.get_accumulated_time() >= time_limit) && (task_type_index == 0)))
                  break; // only exit when the whole time is consumed and we're at the low_priority tasks

                // direct (unsorted tasks)
                if (!skip_direct_tasks)
                {
                  const size_t grab_task = current->unsorted_task_list_index++;
                  if (grab_task < current->unsorted_task_list.size())
                  {
                    run_task(&current->unsorted_task_list[grab_task], end_time);
                    continue;
                  }

                  --current->unsorted_task_list_index; // restore the old index
                  skip_direct_tasks = true;
                }

                // normal (sorted tasks)
                {
                  const size_t grab_task = current->task_list_index++;
                  if (grab_task < current->task_list.size())
                  {
                    task *task_ptr = &current->task_list[grab_task];
                    if (task_ptr->registered_ts + task_ptr->get_task_control().delay < end_time)
                    {
                      run_task(task_ptr, end_time);
                      continue;
                    }
                  }

                  --current->task_list_index; // restore the old index
                  task_buffers[task_type_index].work_done = true; // we've done the work for that frame
                }

                // done all the work in that category, try something else
                ++current_task_buffers_index;
                if (current_task_buffers_index < task_buffers_indexes_sz)
                {
                  task_type_index = task_buffers_indexes[current_task_buffers_index];
                  current = task_buffers[task_type_index].current;
                  skip_direct_tasks = task_buffers[task_type_index].work_done;
                  continue;
                }

                // done, no more work to do
                break;
              }
            }
            // Don't allow to die without decrementing the thread_count...
            catch (std::exception &e)
            {
              NHR_FAIL(neam::r::exception_reason(N_REASON_INFO, "task::scheduler::run_some(): caught exception"));
              neam::cr::out.error() << "task::scheduler::run_some(): caught exception: " << e.what() << std::endl;
            }
            catch (...)
            {
              NHR_FAIL(neam::r::exception_reason(N_REASON_INFO, "task::scheduler::run_some(): caught exception"));
              neam::cr::out.error() << "task::scheduler::run_some(): caught unknown exception" << std::endl;
            }

            if (active_thread_count == 1)
              frame_sync_lock = 2; // END.
            --active_thread_count;
          }

        private:
          void set_thread_id_task_affinity(std::thread::id id, size_t task_type)
          {
            check::on_vulkan_error::n_assert(task_type > max_affinity_id, "task_affinity is bigger than the maximum authorized");

            std::lock_guard<neam::spinlock> _u0(affinity_lock);
            thread_to_affinity[id] = task_type;
          }

          inline size_t get_thread_affinity(std::thread::id id)
          {
            std::lock_guard<neam::spinlock> _u0(affinity_lock);
            auto it = thread_to_affinity.find(id);
            if (it == thread_to_affinity.end())
              return no_task_type;
            return it->second;
          }

          void run_task(task *task_ptr, double end_time)
          {
            if (task_ptr->get_task_control().dismissed)
            {
              // We got a dismissed task...
              if (task_ptr->get_task_control().then)
                task_ptr->get_task_control().then(task_ptr->get_task_control()); // Call the then() method
              task_ptr->get_task_control().registered = false;
              task_ptr->end();
              task_ptr = nullptr;
            }
            else
            {
              // Run the task...
              const double now = frame_chrono.now();

              float delta = task_ptr->get_task_control().delay;
              double fnow = now;
              if (delta <= 0.f || end_time < now) // We tend to run in advance whenever it's possible, but in that case we may be late !
              {
                delta = now - task_ptr->registered_ts;
                fnow = task_ptr->registered_ts + delta;
              }
              try
              {
                task_ptr->get_task_control().registered = false;
                task_ptr->get_task_control().run_func(delta * speed_factor, task_ptr->get_task_control(), fnow); // Here we go ! we run that func !
                if (!task_ptr->get_task_control().registered && task_ptr->get_task_control().then)
                  task_ptr->get_task_control().then(task_ptr->get_task_control());
                task_ptr->end();
              }
              catch (std::exception &e)
              {
                NHR_IND_FAIL(neam::r::exception_reason(N_REASON_INFO, "task::scheduler::run_some(): caught exception"));
                neam::cr::out.error() << "task::scheduler::run_some(): caught exception: " << e.what() << std::endl;
              }
              catch (...)
              {
                NHR_IND_FAIL(neam::r::exception_reason(N_REASON_INFO, "task::scheduler::run_some(): caught exception"));
                neam::cr::out.error() << "task::scheduler::run_some(): caught unknown exception" << std::endl;
              }
            }
          }

        private:
          // flags

          lateness_mode lateness_mode_flag = lateness_mode::total;
          end_frame_mode end_frame_mode_flag = end_frame_mode::whole_time;

          bool no_critical_messages_about_wait = false;

          // settings
          float maximum_run_duration = 1.f / 120.f;
          float speed_factor = 1.f;

          size_t max_affinity_id; // used to define affinity / user-defined thread ids

          // infos
          float lateness = 0.f;
          float this_second_frame_count = 0.f;
          float this_second_advancement = 0.f;
          float last_second_lateness = 0.25f;

          size_t critical_hit = 0;

          std::atomic<bool> other_task_in_time_this_frame = ATOMIC_VAR_INIT(false);

          std::atomic<int> frame_sync_lock = ATOMIC_VAR_INIT(0);

          // affinity management

          neam::spinlock affinity_lock; // TODO: find something better...
          std::map<std::thread::id, size_t> thread_to_affinity;

          // task list, per task "affinity"
          struct task_list_buffer
          {
            std::deque<neam::hydra::task::task> task_list;
            std::atomic<int> task_list_index = ATOMIC_VAR_INIT(0);
            std::deque<neam::hydra::task::task> unsorted_task_list;
            std::atomic<int> unsorted_task_list_index = ATOMIC_VAR_INIT(0);

            // swap + consolidate the buffers
            void swap(task_list_buffer &o)
            {
              if (!task_list.size())
              {
                // fast path: avoid a sort and a O(n) process
                task_list.swap(o.task_list);
                task_list_index = o.task_list_index.load(); // not atomic, but who cares ? there's a mutex above us
              }
              else
              {
                // task_list.reserve(o.task_list.size() - o.task_list_index);
                for (size_t i = o.task_list_index; i < o.task_list.size(); ++i)
                  task_list.emplace_back(std::move(o.task_list[i]));
                std::sort(task_list.begin(), task_list.end());
              }

              // unsorted_task_list.reserve(o.unsorted_task_list.size() - o.unsorted_task_list_index);
              for (size_t i = o.unsorted_task_list_index; i < o.unsorted_task_list.size(); ++i)
                unsorted_task_list.emplace_back(std::move(o.unsorted_task_list[i]));
            }

            // clear everything
            void clear()
            {
              task_list.clear();
              unsorted_task_list.clear();
              unsorted_task_list_index = 0;
              task_list_index = 0;
            }
          };

          // brief handle the end frame thing and the buffer swap
          struct task_type
          {
            std::atomic_bool work_done = ATOMIC_VAR_INIT(false);
            task_list_buffer buffers[2];
            task_list_buffer *current = buffers + 1;

            neam::spinlock op_lock;
            int index = 1;

            // swap from the two lists
            // NOTE: should only be called once per frame
            void end_frame()
            {
              std::lock_guard<neam::spinlock> _u0(op_lock);
              int next_index = (index + 1 % 2);
              buffers[next_index].swap(buffers[index]);
              current = buffers + next_index;
              buffers[index].clear();
              index = next_index;
              work_done = false;
            }

            // clear the buffers
            void clear()
            {
              std::lock_guard<neam::spinlock> _u0(op_lock);
              int next_index = (index + 1 % 2);
              buffers[next_index].clear();
              buffers[index].clear();
              current = buffers + next_index;
              index = next_index;
              work_done = false;
            }

            task_control *push_task(neam::hydra::task::task &&t)
            {
              std::lock_guard<neam::spinlock> _u0(op_lock);
              buffers[index + 1 % 2].task_list.emplace_back(std::move(t));
              return &buffers[index + 1 % 2].task_list.back().get_task_control();
            }

            task_control *push_unsorted_task(neam::hydra::task::task &&t)
            {
              std::lock_guard<neam::spinlock> _u0(op_lock);
              buffers[index + 1 % 2].unsorted_task_list.emplace_back(std::move(t));
              return &buffers[index + 1 % 2].unsorted_task_list.back().get_task_control();
            }
          };

          // the buffers, per task type
          // 0 is for the low priority tasks,
          // 1 is for the tasks without a particular type
          // [2..] are for tasks with a given type
          //
          // So the index is tast type + 2
          std::deque<task_type> task_buffers;
          std::atomic<int> active_thread_count;

          // conf
          neam::cr::chrono chrono; // the main chronometer of the scheduler
          neam::cr::chrono frame_chrono; // the per-frame chronometer of the scheduler
      };
    } // namespace task
  } // namespace hdra
} // namespace neam

#define TASK_SCHEDULER_DEFINED
#include "task_control.hpp"
#undef TASK_SCHEDULER_DEFINED

#endif /*__N_1565169418532046895_210799306__TASK_SCHEDULER_HPP__*/

// kate: indent-mode cstyle; indent-width 2; replace-tabs on; 

