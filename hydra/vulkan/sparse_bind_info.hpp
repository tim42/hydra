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

#pragma once


#include <ntools/mt_check/vector.hpp>
#include <ntools/mt_check/map.hpp>
#include <vulkan/vulkan.h>

#include "engine/vk_context.hpp"
#include "engine/core_context.hpp"
#include "command_buffer.hpp"
#include "semaphore.hpp"
#include "fence.hpp"
#include "queue.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief This class loosely wraps the VkSubmitInfo structure, holding
      /// information about dependencies, semaphores and fences.
      /// It can wraps more than one vkQueueSubmit call, and can contain submit information for multiple queues
      /// \note the class is create-only, thus you can't modify things once they
      ///       are inserted, the only operation possible is a clear() that will
      ///       clear every information held by the object
      ///
      /// \note every fences, semaphores and command buffers must be "alive"
      ///       until the object is destructed or a clear() call is done
      ///
      /// \note You have to sumbit datas in that order:
      ///       wait (semaphores) -> bind (memory) -> signal(semaphores) -> signal (fences)
      ///       You can skip any parts of that chain, or submit multiples elements of the same type.
      ///       The reason of that is the sprse bind info is constructed in "linear" order,
      ///       in the order of execution: you wait for the semaphores to be signaled,
      ///       then execute the command buffers, then signal the semaphores, then the fence.
      class sparse_bind_info
      {
        private:
          enum class operation_t : int
          {
            _cut = -1,
            initial = 0,
            wait = initial,
            bind = 1,
            signal_sema = 2,
            signal_fence = 3,
          };
          struct vk_sbi_wrapper
          {
            std::mtc_vector<VkSubmitInfo> vk_submit_infos;
            std::mtc_vector<std::mtc_vector<VkPipelineStageFlags>> wait_dst_stage_mask;
            std::mtc_vector<std::mtc_vector<VkCommandBuffer>> vk_cmd_bufs;
            std::mtc_vector<std::mtc_vector<VkSemaphore>> vk_wait_semas;
            std::mtc_vector<std::mtc_vector<VkSemaphore>> vk_sig_semas;
            VkFence fence = VK_NULL_HANDLE;
            // VkQueue queue = VK_NULL_HANDLE;
            bool fence_only = true;

            vk_sbi_wrapper() {add();}

            void update();

            void full_update();

            void add();

            bool is_empty() const;

            void sparse_bind(vk_context& vkctx, vk::queue* queue);
          };

        public:
          explicit sparse_bind_info(vk_context& _vkctx) : vkctx(_vkctx) {};
          ~sparse_bind_info() = default;

          /// \brief Clear the whole sp info
          void clear();

          /// \brief Indicate which queue is to be used for the following commands.
          /// \note Commands are registered per queue and switching between queue will not create new entries.
          ///       Call sync() to ensure that the new entry will be submit after any previous entries
          /// \note The default queue is vk_context.spqueue, which should be the only one needed
          sparse_bind_info& on(vk::queue& q);

          /// \brief Ensure that anything following this statement will be submit after anything previously queued
          void sync();

          /// \brief Add a semaphore to wait on
          sparse_bind_info& wait(const semaphore &sem, VkPipelineStageFlags wait_flags);

          /// \brief Bind a memory area
          sparse_bind_info& bind(const command_buffer &cmdbuf);

          /// \brief Add a semaphore to signal
          sparse_bind_info& signal(const semaphore &sem);

          /// \brief Add a fence to signal
          sparse_bind_info& signal(const fence &fnc);

          /// \brief Append a \e copy of \p info in the current sparse_bind_info
          sparse_bind_info& append(const sparse_bind_info& info);

          /// \brief Submit everything using the deferred queue submission
          /// \note Will try to do parallel submits as much as possible
          void deferred_submit();


        public:
          bool has_any_entries_for(vk::queue& q) const;

          vk::queue* get_current_queue() const { return current_queue; }

        private:
          void step(operation_t current_op);

          void switch_to(vk::queue* q);

          void cut();

        private:
          struct queue_operations_t
          {
            operation_t last_op = operation_t::initial;
            std::mtc_vector<vk_si_wrapper> queue_submits;
          };
          vk_context& vkctx;
          queue_operations_t* current = nullptr;
          vk::queue* current_queue = nullptr;

          std::mtc_vector<std::mtc_map<vk::queue*, std::mtc_vector<queue_operations_t>>> queues;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam
