//
// file : task.hpp (2)
// in : file:///home/tim/projects/yaggler/yaggler/bleunw/task/task.hpp
//
// created by : Timothée Feuillet on linux-vnd3.site
// date: 22/10/2015 18:11:22
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

#ifndef __N_10692859991998403593_601204597__TASK_HPP__2___
# define __N_10692859991998403593_601204597__TASK_HPP__2___

#include <functional>

#include "../tools/chrono.hpp"
#include "../tools/backtrace.hpp"

#include "types.hpp"
#include "task_control.hpp"

namespace neam
{
  namespace hydra
  {
    namespace task
    {
      /// \brief a simple task (a function call, repeated and delayed)
      /// It isn't intended to be used by the user directly
      class task
      {
        public:
          /// \brief The task constructor
          /// \note purposedly without \e explicit : it may be easier to simply
          ///       forget the task construct when registering a single function !
          task(const task_func_t &_func) noexcept : ctrl(new task_control)
          {
            ctrl->linked_task = this;
            ctrl->func = _func;

            // We can't reference the task inside the function, as the task will probably be move-contrustructed
            // at another place in the memory prior its execution. We have to rely on ctrl and ctrl->linked_task
            // to perform any operation. Moreover, ctrl.register_task will certainly move the task_control to another task
            // and the only way to access safely the task is to use ctrl.linked_task.
            ctrl->run_func = [](float dt, task_control &ctrl, double now)
            {
              if (ctrl.repeat)
                ctrl.register_task(now);
              try
              {
                ctrl.func(dt, ctrl);
              }
              catch (...)
              {
                ctrl.dismiss();
                throw;
              }
            };
          }

          /// \brief Move the task to another one
          task(task &&t) noexcept : registered_ts(t.registered_ts), ctrl(t.ctrl)
          {
            if (ctrl)
              ctrl->linked_task = this;
            t.ctrl = nullptr;
          }

          task(task_control &t) noexcept : ctrl(&t)
          {
            ctrl->linked_task = this;
          }

          /// \brief Move assignation
          task &operator =(task && t) noexcept
          {
            if (&t != this)
            {
              end();

              registered_ts = t.registered_ts;
              ctrl = t.ctrl;
              ctrl->linked_task = this;
              t.ctrl = nullptr;
            }
            return *this;
          }

          task(const task &t) = delete;
          task &operator = (const task &t) = delete;

          /// \brief destroy the task
          ~task()
          {
            end();
          }

          /// \brief return the task control for the current task
          /// \note nullptr if not registered yet or if dismissed
          task_control &get_task_control()
          {
            return *ctrl;
          }

          /// \brief return the task control for the current task
          /// \note nullptr if not registered yet or if dismissed
          const task_control &get_task_control() const
          {
            return *ctrl;
          }

          /// \brief Comparison operator
          bool operator <(const task &o) const
          {
            float diff = ctrl && o.ctrl ? ((registered_ts + ctrl->delay) - (o.registered_ts + o.ctrl->delay)) : 0.f;
            return !o.ctrl || (ctrl && (diff < 0.f || (diff > -0.001 && diff < 0.001 && ctrl->priority > o.ctrl->priority)));
          }

          double registered_ts = 0.; ///< \brief to be used by the scheduler, it's the timestamp at wich time task has been registered

          /// \brief End the task
          void end()
          {
            if (ctrl && ctrl->linked_task == this)
              ctrl->linked_task = nullptr;
            if (ctrl && ctrl->linked_task == this && ctrl->_do_not_delete == false)
              delete ctrl;
            ctrl = nullptr;
          }

        private:
          task_control *ctrl = nullptr;

          friend class scheduler;
          friend class task_control;
      };
    } // namespace task
  } // namespace hydra
} // namespace neam

#endif /*__N_10692859991998403593_601204597__TASK_HPP__2___*/

// kate: indent-mode cstyle; indent-width 2; replace-tabs on; 

