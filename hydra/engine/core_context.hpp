//
// created by : Timothée Feuillet
// date: 2022-4-26
//
//
// Copyright (c) 2022 Timothée Feuillet
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

#include <ntools/io/context.hpp>
#include <ntools/threading/threading.hpp>
#include <ntools/sys_utils.hpp>

#include <resources/context.hpp>
#include <engine/conf/context.hpp>

namespace neam::hydra
{
  struct index_boot_parameters_t
  {
    enum
    {
      load_index_file,
      init_empty_index,
      init_from_data,
    } mode = load_index_file;

    id_t index_key;
    const std::string index_file;
    size_t index_size;
    const void* index_data;

    std::string argv0 {};

    template<typename T, uint32_t Count>
    static index_boot_parameters_t from(id_t index_key, const T (&ar)[Count])
    {
      return {.mode = init_from_data, .index_key = index_key, .index_size = Count * sizeof(T), .index_data = ar};
    }
  };

  /// \brief Hold the core context. (all which is related to resources, threading, io, memory, ...)
  class core_context
  {
    public:
      threading::task_manager tm;
      io::context io;
      resources::context res = { io, *this };
      conf::context hconf = { *this };

      std::string program_name {};

      class engine_t* engine = nullptr;

    public:
      ~core_context();

      resources::context::status_chain boot(threading::resolved_graph&& task_graph, threading::resolved_threads_configuration&& rtc, index_boot_parameters_t&& ibp,
                                            bool auto_unlock_tm = true, uint32_t thread_count = std::thread::hardware_concurrency() - 2);

      void enroll_main_thread();

      async::continuation_chain stop_app();

      /// \brief Return the number of threads manager by the core context
      uint32_t get_thread_count() const { return (uint32_t)threads.size(); }

      bool is_stopped() const { return halted && !never_started; }
      bool is_booted() const { return !never_started; }

      /// \brief Stall every threads except the \e count first threads. (2 is probably the safest min number of threads to not stall)
      void stall_all_threads_except(uint32_t count) { threads_to_not_stall = count; }

      /// \brief Undo the effects of stall_all_threads_except. Might take some time to take effect
      void unstall_all_threads() { threads_to_not_stall = ~0u; }

      void _exit_all_threads();
    private:
      static void thread_main(core_context& ctx, threading::named_thread_t thread, uint32_t index = ~0u);

    private:
      std::vector<std::thread> threads;
      std::atomic<uint32_t> thread_index;
      spinlock destruction_lock;
      bool should_stop = false;
      bool can_return = false;
      bool halted = false;
      bool never_started = true;
      bool booted = false;


      uint32_t threads_to_not_stall = ~0u;
      uint32_t ms_to_stall = 500;
    };
}

