
//
// created by : Timothée Feuillet
// date: 2021-11-21
//
//
// Copyright (c) 2021 Timothée Feuillet
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


#include "vk_context.hpp"
#include "core_context.hpp"

#include <hydra/utilities/memory_allocator.hpp>
#include <hydra/utilities/deferred_fence_execution.hpp>
#include <hydra/utilities/shader_manager.hpp>
#include <hydra/utilities/pipeline_manager.hpp>
#include <hydra/utilities/command_pool_manager.hpp>
#include <hydra/utilities/transfer.hpp>
#include <hydra/utilities/descriptor_allocator.hpp>
#include <hydra/renderer/renderer.hpp>
#include <hydra/renderer/resources/texture_manager.hpp>

#include <hydra/ecs/ecs.hpp>

namespace neam::hydra
{
  /// \brief Global, über-struct with general purpose utilities. Can be down-casted to individual sub-contexts
  ///        (this avoids making resource code aware of vulkan stuff)
  struct hydra_context : public core_context, public vk_context
  {
    // core stuff:
    ecs::database db;

    // rendering stuff:
    memory_allocator allocator = { device };
    deferred_fence_execution dfe { *this };
    shader_manager shmgr = { device, res };
    pipeline_manager ppmgr = { *this, device };

    command_pool_manager gcpm = { *this, gqueue };
    command_pool_manager tcpm = { *this, tqueue };
    command_pool_manager slow_tcpm = { *this, slow_tqueue };
    command_pool_manager ccpm = { *this, cqueue };

    descriptor_allocator da { *this };

    texture_manager textures { *this };

    using vk_context::vk_context;

    hydra_context(hydra_context&&) = delete;
    hydra_context& operator = (hydra_context&&) = delete;

    command_pool_manager& get_cpm(vk::queue& q)
    {
      if (&q == &gqueue) return gcpm;
      if (&q == &tqueue) return tcpm;
      if (&q == &slow_tqueue) return slow_tcpm;
      if (&q == &cqueue) return ccpm;
      check::debug::n_assert(false, "get_cpm: could not find queue");
      return gcpm;
    }
  };
}
