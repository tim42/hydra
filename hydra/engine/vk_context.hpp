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

#include <hydra/init/bootstrap.hpp>
#include <hydra/vulkan/instance.hpp>
#include <hydra/vulkan/device.hpp>
#include <hydra/vulkan/command_pool.hpp>
#include <hydra/vulkan/queue.hpp>


namespace neam::hydra
{
  /// \brief Most of the important vulkan entries, in a single entry, for you to pass around
  struct vk_context
  {
    vk::instance instance;
    vk::device device;

    vk::queue gqueue;
    vk::queue tqueue;
    vk::queue cqueue;


    vk_context(vk::instance&& _instance, neam::hydra::bootstrap& hydra_init,
               temp_queue_familly_id_t graphic_queue_id,
               temp_queue_familly_id_t transfer_queue_id,
               temp_queue_familly_id_t compute_queue_id)
      : instance(std::move(_instance)),
        device(hydra_init.create_device(instance)),
        gqueue(device, graphic_queue_id),
        tqueue(device, transfer_queue_id),
        cqueue(device, compute_queue_id)
    {
    }

    // We rely on addresses staying the same, so we cannot move
    // If you need to transfer ownership of this struct, use a unique_pointer
    vk_context(vk_context&&) = delete;
    vk_context& operator = (vk_context&&) = delete;
  };
}
