//
// created by : Timothée Feuillet
// date: 2022-5-23
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

#include <hydra/engine/engine_module.hpp>
#include <hydra/engine/core_context.hpp>

namespace neam::hydra
{
  class io_module final : public engine_module<io_module>
  {
    public:
      // instead of calling process() (a non-blocking call),
      // call _wait_for_submit_queries which will stall the io task-group
      // until everything submit has been completed (including stuff submit during the call)
      //
      // the default is false (it's a realtime engine after all),
      // but specific tools can require to have it set to true to enforce io completion
      bool wait_for_submit_queries = false;

    private:
      static constexpr const char* module_name = "io";

      // the io module should always be present
      static bool is_compatible_with(runtime_mode /*m*/) { return true; }

      void on_context_initialized() override
      {
        cctx->tm.set_start_task_group_callback("io"_rid, [this]
        {
          // we process io in a separate task so as to dispatch io tasks as early as possible
          // while we process io stuff
          // (if we did do the process in the start callback, the dispatched tasks would only run after process() returned)
          cctx->tm.get_task([this]
          {
            if (wait_for_submit_queries)
              cctx->io._wait_for_submit_queries();
            else
              cctx->io.process();
          });
        });
      }

      friend class engine_t;
      friend engine_module<io_module>;
  };
}

