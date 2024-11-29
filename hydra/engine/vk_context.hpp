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

#include "core_context.hpp"


namespace neam::hydra
{
  /// \brief Most of the important vulkan entries, in a single entry, for you to pass around
  struct vk_context
  {
    core_context& cctx;
    vk::instance instance;
    vk::device device;

    vk::queue gqueue;
    vk::queue tqueue;
    vk::queue slow_tqueue;
    vk::queue cqueue;
    vk::queue spqueue;

    deferred_queue_execution dqe { cctx.tm };

    template<typename TrueBaseType>
    vk_context(TrueBaseType*, vk::instance&& _instance, neam::hydra::bootstrap& hydra_init,
               const temp_queue_familly_id_t* graphic_queue_id,
               const temp_queue_familly_id_t* transfer_queue_id,
               const temp_queue_familly_id_t* slow_transfer_queue_id,
               const temp_queue_familly_id_t* compute_queue_id,
               const temp_queue_familly_id_t* sparse_binding_queue_id,
               hydra_device_creator::filter_device_preferences vulkan_device_preferences = hydra_device_creator::prefer_discrete_gpu)
      : cctx(*static_cast<TrueBaseType*>(this)),
        instance(std::move(_instance)),
        device(hydra_init.create_device(instance, vulkan_device_preferences)),
        gqueue(device, *graphic_queue_id),
        tqueue(device, *transfer_queue_id),
        slow_tqueue(device, *slow_transfer_queue_id),
        cqueue(device, *compute_queue_id),
        spqueue(device, *sparse_binding_queue_id)
    {
      gqueue.queue_id = "gqueue"_rid;
      tqueue.queue_id = "tqueue"_rid;
      slow_tqueue.queue_id = "slow_tqueue"_rid;
      cqueue.queue_id = "cqueue"_rid;
      spqueue.queue_id = "spqueue"_rid;
    }

    // We rely on addresses staying the same, so we cannot move
    // If you need to transfer ownership of this struct, use a unique_pointer
    vk_context(vk_context&&) = delete;
    vk_context& operator = (vk_context&&) = delete;

    std::string get_queue_name(vk::queue& q) const
    {
      if (&q == &gqueue) return "gqueue";
      if (&q == &tqueue) return "tqueue";
      if (&q == &slow_tqueue) return "slow_tqueue";
      if (&q == &cqueue) return "cqueue";
      if (&q == &spqueue) return "spqueue";
      return "<unknown>";
    }
    std::string get_queue_name(VkQueue q) const
    {
      if (q == gqueue._get_vk_queue()) return "gqueue";
      if (q == tqueue._get_vk_queue()) return "tqueue";
      if (q == slow_tqueue._get_vk_queue()) return "slow_tqueue";
      if (q == cqueue._get_vk_queue()) return "cqueue";
      if (q == spqueue._get_vk_queue()) return "spqueue";
      if (q == nullptr) return "<nullptr>";
      return "<unknown>";
    }
  };
}
