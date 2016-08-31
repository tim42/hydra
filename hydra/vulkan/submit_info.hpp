//
// file : submit_info.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/submit_info.hpp
//
// created by : Timothée Feuillet
// date: Fri Apr 29 2016 17:07:33 GMT+0200 (CEST)
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

#ifndef __N_17114524758186496_151471944_SUBMIT_INFO_HPP__
#define __N_17114524758186496_151471944_SUBMIT_INFO_HPP__

#include <vector>
#include <deque>
#include <vulkan/vulkan.h>

#include "command_buffer.hpp"
#include "semaphore.hpp"
#include "fence.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief This class loosely wraps the VkSubmitInfo structure, holding
      /// information about dependencies, semaphores and fences.
      /// It can wraps more than one vkQueueSubmit call.
      /// \note the class is create-only, thus you can't modify things once they
      ///       are inserted, the only operation possible is a clear() that will
      ///       clear every information held by the object
      ///
      /// \note every fences, semaphores and command buffers must be "alive"
      ///       until the object is destructed or a clear() call is done
      ///
      /// \note You have to sumbit datas in that order:
      ///       add_wait (semaphores) -> add (command_buffers) -> add_signal(semaphores) -> add_signal (fences)
      ///       You can skip any parts of that chain, or submit multiples elements of the same type.
      ///       The reason of that is the submit info is constructed in "linear" order, aka
      ///       in the order of execution: you wait for the semaphores to be signaled,
      ///       then execute the command buffers, then signal the semaphores, then the fence.
      class submit_info
      {
        private:
          struct vk_si_wrapper
          {
            std::vector<VkSubmitInfo> vk_submit_infos;
            std::deque<std::vector<VkPipelineStageFlags>> wait_dst_stage_mask;
            std::deque<std::vector<VkCommandBuffer>> vk_cmd_bufs;
            std::deque<std::vector<VkSemaphore>> vk_wait_semas;
            std::deque<std::vector<VkSemaphore>> vk_sig_semas;
            VkFence fence = VK_NULL_HANDLE;
            bool fence_only = false;

            vk_si_wrapper() {add();}

            void update()
            {
              if (vk_submit_infos.size() != 0)
              {
                vk_submit_infos.back().commandBufferCount = vk_cmd_bufs.back().size();
                vk_submit_infos.back().pWaitDstStageMask = wait_dst_stage_mask.back().data();
                vk_submit_infos.back().pCommandBuffers = vk_cmd_bufs.back().data();
                vk_submit_infos.back().waitSemaphoreCount = vk_wait_semas.back().size();
                vk_submit_infos.back().pWaitSemaphores = vk_wait_semas.back().data();
                vk_submit_infos.back().signalSemaphoreCount = vk_sig_semas.back().size();
                vk_submit_infos.back().pSignalSemaphores = vk_sig_semas.back().data();

                fence_only = vk_submit_infos.size() == 1
                             && vk_submit_infos.back().commandBufferCount == 0
                             && vk_submit_infos.back().waitSemaphoreCount == 0
                             && vk_submit_infos.back().signalSemaphoreCount == 0;
              }
            }

            void add()
            {
              update();

              vk_submit_infos.emplace_back();
              wait_dst_stage_mask.emplace_back();
              vk_cmd_bufs.emplace_back();
              vk_wait_semas.emplace_back();
              vk_sig_semas.emplace_back();

              vk_submit_infos.back().sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
              vk_submit_infos.back().pNext = nullptr;
              vk_submit_infos.back().commandBufferCount = 0;
              vk_submit_infos.back().pWaitDstStageMask = wait_dst_stage_mask.back().data();
              vk_submit_infos.back().pCommandBuffers = vk_cmd_bufs.back().data();
              vk_submit_infos.back().waitSemaphoreCount = 0;
              vk_submit_infos.back().pWaitSemaphores = vk_wait_semas.back().data();
              vk_submit_infos.back().signalSemaphoreCount = 0;
              vk_submit_infos.back().pSignalSemaphores = vk_sig_semas.back().data();
            }

            void submit(const device &dev, VkQueue vk_queue)
            {
              update();
              if (fence_only)
              {
                if (this->fence)
                    check::on_vulkan_error::n_throw_exception(dev._fn_vkQueueSubmit(vk_queue, 0, nullptr, this->fence));
              }
              else
                check::on_vulkan_error::n_throw_exception(dev._fn_vkQueueSubmit(vk_queue, vk_submit_infos.size(), vk_submit_infos.data(), this->fence));
            }
          };

        public:
          submit_info() = default;
          ~submit_info() = default;

          /// \brief Clear the whole submit info
          void clear()
          {
            queue_submits.clear();
          }

          /// \brief Add a semaphore to wait on
          void add_wait(const semaphore &sem, VkPipelineStageFlags wait_flags)
          {
            init(0);

            queue_submits.back().vk_wait_semas.back().push_back(sem._get_vk_semaphore());
            queue_submits.back().wait_dst_stage_mask.back().push_back(wait_flags);
          }

          /// \brief Add a command buffer
          void add(const command_buffer &cmdbuf)
          {
            init(1);

            queue_submits.back().vk_cmd_bufs.back().push_back(cmdbuf._get_vk_command_buffer());
          }

          /// \brief Add a semaphore to signal
          void add_signal(const semaphore &sem)
          {
            init(2);

            queue_submits.back().vk_sig_semas.back().push_back(sem._get_vk_semaphore());
          }

          /// \brief Add a fence to signal
          void add_signal(const fence &fnc)
          {
            if (!queue_submits.size() || queue_submits.back().fence != VK_NULL_HANDLE)
              queue_submits.emplace_back();
            last_op = 3;
            queue_submits.back().fence = fnc._get_vk_fence();
          }

        public: // advanced
          /// \brief Submit everything to a vulkan queue
          /// \note you should use instead queue::submit()
          void _submit(const device &dev, VkQueue vk_queue)
          {
            for (vk_si_wrapper &it : queue_submits)
              it.submit(dev, vk_queue);
          }

        private:
          inline void init(size_t current_op)
          {
            if (!queue_submits.size() || (last_op > current_op && queue_submits.back().fence != VK_NULL_HANDLE))
              queue_submits.emplace_back();
            else if (last_op > current_op)
              queue_submits.back().add();
            last_op = current_op;
          }
        private:
          size_t last_op = 0;
          std::deque<vk_si_wrapper> queue_submits;
      };

      /// \brief Alias for si.add_wait(semaphore)
      using cmd_sema_pair = std::pair<const semaphore &, VkPipelineStageFlags>;
      submit_info &operator << (submit_info &si, const cmd_sema_pair &sem)
      {
        si.add_wait(sem.first, sem.second);
        return si;
      }


      /// \brief Alias for si.add(command_buffer)
      submit_info &operator << (submit_info &si, const command_buffer &cb)
      {
        si.add(cb);
        return si;
      }

      /// \brief Alias for si.add_signal(semaphore)
      submit_info &operator >> (submit_info &si, const semaphore &sem)
      {
        si.add_signal(sem);
        return si;
      }

      /// \brief Alias for si.add_signal(fence)
      submit_info &operator >> (submit_info &si, const fence &fnc)
      {
        si.add_signal(fnc);
        return si;
      }
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_17114524758186496_151471944_SUBMIT_INFO_HPP__

