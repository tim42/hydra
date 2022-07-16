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

namespace neam::hydra
{
  render_pass::render_pass(hydra_context& _context)
    : context(_context)
    , graphic_transient_cmd_pool{context.gqueue.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)}
    , compute_transient_cmd_pool{context.cqueue.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)}
  {}

  render_pass::~render_pass()
  {
    context.vrd.postpone_destruction_to_next_fence(context.gqueue, std::move(graphic_transient_cmd_pool));
    context.vrd.postpone_destruction_to_next_fence(context.cqueue, std::move(compute_transient_cmd_pool));
  }

  pass_manager::pass_manager(hydra_context& _context)
    : render_pass(_context)
    , gfx_transfer_finished { context.device }
    , cmp_transfer_finished { context.device }
  {}
  pass_manager::~pass_manager()
  {
//     context.vrd.postpone_destruction_to_next_fence(context.gqueue, std::move(passes), std::move(gfx_transfer_finished));
//     context.vrd.postpone_destruction_to_next_fence(context.cqueue, std::move(cmp_transfer_finished));
  }

  void pass_manager::setup(render_pass_context& rpctx)
  {
    setup(rpctx, false);
  }

  void pass_manager::setup(render_pass_context& rpctx, bool force)
  {
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
    for (auto& it : passes)
    {
      it->prepare(rpctx);
    }
  }

  render_pass_output pass_manager::submit(render_pass_context& rpctx)
  {
    constexpr uint32_t k_pass_per_task = 1; // FIXME: Should be around 2
    std::vector<render_pass_output> rpo;
    rpo.resize(passes.size());
    threading::for_each(context.tm, context.tm.get_current_group(), passes, [this, &rpctx, &rpo](auto& rp, size_t idx)
    {
      rpo[idx] = rp->submit(rpctx);
    }, k_pass_per_task);

//     for (uint32_t i = 0; i < (uint32_t)passes.size(); ++i)
//     {
//       rpo[i] = passes[i]->submit(rpctx);
//     }

    render_pass_output out;
    for (auto& it : rpo)
    {
      out.insert_back(std::move(it));
    }
    return out;
  }

  void pass_manager::cleanup()
  {
    for (auto& it : passes)
    {
      it->cleanup();
    }
  }

  void pass_manager::render(neam::hydra::vk::submit_info& gsi, neam::hydra::vk::submit_info& csi, render_pass_context& rpctx, bool& has_any_compute)
  {
    constexpr uint32_t k_pass_per_task = 1; // FIXME: Should be around 2
    std::vector<render_pass_output> rpo;
    rpo.resize(passes.size());
    threading::for_each(context.tm, context.tm.get_current_group(), passes, [this, &rpctx, &rpo](auto& rp, size_t idx)
    {
      rpo[idx] = rp->submit(rpctx);
    }, k_pass_per_task);
//     for (uint32_t i = 0; i < (uint32_t)passes.size(); ++i)
//     {
//       rpo[i] = passes[i]->submit(rpctx);
//     }
    has_any_compute = false;
    for (auto& it : rpo)
    {
      if (!it.compute.empty())
      {
        has_any_compute = true;
        break;
      }
    }
    // perform the transfers:
    to_transfer = context.transfers.get_total_size_to_transfer();
    const bool has_transfers = has_any_compute
       ? context.transfers.transfer(context.allocator, { &gfx_transfer_finished, &cmp_transfer_finished })
       : context.transfers.transfer(context.allocator, { &gfx_transfer_finished })
     ;

    // wait for the transfers on both queues:
    if (has_transfers)
    {
      gsi << neam::hydra::vk::cmd_sema_pair {gfx_transfer_finished, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
      if (has_any_compute)
        csi << neam::hydra::vk::cmd_sema_pair {cmp_transfer_finished, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    }

    // submit the command buffers:
    for (auto& it : rpo)
    {
      for (auto& b : it.graphic)
      {
        gsi << b;
        context.vrd.postpone_destruction_to_next_fence(context.gqueue, std::move(b));
      }
      for (auto& b : it.compute)
      {
        csi << b;
        context.vrd.postpone_destruction_to_next_fence(context.cqueue, std::move(b));
      }
    }
  }
}

