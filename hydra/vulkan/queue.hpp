//
// file : queue.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/queue.hpp
//
// created by : Timothée Feuillet
// date: Thu Apr 28 2016 16:10:45 GMT+0200 (CEST)
//
//
// Copyright (c) 2016 Timothée Feuillet
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


#include <cstddef>
#include <list>

#include "device.hpp"
#include "command_pool.hpp"
#include "fence.hpp"
#include "semaphore.hpp"
#include "swapchain.hpp"

#include <utilities/deferred_queue_execution.hpp>

#include <ntools/tracy.hpp>
#include <ntools/threading/threading.hpp>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Describe a queue inside a queue familly
      /// \note because of vulkan, that class can be copied
      class queue
      {
        public: // advanced
          queue(device &_dev, uint32_t _queue_familly_index, uint32_t _queue_index)
            : dev(_dev), queue_familly_index(_queue_familly_index), queue_index(_queue_index)
          {
            dev._vkGetDeviceQueue(queue_familly_index, queue_index, &vk_queue);
          }

        public:
          /// \brief Create the queue from a temporary queue_id
          queue(device &_dev, temp_queue_familly_id_t _queue_id)
            : dev(_dev)
          {
            std::pair<uint32_t, uint32_t> nfo = dev._get_queue_info(_queue_id);
            queue_familly_index = nfo.first;
            queue_index = nfo.second;

            dev._vkGetDeviceQueue(queue_familly_index, queue_index, &vk_queue);
          }

          /// \brief Return the familly index of the queue
          [[nodiscard]] uint32_t get_queue_familly_index() const
          {
            return queue_familly_index;
          }

          /// \brief Return the index of the queue inside the queue familly
          [[nodiscard]] uint32_t get_queue_index() const
          {
            return queue_index;
          }

          /// \brief Create a new command pool
          /// \note Please use command_pool_manager instead for transient pools as it assign a pool for a given frame / thread and reset it when they are done
          [[nodiscard]] command_pool _create_command_pool(VkCommandPoolCreateFlags flags = 0)
          {
            VkCommandPoolCreateInfo cmd_pool_info = {};
            cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmd_pool_info.pNext = nullptr;
            cmd_pool_info.queueFamilyIndex = queue_familly_index;
            cmd_pool_info.flags = flags;

            VkCommandPool cmd_pool;
            check::on_vulkan_error::n_assert_success(dev._vkCreateCommandPool(&cmd_pool_info, nullptr, &cmd_pool));

            command_pool ret(dev, cmd_pool);
#if !N_DISABLE_CHECKS
            ret.queue = vk_queue;
#endif
            return ret;
          }

          /// \brief Submit a fence to the queue. That fence will be signaled
          /// when all the work previously submitted will be done.
          void submit(deferred_queue_execution& dqe, const fence& fence_to_sig)
          {
            std::lock_guard _lg(dqe.lock);
            dqe.defer_sync_unlocked(); // FIXME: remove?
            dqe.defer_execution_unlocked(queue_id, [this, fence = fence_to_sig._get_vk_fence()]
            {
              TRACY_SCOPED_ZONE;
              // std::lock_guard _l(queue_lock);
              check::on_vulkan_error::n_assert_success(dev._vkQueueSubmit(vk_queue, 0, nullptr, fence));
            });
          }

          /// \brief Wait the queue to be idle
          void wait_idle() const
          {
            TRACY_SCOPED_ZONE;
            dev._vkQueueWaitIdle(vk_queue);
          }

          /// \brief Submit a request to present the image
          void present(deferred_queue_execution& dqe, const swapchain& sw, uint32_t image_index, const std::vector<const semaphore*>& wait_semaphore, bool* out_of_date = nullptr)
          {
            TRACY_SCOPED_ZONE;
            std::vector<VkSemaphore> vk_wait_sema;
            vk_wait_sema.reserve(wait_semaphore.size());
            for (const semaphore *it : wait_semaphore)
              vk_wait_sema.push_back(it->_get_vk_semaphore());

            VkSwapchainKHR vk_sw = sw._get_vk_swapchain();

            std::lock_guard _lg(dqe.lock);
            dqe.defer_sync_unlocked();
            dqe.defer_execution_unlocked(queue_id, [this, vk_sw, &lock = sw.lock, image_index, vk_wait_sema = std::move(vk_wait_sema)]
            {
              TRACY_SCOPED_ZONE;
              VkResult res = VK_SUCCESS;
              VkPresentInfoKHR present_info
              {
                VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr,
                (uint32_t)vk_wait_sema.size(), vk_wait_sema.data(),

                1, &vk_sw, &image_index, &res,
              };
              // std::lock_guard _l(queue_lock);
              std::lock_guard _l(const_cast<spinlock&>(lock));
              /*auto result = */vkQueuePresentKHR(vk_queue, &present_info);
              // if (result == VK_ERROR_OUT_OF_DATE_KHR && out_of_date)
              //   *out_of_date = true;
              // else
                // check::on_vulkan_error::n_assert_success(result);
            });
          }

        public: // advanced
          /// \brief Return the vulkan queue object
          VkQueue _get_vk_queue() const
          {
            return vk_queue;
          }

          void _set_debug_name(const std::string& name)
          {
            dev._set_object_debug_name((uint64_t)vk_queue, VK_OBJECT_TYPE_QUEUE, name);
          }

          id_t queue_id = id_t::invalid;
        private:
          device &dev;
          uint32_t queue_familly_index;
          uint32_t queue_index;
          VkQueue vk_queue;
        public:
          spinlock queue_lock;
          friend class submit_info;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



