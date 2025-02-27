//
// created by : Timothée Feuillet
// date: Sat Aug 27 2016 15:53:51 GMT+0200 (CEST)
//
//
// Copyright (c) 2023 Timothée Feuillet
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

#include <ntools/mt_check/deque.hpp>
#include <ntools/mt_check/map.hpp>
#include <ntools/mt_check/vector.hpp>
#include <cstring>
#include <mutex>
#include <atomic>

#include <ntools/spinlock.hpp>
#include <ntools/tracy.hpp>
#include <ntools/threading/threading.hpp>
#include <ntools/async/async.hpp>
#include <hydra_glm.hpp>


#include "../hydra_debug.hpp"
#include "../vulkan/vulkan.hpp"

#include "memory_allocator.hpp"
#include "holders.hpp"

namespace neam::hydra
{
  struct hydra_context;

  /// \brief Batch and dispatch cpu to gpu data transfers (using staging buffers or directly in buffers, ...)
  /// \note this class is simply a helper to perform the necessary memcpy, acquisition, submission, release
  class transfer_context
  {
    public:
      transfer_context(hydra_context &_hctx, vk::queue& _tqueue) : hctx(_hctx), tqueue(_tqueue) {}
      explicit transfer_context(hydra_context &_hctx);

      void set_global_wait_semaphore(vk::semaphore& _wait_sema) { wait_sema = &_wait_sema; }
      void set_global_signal_fence(vk::fence& _sig_fence) { sig_fence = &_sig_fence; }

      /// \brief Acquire a resource from another queue
      /// \note Acquire operations are done before any transfers, independently of the order of function calls
      /// \note If the resource is newly created, this step is unnecessary
      void acquire(vk::buffer& buf, vk::queue& src_queue, vk::semaphore* wait_semaphore = nullptr);

      /// \brief Acquire a resource from another queue
      /// \note Acquire operations are done before any transfers, independently of the order of function calls
      /// \note If the resource is newly created, this step is unnecessary
      /// \param source_layout The current layout of the image (set to VK_IMAGE_LAYOUT_UNDEFINED to discard contents)
      void acquire(vk::image& img, vk::queue& src_queue, VkImageLayout source_layout = VK_IMAGE_LAYOUT_UNDEFINED, vk::semaphore* wait_semaphore = nullptr);

      /// \brief Acquire an image that only require a layout transition, not a queue transfer
      /// \note Acquire operations are done before any transfers, independently of the order of function calls
      /// \note If the resource is newly created, this step is unnecessary
      /// \param source_layout The current layout of the image (set to VK_IMAGE_LAYOUT_UNDEFINED to discard contents)
      void acquire(vk::image& img, VkImageLayout source_layout = VK_IMAGE_LAYOUT_UNDEFINED, vk::semaphore* wait_semaphore = nullptr);

      /// \brief Acquire an image that require a layout transition, using a custom layout for the copy operation
      void acquire_custom_layout_transition(vk::image& img, VkImageLayout source_layout, VkImageLayout copy_layout, vk::semaphore* wait_semaphore = nullptr);

      /// \brief Indicate that the resource should be released to a specific queue
      /// \note Release operations are done after the transfers, independently of the order of function calls
      void release(vk::buffer& buf, vk::queue& dst_queue, vk::semaphore* signal_semaphore = nullptr);

      /// \brief Indicate that the resource should be released to a specific queue
      /// \note Release operations are done after the transfers, independently of the order of function calls
      void release(vk::image& img, vk::queue& dst_queue, VkImageLayout dst_layout, vk::semaphore* signal_semaphore = nullptr);

      /// \brief Layout change on release (after the transfers)
      void release_custom_layout_transition(vk::image& img, VkImageLayout copy_layout, VkImageLayout dst_layout, vk::semaphore* signal_semaphore = nullptr);


      /// \brief Add a buffer to be filled with some data
      /// \note The cost of this function is constant (a task creation)
      void transfer(vk::buffer& buf, raw_data&& data, size_t buf_offset = 0);

      /// \brief Similar to transfer, but will only add the transfer once the copy task is done, potentially spanning multiple frames.
      /// This function is aimed at cpu to gpu transfer where the cost of memcpy is big and the result isn't necessary for the current frame (large data stream-in is a good use-case for this)
      /// \warning release operation should be called in the then of the async, while remembering that the completion will be called in another thread
      /// \note Can be cancelled
      /// \note If not completed, ignored in append().
      /// \note The cost of this function is constant (a task creation)
      [[nodiscard]] async::continuation_chain async_transfer(vk::buffer& buf, raw_data&& data, size_t buf_offset = 0);

      /// \brief Add a subregion of an image to be filled with some data
      /// \note The cost of this function is constant (a task creation)
      void transfer(vk::image& img, raw_data&& data, const glm::uvec3& size, const glm::ivec3& offset = {0, 0, 0}, vk::image_subresource_layers isl = {}, VkImageLayout current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

      /// \brief Similar to transfer, but will only add the transfer once the copy task is done, potentially spanning multiple frames.
      /// This function is aimed at cpu to gpu transfer where the cost of memcpy is big and the result isn't necessary for the current frame (large data stream-in is a good use-case for this)
      /// \warning release operation should be called in the then of the async, while remembering that the completion will be called in another thread
      /// \note Can be cancelled
      /// \note If not completed, ignored in append().
      /// \note The cost of this function is constant (a task creation)
      [[nodiscard]] async::continuation_chain async_transfer(vk::image& img, raw_data&& data, const glm::uvec3& size, const glm::ivec3& offset = {0, 0, 0}, vk::image_subresource_layers isl = {}, VkImageLayout current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

      /// \brief Add an image to be filled with some data
      /// \note The cost of this function is constant (a task creation)
      void transfer(vk::image& img, raw_data&& data)
      {
        return transfer(img, std::move(data), img.get_size());
      }

      /// \brief Add an image to be filled with some data
      /// \note The cost of this function is constant (a task creation)
      void transfer(vk::image& img, raw_data&& data, const glm::uvec3& size, vk::image_subresource_layers isl, VkImageLayout current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
      {
        return transfer(img, std::move(data), size, {0, 0, 0}, isl, current_layout);
      }

      /// \brief Add an image to be filled with some data
      /// \note The cost of this function is constant (a task creation)
      /// The returned chain will be completed when the gpu transfer is actually completed (_not_ when the command is sent or the memcpy finished)
      [[nodiscard]] async::continuation_chain async_transfer(vk::image& img, raw_data&& data)
      {
        return async_transfer(img, std::move(data), img.get_size());
      }

      /// \brief Add an image to be filled with some data
      /// \note The cost of this function is constant (a task creation)
      /// The returned chain will be completed when the gpu transfer is actually completed (_not_ when the command is sent or the memcpy finished)
      [[nodiscard]] async::continuation_chain async_transfer(vk::image& img, raw_data&& data, const glm::uvec3& size, vk::image_subresource_layers isl, VkImageLayout current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
      {
        return async_transfer(img, std::move(data), size, {0, 0, 0}, isl, current_layout);
      }

      /// \brief Return whether any copy operation are still in progress
      /// \note does not account for any async operations
      [[nodiscard]] bool has_any_operation_still_in_progress() const;

      /// \brief Remove any operations for the provided resource.
      /// \warning If there is any transfer queued for the resource, this function will have to wait for all the tasks to finish
      void remove_operations_for(vk::buffer& buffer);

      /// \brief Remove any operations for the provided resource.
      /// \warning If there is any transfer queued for the resource, this function will have to wait for all the tasks to finish
      void remove_operations_for(vk::image& image);

      [[nodiscard]] async::continuation_chain queue_operation_on_build();

      /// \brief Append all the acquire - transfer - release to the submit info
      /// \note May stall for a bit, waiting for all the copies to finish
      void build(vk::submit_info& si);

      void clear();

      /// \brief append \p other to the current context
      /// \note doesn't wait on any in-progress tasks, tho they will automatically transfer to the correct context
      void append(transfer_context& other);

      std::string debug_context;

    private:
      void dispatch_copy_task(raw_data&& data, std::optional<buffer_holder>& holder);

      template<typename GetHolderFnc>
      void inner_copy_task(raw_data&& data, GetHolderFnc get_holder);

    private:
      struct buffer_acqrel_t
      {
        VkBuffer buffer;
        VkSemaphore semaphore;
        VkAccessFlags access;
      };

      struct buffer_copy_t
      {
        VkBuffer dst_buffer;

        // vk::buffer src_buffer;
        std::unique_ptr<std::optional<buffer_holder>> src_buffer;
        size_t offset;
        size_t size;

        async::continuation_chain::state completion_state;
      };

      struct image_acqrel_t
      {
        VkImage image;
        VkSemaphore semaphore;

        VkImageLayout layout;
        VkImageLayout layout_for_copy;
        VkAccessFlags access;
      };

      struct image_copy_t
      {
        VkImage dst_image;
        // vk::buffer src_buffer;
        std::unique_ptr<std::optional<buffer_holder>> src_buffer;
        glm::uvec3 offset;
        glm::uvec3 size;
        vk::image_subresource_layers isl;
        VkImageLayout layout;
        async::continuation_chain::state completion_state;
      };

      struct acqrel_t
      {
        std::mtc_vector<buffer_acqrel_t> buffers;
        std::mtc_vector<image_acqrel_t> images;
      };

  private:
      hydra_context& hctx;
      vk::queue& tqueue;
      vk::semaphore* wait_sema = nullptr;
      vk::fence* sig_fence = nullptr;

      mutable spinlock lock;
      std::mtc_map<vk::queue*, acqrel_t> acquisitions;
      std::mtc_map<vk::queue*, acqrel_t> releases;

      std::mtc_deque<buffer_copy_t> buffer_copies;
      std::mtc_deque<image_copy_t> image_copies;

      std::mtc_deque<async::continuation_chain::state> states;

      std::mtc_vector<threading::task_completion_marker_ptr_t> tasks;
  };
}

