//
// created by : Timothée Feuillet
// date: 2/11/25
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

#pragma once

#include <ntools/async/chain.hpp>
#include "../../vulkan/submit_info.hpp"
#include "ecs.hpp"

namespace neam::hydra
{
  namespace renderer::concepts
  {
    class gpu_task_producer;
  }

  struct hydra_context;
}

namespace neam::hydra::renderer::internals
{
  class gpu_task_order : public ecs::internal_component<gpu_task_order>
  {
    public:
      gpu_task_order(param_t p) : internal_component_t(p) {}

      /// \brief update all the gpu-task-producers in the hierarchy and submit their tasks to the gpu
      /// \note dispatches a task, and immediately returns
      void prepare_and_dispatch_gpu_tasks(hydra_context& hctx);

    private:
      void push_pass_data(async::chain<std::vector<vk::submit_info>>&& chain)
      {
        pass_data.emplace_back(std::move(chain));
      }

      /// \brief prepare the submission process so that once all the render tasks are complete,
      /// the data is queued to be sent to the gpu
      /// \note once everything is submit (using th DQE), the data is cleared
      /// \warning push_pass_data should never be called once the prepare_submission is done
      void prepare_submissions(hydra_context& hctx);

    private:
      std::mtc_deque<async::chain<std::vector<vk::submit_info>>> pass_data;
      std::mtc_vector<std::vector<vk::submit_info>> vvsi;

      std::atomic<uint32_t> remaining_tasks;
      friend concepts::gpu_task_producer;
  };
}
