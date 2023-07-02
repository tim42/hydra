//
// created by : Timothée Feuillet
// date: 2023-1-21
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

#include <hydra/engine/engine.hpp>
#include <hydra/engine/engine_module.hpp>
#include <hydra/engine/core_context.hpp>
#include <hydra/engine/hydra_context.hpp>

#include <ntools/chrono.hpp>

namespace neam::hydra
{
  /// \brief Handle core functionalities for hydra
  class core_module final : public engine_module<core_module>
  {
    public:
      // Frame throttling. This provide an effective mechanism to control frame length and save cpu power.
      std::chrono::microseconds min_frame_length {0};

      // If difference is less than this, do nothing
      std::chrono::microseconds min_delta_time_to_sleep {50};

    private:
      static constexpr const char* module_name = "core";

      // the core module should always be present
      static bool is_compatible_with(runtime_mode /*m*/) { return true; }

      void add_task_groups(threading::task_group_dependency_tree& tgd) override;
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override;

      void on_context_initialized() override;
      void on_engine_boot_complete() override;
      void on_start_shutdown() override;

    private: // index watcher/auto-reload stuff:
      std::filesystem::file_time_type last_index_timestamp;
      cr::chrono index_watcher_chrono; // throttle index watch

      std::chrono::time_point<std::chrono::high_resolution_clock> last_frame_timepoint;

      void watch_for_index_change();
      void throttle_frame();

    private:


      friend class engine_t;
      friend engine_module<core_module>;
  };
}

