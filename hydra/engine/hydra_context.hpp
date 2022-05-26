
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
#include <hydra/utilities/vk_resource_destructor.hpp>
#include <hydra/utilities/shader_manager.hpp>
#include <hydra/utilities/pipeline_manager.hpp>
#include <hydra/utilities/transfer.hpp>
#include <hydra/renderer/renderer.hpp>

namespace neam::hydra
{
  /// \brief Global, über-struct with general purpose utilities. Can be down-casted to individual sub-contexts
  ///        (this avoid making resource code aware of vulkan stuff)
  struct hydra_context : public core_context, public vk_context
  {
    // rendering stuff:
    memory_allocator allocator = { device };
    vk_resource_destructor vrd;
    shader_manager shmgr = { device, res };
    pipeline_manager ppmgr = { device };
    batch_transfers transfers = { device, tqueue, vrd};

    using vk_context::vk_context;

    hydra_context(hydra_context&&) = delete;
    hydra_context& operator = (hydra_context&&) = delete;
  };
}
