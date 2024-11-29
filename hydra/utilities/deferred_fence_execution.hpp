//
// created by : Timothée Feuillet
// date: 2023-9-8
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

#include <tuple>

#include <ntools/id/string_id.hpp>
#include <ntools/mt_check/deque.hpp>
#include <ntools/mt_check/vector.hpp>
#include <ntools/spinlock.hpp>
#include <ntools/threading/types.hpp>
#include <ntools/frame_allocation.hpp>

#include <hydra/vulkan/fence.hpp>
#include <hydra/vulkan/queue.hpp>

namespace neam::hydra
{
  struct hydra_context;

  /// \brief Defer execution (or destruction) to after a fence/fences being signaled
  /// \note Also tracks generic progress across all the queues to avoid spamming fences for every resource waiting to be destructed
  class deferred_fence_execution
  {
    private:
      const uint32_t k_full_mask = ~0u;

    public:
      deferred_fence_execution(hydra_context& _hctx) : hctx(_hctx) {}

      /// \brief Defer the call to when all the queues in the specified mask have completed the current frame
      void defer(uint32_t mask, threading::function_t&& function);
      /// \brief Defer the call to when all the queues have completed the current frame
      void defer(threading::function_t&& function) { return defer(k_full_mask, std::move(function)); }

      /// \brief Defer the destruction to when all the queues in the specified mask have completed the current frame
      template<typename... Args> void defer_destruction(uint32_t mask, Args&& ...objs) { return defer(mask, [...objs = std::move(objs)]{}); }

      /// \brief Defer the destruction to when all the queues in the specified mask have completed the current frame
      /// \note prefer the queue_mask call when there's multiple entries to free to avoid making multiple searches
      template<typename... Args> void defer_destruction(const vk::queue& queue, Args&& ...objs) { return defer_destruction(queue_mask(queue), std::move(objs)...); }

      /// \brief Defer the destruction to when all the queues have completed the current frame
      template<typename... Args> void defer_destruction(Args&& ...objs) { return defer_destruction(k_full_mask, std::move(objs)...); }

      /// \brief Call the function when the fence has been completed
      /// \note Destroy the fence afterward
      /// \note In most cases, defer() is preferrable to this (defer is faster)
      void call_on_fence_completion(vk::fence&& fence, threading::function_t function);

      template<typename... Args>
      uint32_t queue_mask(const Args& ...queues) { return ((1u << queue_index(queues)) | ...); }

    public:
      /// \brief Poll and dispatch entries. (entries will be dispatch in tasks belonging to the same task-group as the current task)
      void poll() { do_poll(true); }

      /// \brief Like poll, but single threaded. Will not touch the task-manager. Slow.
      void poll_single_threaded() { do_poll(false); }

      /// \brief Set the end frame fences for all the queues (or all the queues that had something this frame)
      /// This effectively ends the frame. Anything deferred after the return of this function will go to the next frame.
      /// \warning Must be called after poll() has returned (and must be called after a call to poll has been made)
      void set_end_frame_fences(std::vector<std::pair<uint32_t, vk::fence>>&& queue_fences);

      /// \brief Return whether the current instance has any deferred entries that are pending execution
      bool has_any_pending_entries() const;

      /// \brief Indicate that the vulkan device is idle.
      /// Similar to set_end_frame_fences, but assumes that anything is completed, no end-frame fence needed
      void _assume_vulkan_device_is_idle();

    private:
      uint32_t queue_index(const vk::queue& q);

    private:
      static constexpr uint32_t k_max_queue_count = 8;

      struct tracked_queue_progress_t
      {
        uint64_t last_finished_frame;
        uint64_t last_submit_frame;
      };

      struct frame_entry_t
      {
        uint32_t queue_mask;
        threading::function_t function;
      };
      using frame_entry_frame_alloc_t = cr::frame_allocator<1, true, alignof(frame_entry_t)>;

      struct single_fence_entry_t
      {
        vk::fence fence;
        threading::function_t function;
      };
      using single_fence_frame_alloc_t = cr::frame_allocator<1, true, alignof(single_fence_entry_t)>;


      struct frame_allocations_t
      {
        bool is_submit = false;
        uint32_t completed_queues_mask = 0;

        std::mtc_vector<std::pair<uint32_t, vk::fence>> queue_fences;

        std::optional<typename frame_entry_frame_alloc_t::allocator_state> raw_frame_entries;
        std::mtc_vector<frame_entry_t> remaining_frame_entries;

        std::optional<typename single_fence_frame_alloc_t::allocator_state> raw_single_fence_entries;
        std::mtc_vector<single_fence_entry_t> remaining_single_fence_entries;

        spinlock lock;

        bool has_remaining_entries() const
        {
          return !queue_fences.empty()
              ||!!raw_frame_entries || !remaining_frame_entries.empty()
              || !!raw_single_fence_entries || !remaining_single_fence_entries.empty();
        }
      };

    private:
      void do_poll(bool use_tasks);

      /// \brief Process a frame data, usually called in a task
      void process_frame(bool use_tasks, uint32_t frame_alloc_index);
      void process_single_fence_frame(bool use_tasks, uint32_t frame_alloc_index);

    private:

      hydra_context& hctx;

      mutable shared_spinlock queue_list_lock;
      const vk::queue* queues[k_max_queue_count];
      uint32_t queue_count = 0;

      frame_entry_frame_alloc_t this_frame_entries;
      single_fence_frame_alloc_t this_frame_single_fences;

      // only protect the container, not the contents
      mutable shared_spinlock frame_entries_lock;
      std::mtc_deque<frame_allocations_t> frame_entries;
  };
}
