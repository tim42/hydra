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

#ifndef __N_3064046323163029733_126501074_QUEUE_HPP__
#define __N_3064046323163029733_126501074_QUEUE_HPP__

#include <cstddef>
#include <list>

#include "device.hpp"
#include "command_pool.hpp"
#include "fence.hpp"
#include "semaphore.hpp"
#include "submit_info.hpp"
#include "swapchain.hpp"

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
          queue(device &_dev, size_t _queue_familly_index, size_t _queue_index)
            : dev(_dev), queue_familly_index(_queue_familly_index), queue_index(_queue_index)
          {
            dev._vkGetDeviceQueue(queue_familly_index, queue_index, &vk_queue);
          }

        public:
          /// \brief Create the queue from a temporary queue_id
          queue(device &_dev, temp_queue_familly_id_t queue_id)
            : dev(_dev)
          {
            std::pair<size_t, size_t> nfo = dev._get_queue_info(queue_id);
            queue_familly_index = nfo.first;
            queue_index = nfo.second;

            dev._vkGetDeviceQueue(queue_familly_index, queue_index, &vk_queue);
          }

          /// \brief Return the familly index of the queue
          size_t get_queue_familly_index() const
          {
            return queue_familly_index;
          }

          /// \brief Return the index of the queue inside the queue familly
          size_t get_queue_index() const
          {
            return queue_index;
          }

          /// \brief Create a new command pool
          command_pool create_command_pool(VkCommandPoolCreateFlags flags = 0)
          {
            VkCommandPoolCreateInfo cmd_pool_info = {};
            cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmd_pool_info.pNext = nullptr;
            cmd_pool_info.queueFamilyIndex = queue_familly_index;
            cmd_pool_info.flags = flags;

            VkCommandPool cmd_pool;
            check::on_vulkan_error::n_throw_exception(dev._vkCreateCommandPool(&cmd_pool_info, nullptr, &cmd_pool));

            return command_pool(dev, cmd_pool);
          }

          /// \brief Submit some work to the queue
          /// The submit info object is the best way to submit some work through
          /// the queue as it is a re-usable object and can wrap more than one
          /// vkQueueSubmit() call.
          /// If you always submit the same command buffers with the same
          /// semaphores/fences to wait for/signal, then this is the ideal
          /// solution as you initialize one time a submit_info object, then
          /// everything you have to do is to call submit() with that object.
          void submit(submit_info &si)
          {
            si._submit(dev, vk_queue);
          }

          /// \brief Submit a fence to the queue. That fence will be signaled
          /// when all the work previously submitted will be done.
          void submit(const fence &fence_to_sig)
          {
            check::on_vulkan_error::n_throw_exception(dev._fn_vkQueueSubmit(vk_queue, 0, nullptr, fence_to_sig._get_vk_fence()));
          }

          /// \brief Wait the queue to be idle
          void wait_idle() const
          {
            dev._fn_vkQueueWaitIdle(vk_queue);
          }

          /// \brief Submit a request to present the image
          void present(const swapchain &sw, uint32_t image_index, const std::vector<const semaphore *> &wait_semaphore)
          {
            std::vector<VkSemaphore> vk_wait_sema;
            vk_wait_sema.reserve(wait_semaphore.size());
            for (const semaphore *it : wait_semaphore)
              vk_wait_sema.push_back(it->_get_vk_semaphore());

            VkSwapchainKHR vk_sw = sw._get_vk_swapchain();

            VkPresentInfoKHR present_info
            {
              VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr,
              (uint32_t)vk_wait_sema.size(), vk_wait_sema.data(),

              1, &vk_sw, &image_index,
              nullptr
            };

            check::on_vulkan_error::n_throw_exception(vkQueuePresentKHR(vk_queue, &present_info));
          }

        public: // advanced
          /// \brief Return the vulkan queue object
          VkQueue _get_vk_queue() const
          {
            return vk_queue;
          }

        private:
          device &dev;
          size_t queue_familly_index;
          size_t queue_index;
          VkQueue vk_queue;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_3064046323163029733_126501074_QUEUE_HPP__

