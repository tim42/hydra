//
// file : types.hpp
// in : file:///home/tim/projects/yaggler/yaggler/bleunw/task/types.hpp
//
// created by : Timothée Feuillet on linux-vnd3.site
// date: 22/10/2015 18:12:40
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

#ifndef __N_9242203241113295496_328999000__TYPES_HPP__
# define __N_9242203241113295496_328999000__TYPES_HPP__

#include <functional>
#include <cstdint>

namespace neam
{
  namespace hydra
  {
    namespace task
    {
      class task_control;

      /// \brief The function type used by tasks
      typedef std::function<void(float, task_control &)> task_func_t;
      /// \brief The function type used by task controls
      typedef std::function<void(float, task_control &, double)> ctrl_func_t;
      /// \brief The function type used by task controls for the then
      typedef std::function<void(task_control &)> then_func_t;

      /// \brief Tells the thread will have no special type
      constexpr uint32_t no_task_type = 0xFFFFFFFF;

      /// \brief Control how a task will be executed
      enum class execution_type
      {
        normal, // The task will be pushed into an array (depending on its affinity)
                // that array will then be sorted (thus the task can be delayed)

        direct, // Direct push into unsorted arrays, faster, but also means the
                // task will run asap. Direct tasks have a higher priority than
                // normal ones.

        low_priority, // The task will run when a thread will have some free time
                      // low priority tasks are pushed into unsorted arrays and don't
                      // have affinity
      };

      /// \brief Control how the lateness indicator is computed
      enum class lateness_mode
      {
        total,          // The lateness won't exceed 1, and a lateness of 1
                        // means at least one task type has been late every frame
                        // (can't be the low_priority tho)
                        // If somehow the lateness is greater than 1, check the critical
                        // hit counter, as that may indicate a serious problem

        per_task_type,  // The lateness can be > 1 as every task_type that is late
                        // Add 1 to the lateness indicator
                        // (a lateness of 2 mean that two task type are late, and
                        // you probably need some rescheduling / more thread for those
                        // task type.
                        // low_priority tasks can't be late
      };

      /// \brief Control how the end_frame affect threads that are executing tasks
      enum class end_frame_mode
      {
        whole_time,     // Consume, if there's enough tasks, all the dedicated time-per-frame
                        // before returning. This will decrease the lateness indicator but in
                        // heavy loads it may affect the framerate a little (depending on the
                        // duration and nature of the tasks you have)
                        // This is the advised mode.

        early_exit,     // Exit the run_some() function as soon as end_frame() is called
                        // This will possibly possibly increase the lateness indicator
                        // but may improve the framerate

        whole_work,     // Only exit when there's no more work to do. Lateness may
                        // be equal to 0, framerate may be negatively impacted.
                        // Only low_priority tasks are skipped if running out of time
                        // If completing the work for the frame takes too much time
                        // (more than 30x the dedicated time per frame) it will
                        // cause issues with the scheduler and you may have to call _reset()
                        // to fix the scheduler and prevent a possible crash/deadlock.
                        // Use this if every single task you have is important and MUST be
                        // executed before ending a frame.
                        // This mode isn't advised.
      };

#ifdef N_REFLECTIVE_PRESENT
      static constexpr neam::r::reason scheduler_lateness_reason = neam::r::reason {"scheduler lateness"};
      static constexpr neam::r::reason scheduler_critical_reason = neam::r::reason {"scheduler critical"};
#endif
    } // namespace task
  } // namespace hydra
} // namespace neam

#endif /*__N_9242203241113295496_328999000__TYPES_HPP__*/

// kate: indent-mode cstyle; indent-width 2; replace-tabs on; 

