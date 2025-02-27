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
#include "buffer.hpp"
#include "debug_marker.hpp"
#include "image_subresource.hpp"

namespace neam
{
  namespace hydra
  {
    class memory_allocation;

    namespace vk
    {
      /// \brief This class loosely wraps the VkSubmitInfo structure, holding
      /// information about dependencies, semaphores and fences.
      /// It can wraps more than one vkQueueSubmit / vkQueueSparseBind call, and can contain submit information for multiple queues
      /// \note the class is create-only, thus you can't modify things once they
      ///       are inserted, the only operation possible is a clear() that will
      ///       clear every information held by the object
      ///
      /// \note every fences, semaphores and command buffers must be "alive"
      ///       until the object is destructed or a clear() call is done
      ///
      /// \note You have to sumbit datas in that order:
      ///       wait (semaphores) -> execute (command_buffers) | bind (memory) -> signal(semaphores) -> signal (fences)
      ///       You can skip any parts of that chain, or submit multiples elements of the same type.
      ///       The reason of that is the submit info is constructed in "linear" order,
      ///       in the order of execution: you wait for the semaphores to be signaled,
      ///       then execute the command buffers, then signal the semaphores, then the fence.
      class submit_info
      {
        private:
          enum class operation_t : int
          {
            _cut = -1,
            initial = 0,
            wait = initial,
            cmd_buff_or_bind = 1,
            signal_sema = 2,
            signal_fence = 3,
            end_marker = 4,
          };
          struct vk_si_marker
          {
            std::string str;
            VkDebugUtilsLabelEXT marker;
          };

          struct vk_si_vectors
          {
            std::mtc_vector<VkPipelineStageFlags> wait_dst_stage_mask;
            std::mtc_vector<VkCommandBuffer> vk_cmd_bufs;

            std::mtc_vector<VkSemaphore> vk_wait_semas;
            std::mtc_vector<VkSemaphore> vk_sig_semas;

            void update(VkSubmitInfo& si);
          };

          struct vk_sbi_vectors
          {
            std::mtc_vector<VkSparseBufferMemoryBindInfo> vk_buffer_binds;
            std::mtc_vector<VkSparseImageOpaqueMemoryBindInfo> vk_image_opaque_binds;
            std::mtc_vector<VkSparseImageMemoryBindInfo> vk_image_binds;

            std::mtc_vector<VkSemaphore> vk_wait_semas;
            std::mtc_vector<VkSemaphore> vk_sig_semas;

            std::mtc_map<VkBuffer, std::mtc_vector<VkSparseMemoryBind>> buffer_sparse_binds;
            std::mtc_map<VkImage, std::mtc_vector<VkSparseMemoryBind>> image_sparse_opaque_binds;
            std::mtc_map<VkImage, std::mtc_vector<VkSparseImageMemoryBind>> image_sparse_binds;

            void update(VkBindSparseInfo& sbi);
          };

          struct vk_si_wrapper
          {
            //std::mtc_vector<vk_si_marker> begin_markers;
            //std::mtc_vector<vk_si_marker> insert_markers;
            //uint32_t end_markers = 0;

            std::mtc_vector<VkSubmitInfo> vk_submit_infos;
            std::mtc_vector<vk_si_vectors> si_vectors;

            std::mtc_vector<VkBindSparseInfo> vk_sparse_bind_infos;
            std::mtc_vector<vk_sbi_vectors> sbi_vectors;

            VkFence fence = VK_NULL_HANDLE;

            const bool sparse_bind = false;

            vk_si_wrapper(bool is_sparse_bind = false) : sparse_bind(is_sparse_bind)
            {
              add();
            }

            void update(uint32_t index = ~0u);

            void full_update();

            void add();

            void submit(vk_context& vkctx, vk::queue* queue);
          };

