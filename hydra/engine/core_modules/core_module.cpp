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

#include <hydra/engine/engine.hpp>
#include <hydra/engine/engine_module.hpp>
#include <hydra/engine/core_context.hpp>
#include <hydra/engine/hydra_context.hpp>

#include <ntools/chrono.hpp>

namespace neam::hydra
{
  class core_module final : public engine_module<core_module>
  {
    private:
      static constexpr const char* module_name = "core";

      // the core module should always be present
      static bool is_compatible_with(runtime_mode /*m*/) { return true; }

      void add_task_groups(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_task_group("init"_rid, "init");
      }
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_dependency("io"_rid, "init"_rid);
      }

      void on_context_initialized() override
      {
        // do some hctx init:
        if (hctx)
        {
          hctx->ppmgr.register_shader_reload_event(*hctx);
        }
      }

      void on_engine_boot_complete() override
      {
        last_index_timestamp = cctx->res.get_index_modified_time();
        index_watcher_chrono.reset();


        const bool is_release_engine = (engine->get_runtime_mode() & runtime_mode::release) != runtime_mode::none;

        hctx->tm.set_start_task_group_callback("init"_rid, [this, is_release_engine]()
        {
          if (!is_release_engine)
          {
            // spawn the index watcher task
            hctx->tm.get_task([this]() { watch_for_index_change(); });
          }
        });
      }

    private: // index watcher/auto-reload stuff:
      std::filesystem::file_time_type last_index_timestamp;
      cr::chrono index_watcher_chrono; // throttle index watch

      void watch_for_index_change()
      {
        // rate-limit the function:
        if (index_watcher_chrono.get_accumulated_time() < 0.5)
          return;
        index_watcher_chrono.reset();

        const std::filesystem::file_time_type index_mtime = cctx->res.get_index_modified_time();
        if (index_mtime > last_index_timestamp)
        {
          neam::cr::out().debug("core_module: index change detected, reloading index");
          last_index_timestamp = index_mtime;
          cctx->res.reload_index();
        }
      }

    private:
      friend class engine_t;
      friend engine_module<core_module>;
  };
}

