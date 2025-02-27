//
// created by : Timothée Feuillet
// date: 2/13/25
//
//
// Copyright (c) 2025 Timothée Feuillet
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

#include "gpu_tasks_order.hpp"
#include "../../vulkan/submit_info.hpp"
#include "../../ecs/hierarchy.hpp"
#include "../../engine/hydra_context.hpp"

namespace neam::hydra::renderer::internals
{
  void gpu_task_order::prepare_and_dispatch_gpu_tasks(hydra_context& hctx)
  {
    hctx.tm.get_task([this, &hctx]
    {
      TRACY_SCOPED_ZONE;
      const auto* hc = get_unsafe<ecs::components::hierarchy>();
      if (!check::debug::n_check(hc != nullptr, "gpu_task_order::prepare_and_dispatch_gpu_tasks: no hierarchy on the entity"))
        return;
      if (!check::debug::n_check(hc->is_universe_root(), "gpu_task_order::prepare_and_dispatch_gpu_tasks: component is not attached on the universe root"))
        return;

      {
        // push allocation scope (the root scope, most likely)
        allocator::scope gpu_alloc_scope = hctx.allocator.push_scope();

        // perform single-threaded hierarchical update
        hc->get_universe()->hierarchical_update_single_thread();
      }
      // once the update is done, wait for all launched task to be completed and submit the data to the gpu
      prepare_submissions(hctx);
    });
  }

  void gpu_task_order::prepare_submissions(hydra_context& hctx)
  {
    if (pass_data.empty())
      return;

    vvsi.resize(pass_data.size());

#if N_ENABLE_MT_CHECK
    // force a non-scoped write section on both pass_data and vvsi.
    // we assume that no-one will write or destruct this component, so we enforce that assumption here
    // this will catch almost all incorrect behaviors
   // pass_data._get_mt_instance_checker().enter_write_section();
   // vvsi._get_mt_instance_checker().enter_write_section();
#endif

    remaining_tasks.store(pass_data.size(), std::memory_order_release);


    // we don't use async's multi-chain, as we have a way to guarantee that the context data will not get destructed
    // we also don't need to support task cancellation

    const auto on_completion = [this, &hctx](uint32_t idx, std::vector<vk::submit_info>&& vsi)
    {
      // one thread per entry
      vvsi[idx] = std::move(vsi);

      const uint32_t rem = remaining_tasks.fetch_sub(1, std::memory_order_acq_rel) - 1;
      if (rem == 0)
      {
        // we don't dispatch a task, as we should be called from a task.
        // this means tho that one of the task will have a higher than normal cost, but this prevents a dispatch
        TRACY_SCOPED_ZONE;

        {
          std::lock_guard _lg(hctx.dqe.lock);
          for (auto& vit : vvsi)
          {
            for (auto& it : vit)
            {
              it.deferred_submit_unlocked();
            }
          }
        }
#if N_ENABLE_MT_CHECK
        // leave the write section
    //    pass_data._get_mt_instance_checker().leave_write_section();
     //   vvsi._get_mt_instance_checker().leave_write_section();
#endif

        // free the memory
        pass_data.clear();
        vvsi.clear();
      }
    };

    // call on_completion on the chain completion
    for (uint32_t i = 0; i < pass_data.size(); ++i)
    {
      pass_data[i].then([this, i, on_completion](std::vector<vk::submit_info>&& vsi) { on_completion(i, std::move(vsi)); });
    }
  }
}