        public:
          explicit submit_info(vk_context& _vkctx) : vkctx(_vkctx) {};
          submit_info(submit_info&& o) = default;
          submit_info&operator=(submit_info&& o)
          {
            if (&o == this) return *this;
            current = o.current;
            current_queue = o.current_queue;
            queues = std::move(o.queues);
            o.current = nullptr;
            o.current_queue = nullptr;
            return *this;
          }
          ~submit_info() = default;

          /// \brief Clear the whole submit info
          void clear();

          /// \brief Indicate which queue is to be used for the following commands.
          /// \note following commands will be normal submit commands, not sparse-bind ones
          /// \note Commands are registered per queue and switching between queue will not create new entries.
          ///       Call sync() to ensure that the new entry will be submit after any previous entries
          submit_info& on(vk::queue& q);

          /// \brief Indicate which queue is to be used for the following commands, and that the following commands will be sparse-bind commands
          /// \note Commands are registered per queue and switching between queue will not create new entries.
          ///       Call sync() to ensure that the new entry will be submit after any previous entries
          submit_info& sparse_bind_on(vk::queue& q);

          /// \brief Ensure that anything following this statement will be submit after anything previously queued
          void sync();

          /// \brief Add a semaphore to wait on
          submit_info& wait(const semaphore& sem, VkPipelineStageFlags wait_flags);

          /// \brief Add a semaphore to wait on
          /// \warning Only valid on sparse-binding queues. Will assert otherwise.
          submit_info& wait(const semaphore& sem);

          /// \brief Add a command buffer
          submit_info& execute(const command_buffer &cmdbuf);

          /// \brief Bind a memory area to a buffer
          submit_info& bind(const buffer& buff, memory_allocation& alloc, size_t offset_in_buffer);

          /// \brief Bind a memory area to an image (in the mip-tail segment)
          submit_info& bind_mip_tail(const image& img, memory_allocation& alloc, size_t offset_in_mip_tail);

          /// \brief Bind a memory area to an image
          submit_info& bind(const image& img, memory_allocation& alloc, size_t offset_in_opaque);

          /// \brief Bind a memory area to an image (in the opaque segment)
          submit_info& bind(const image& img, memory_allocation& alloc, glm::uvec3 offset, glm::uvec3 extent, const image_subresource& subres);

          /// \brief Add a semaphore to signal
          submit_info& signal(const semaphore &sem);

          /// \brief Add a fence to signal
          submit_info& signal(const fence &fnc);

          /// \brief Append a \e copy of \p info in the current submit_info
          submit_info& append(const submit_info& info);

          /// \brief Submit everything using the deferred queue submission
          /// \note Will try to do parallel submits as much as possible
          void deferred_submit();
          /// \brief Same as deferred_submit, but does not lock dqe
          void deferred_submit_unlocked();

          /// \brief Begin a new section on the current queue. Implies a cut.
          /// \note If markers are skipped, the cut will still remain to avoid issues
          void begin_marker(std::string_view name, glm::vec4 color = {1, 1, 1, 1});

          /// \brief End a section that was started with begin_marker. Implies a cut.
          /// \note the queue must be the same
          void end_marker();

          /// \brief Insert a debug marker on the current queue.
          void insert_marker(std::string_view name, glm::vec4 color = {1, 1, 1, 1});

        public:
          bool has_any_entries_for(vk::queue& q) const;

          vk::queue* get_current_queue() const { return current_queue; }

        private:
          void step(operation_t current_op);
          void sparse_bind_ops(bool do_sparse_bind);

          void switch_to(vk::queue* q);

          void cut();

        private:
          struct queue_operations_t
          {
            operation_t last_op = operation_t::initial;
            std::mtc_deque<vk_si_wrapper> queue_submits;
          };
          vk_context& vkctx;
          queue_operations_t* current = nullptr;
          vk::queue* current_queue = nullptr;

          std::mtc_deque<std::mtc_map<vk::queue*, std::mtc_deque<queue_operations_t>>> queues;
      };

      using si_debug_marker = debug_marker<submit_info>;
    } // namespace vk
  } // namespace hydra
} // namespace neam
