//
// created by : Timothée Feuillet
// date: 2022-5-20
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

#include "pass_manager.hpp"

#include <hydra/engine/hydra_context.hpp>
#include <hydra/utilities/transfer_context.hpp>

#include <ntools/tracy.hpp>

namespace neam::hydra
{
  render_pass::render_pass(hydra_context& _context)
    : context(_context)
  {}

  render_pass::~render_pass()
  {
  }

  pass_manager::pass_manager(hydra_context& _context)
    : render_pass(_context)
    // , gfx_transfer_finished { context.device }
    // , cmp_transfer_finished { context.device }
  {}
  pass_manager::~pass_manager()
  {
  }

  void pass_manager::setup(render_pass_context& rpctx)
  {
    TRACY_SCOPED_ZONE;
    setup(rpctx, false);
  }

  void pass_manager::setup(render_pass_context& rpctx, bool force)
  {
    TRACY_SCOPED_ZONE;
    for (auto& it : passes)
    {
      if (it->need_setup || force)
      {
        it->setup(rpctx);
        it->need_setup = false;
      }
    }
  }

  void pass_manager::prepare(render_pass_context& rpctx)
  {
    TRACY_SCOPED_ZONE;
    allocator::scope gpu_alloc_scope = context.allocator.push_scope();
    for (auto& it : passes)
    {
      const allocator::scope pass_gpu_alloc_scope = gpu_alloc_scope.push_scope();
      it->prepare(rpctx);
    }
  }

  render_pass_output pass_manager::submit(render_pass_context& rpctx)
  {
    TRACY_SCOPED_ZONE;
    constexpr uint32_t k_pass_per_task = 1; // FIXME: Should be around 2
    std::vector<render_pass_output> rpo;
    rpo.resize(passes.size());
    {
      TRACY_SCOPED_ZONE;
      threading::for_each(context.tm, context.tm.get_current_group(), passes, [this, &rpctx, &rpo](auto & rp, size_t idx)
      {
        TRACY_SCOPED_ZONE;
        rpo[idx] = rp->submit(rpctx);
      }, k_pass_per_task);
    }

    render_pass_output out;
    for (auto& it : rpo)
    {
      out.insert_back(std::move(it));
    }
    return out;
  }

  void pass_manager::cleanup(render_pass_context& rpctx)
  {
    TRACY_SCOPED_ZONE;
    for (auto& it : passes)
    {
      it->cleanup(rpctx);
    }
  }

  void pass_manager::render(neam::hydra::vk::submit_info& si, render_pass_context& rpctx)
  {
    TRACY_SCOPED_ZONE;
    auto transfer_build_completion_marker = context.tm.get_task([this, &si, &rpctx]
    {
      rpctx.transfers.build(si);
    }).create_completion_marker();

    constexpr uint32_t k_pass_per_task = 1; // FIXME: Should be around 2
    std::vector<render_pass_output> rpo;
    rpo.resize(passes.size());


    threading::for_each(context.tm, context.tm.get_current_group(), passes, [this, &rpctx, &rpo](auto& rp, size_t idx)
    {
      TRACY_SCOPED_ZONE;
      rpo[idx] = rp->submit(rpctx);
    }, k_pass_per_task);

    context.tm.actively_wait_for(std::move(transfer_build_completion_marker), threading::task_selection_mode::only_current_task_group);

//     for (uint32_t i = 0; i < (uint32_t)passes.size(); ++i)
//     {
//       rpo[i] = passes[i]->submit(rpctx);
//     }


#if 0
    if (has_transfers)
    {
      // FIXME (2023-08):
      // {
      //   {
      //     vk::command_buffer cmd_buf = graphic_transient_cmd_pool.create_command_buffer();
      //     neam::hydra::vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
      //
      //     rpctx.transfers.acquire_resources(cbr, context.gqueue.get_queue_familly_index());
      //     cmd_buf.end_recording();
      //     si.on(context.gqueue).execute(cmd_buf);
      //     rpctx.vrd.postpone_destruction_to_next_fence(context.gqueue, std::move(cmd_buf));
      //   }
      //   if (has_any_compute)
      //   {
      //     vk::command_buffer cmd_buf = compute_transient_cmd_pool.create_command_buffer();
      //     neam::hydra::vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
      //
      //     rpctx.transfers.acquire_resources(cbr, context.cqueue.get_queue_familly_index());
      //     cmd_buf.end_recording();
      //     si.on(context.cqueue).execute(cmd_buf);
      //     rpctx.vrd.postpone_destruction_to_next_fence(context.cqueue, std::move(cmd_buf));
      //   }
      // }

      // perform the transfers:

      si.on(context.tqueue).signal(gfx_transfer_finished);
      // if (has_any_compute)
        // si.on(context.tqueue).signal(cmp_transfer_finished);
    // const bool has_transfers = has_any_compute
    //    ? rpctx.transfers.transfer(context.allocator, { &gfx_start, &cmp_start }, { &gfx_transfer_finished, &cmp_transfer_finished })
    //    : rpctx.transfers.transfer(context.allocator, { &gfx_start }, { &gfx_transfer_finished })
    //  ;

      // wait for the transfers on both queues:
      if (has_transfers)
      {
        // FIXME (2023-08):
        // gsi << neam::hydra::vk::cmd_sema_pair {gfx_transfer_finished, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
        // if (has_any_compute)
        //   csi << neam::hydra::vk::cmd_sema_pair {cmp_transfer_finished, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
        // {
        //   {
        //     vk::command_buffer cmd_buf = graphic_transient_cmd_pool.create_command_buffer();
        //     neam::hydra::vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        //
        //     rpctx.transfers.release_resources(cbr, context.gqueue.get_queue_familly_index());
        //     cmd_buf.end_recording();
        //     gsi << cmd_buf;
        //     rpctx.vrd.postpone_destruction_to_next_fence(context.gqueue, std::move(cmd_buf));
        //   }
        //   if (has_any_compute)
        //   {
        //     vk::command_buffer cmd_buf = compute_transient_cmd_pool.create_command_buffer();
        //     neam::hydra::vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        //
        //     rpctx.transfers.release_resources(cbr, context.cqueue.get_queue_familly_index());
        //     cmd_buf.end_recording();
        //     csi << cmd_buf;
        //     rpctx.vrd.postpone_destruction_to_next_fence(context.cqueue, std::move(cmd_buf));
        //   }
        //
        // }
      }

      rpctx.transfers.clear_resources_to_release();
    }
    // else
    // {
      // to_transfer = 0;
    // }
#endif
    // submit the command buffers:
    {
      TRACY_SCOPED_ZONE_COLOR(0xFF0000);
      for (auto& it : rpo)
      {
        si.on(context.gqueue);
        for (auto& b : it.graphic)
          si.execute(b);
        context.dfe.defer_destruction(context.dfe.queue_mask(context.gqueue), std::move(it.graphic));

        si.on(context.cqueue);
        for (auto& b : it.compute)
          si.execute(b);
        context.dfe.defer_destruction(context.dfe.queue_mask(context.cqueue), std::move(it.compute));
      }
    }
  }
}

