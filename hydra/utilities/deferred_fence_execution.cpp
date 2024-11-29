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

#include "deferred_fence_execution.hpp"

#include "../engine/hydra_context.hpp"

namespace neam::hydra
{
  void deferred_fence_execution::defer(uint32_t mask, threading::function_t&& function)
  {
    this_frame_entries.allocate<frame_entry_t>(/*mask*/~0u, std::move(function));
  }

  void deferred_fence_execution::call_on_fence_completion(vk::fence&& fence, threading::function_t function)
  {
    this_frame_single_fences.allocate<single_fence_entry_t>(std::move(fence), std::move(function));
  }

  void deferred_fence_execution::set_end_frame_fences(std::vector<std::pair<uint32_t, vk::fence>>&& queue_fences)
  {
    TRACY_SCOPED_ZONE;

    auto frame_entries_state = this_frame_entries.swap_and_reset();
    frame_entries_state.build_array_access_accelerator();

    auto single_fence_state = this_frame_single_fences.swap_and_reset();
    single_fence_state.build_array_access_accelerator();

    // compute a mask where missing queues are treated as "completed"
    uint32_t initial_queue_mask = k_full_mask;
    for (auto&& it : queue_fences)
      initial_queue_mask &= ~(1u << it.first);

    {
      std::lock_guard _lg { spinlock_shared_adapter::adapt(frame_entries_lock) };

      if (frame_entries.empty() || frame_entries.back().has_remaining_entries())
      {
        [[unlikely]];
        std::lock_guard _lg { spinlock_shared_to_exclusive_adapter::adapt(frame_entries_lock) };
        if (frame_entries.empty() || frame_entries.back().has_remaining_entries())
        {
          frame_entries.emplace_back(); // ouch...
        }
      }

      auto& current_entry = frame_entries.back();

      std::lock_guard _fal { current_entry.lock };

      current_entry.is_submit = false;
      current_entry.completed_queues_mask = initial_queue_mask;
      current_entry.queue_fences = std::move(queue_fences);
      if (frame_entries_state.size() > 0)
        current_entry.raw_frame_entries = std::move(frame_entries_state);
      if (single_fence_state.size() > 0)
        current_entry.raw_single_fence_entries = std::move(single_fence_state);

      if (!current_entry.has_remaining_entries() || !current_entry.queue_fences.size())
        return;

      // we are at the very end of the rendering frame, after all the rendering operations,
      // so we can insert a callback to mark the current_entry as submit using DQE:
      {
        std::lock_guard _l { hctx.dqe.lock };
        hctx.dqe.defer_sync_unlocked(); // force a sync, so we wait after all the queues to be submit
        hctx.dqe.defer_execution_unlocked([&current_entry]
        {
          std::lock_guard _fal { current_entry.lock };
          current_entry.is_submit = true;
        });
      }
    }
  }

  void deferred_fence_execution::_assume_vulkan_device_is_idle()
  {
    set_end_frame_fences({});

    std::lock_guard _lg { spinlock_shared_adapter::adapt(frame_entries_lock) };
    for (auto& it : frame_entries)
    {
      std::lock_guard _fal { it.lock };
      it.queue_fences.clear();
      it.completed_queues_mask = ~0u;
      it.is_submit = true;
    }
  }

  bool deferred_fence_execution::has_any_pending_entries() const
  {
    std::lock_guard _lg { spinlock_exclusive_adapter::adapt(frame_entries_lock) };
    for (const auto& it : frame_entries)
    {
      if (it.has_remaining_entries())
        return true;
    }
    return false;
  }

  void deferred_fence_execution::do_poll(bool use_tasks)
  {
    TRACY_SCOPED_ZONE;
    {
      // start by removing empty / completed entries at the front:
      std::lock_guard _lg { spinlock_exclusive_adapter::adapt(frame_entries_lock) };

      while (frame_entries.size() > 0 && !frame_entries.front().has_remaining_entries() && frame_entries.front().is_submit)
      {
        {
          std::lock_guard _fal { frame_entries.front().lock };
        }
        frame_entries.pop_front();
      }

      if (frame_entries.size() > 10)
        cr::out().warn("DFE: More than 10 frames ({} frames) of data are being queued and not being deleted", frame_entries.size());

      // add an entry for the current frame (avoids an exclusive lock later in the frame)
      if (frame_entries.empty() || (frame_entries.back().has_remaining_entries() || frame_entries.back().is_submit))
        frame_entries.emplace_back();
    }

    // we then dispatch one task per frame (which may dispatch more tasks to complete the work)
    {
      std::lock_guard _lg { spinlock_shared_adapter::adapt(frame_entries_lock) };
      // ignore the last two entries
      // (the previous frame entry, as it's being currently submit/not submit yet; the last entry, as it's the current frame, being filled)
      const uint32_t dispatch_count = (uint32_t)frame_entries.size();
      for (uint32_t i = 0; i < dispatch_count; ++i)
      {
        if (!frame_entries[i].is_submit)
          return;

        if (use_tasks)
        {
          [[likely]];
          hctx.tm.get_task([index = i, this]
          {
            process_frame(true, index);
          });
        }
        else
        {
          process_frame(false, i);
        }
      }
    }
  }

  void deferred_fence_execution::process_single_fence_frame(bool use_tasks, uint32_t frame_alloc_index)
  {
    TRACY_SCOPED_ZONE;
    std::mtc_vector<single_fence_entry_t> entries_to_run;

    {
      std::lock_guard _lg { spinlock_shared_adapter::adapt(frame_entries_lock) };

      frame_allocations_t& fa = frame_entries[frame_alloc_index];
      if (fa.raw_single_fence_entries)
      {
        // we have something coming from the raw allocator, use this:
        const typename single_fence_frame_alloc_t::allocator_state state = std::move(fa.raw_single_fence_entries.value());
        fa.raw_single_fence_entries.reset();

        for (uint32_t i = 0; i < state.size(); ++i)
        {
          if (single_fence_entry_t* it = state.get_entry<single_fence_entry_t>(i); it != nullptr)
          {
            if (it->fence.is_signaled())
              entries_to_run.push_back(std::move(*it));
            else
              fa.remaining_single_fence_entries.push_back(std::move(*it));

            // NOTE: destructor will never be called, we must do it ourselves
            it->~single_fence_entry_t();
          }
        }
      }
      else
      {
        std::mtc_vector<single_fence_entry_t> remaining_entries;
        std::swap(fa.remaining_single_fence_entries, remaining_entries);
        const uint32_t entry_count = (uint32_t)remaining_entries.size();

        for (uint32_t i = 0; i < entry_count; ++i)
        {
          auto& it = remaining_entries[i];
          if (it.fence.is_signaled())
            entries_to_run.push_back(std::move(it));
          else
            fa.remaining_single_fence_entries.push_back(std::move(it));
        }
      }
    }

    // run the functions:
    if (use_tasks)
    {
      [[likely]];
      constexpr size_t k_entry_per_dispatch = 8;
      threading::for_each(hctx.tm, hctx.tm.get_current_group(), entries_to_run, [](auto& it, size_t /*index*/)
      {
        TRACY_SCOPED_ZONE;
        it.function();
      }, k_entry_per_dispatch);
    }
    else
    {
      for (auto& it : entries_to_run)
      {
        it.function();
      }
    }
  }

  void deferred_fence_execution::process_frame(bool use_tasks, uint32_t frame_alloc_index)
  {
    TRACY_SCOPED_ZONE;
    std::mtc_vector<frame_entry_t> entries_to_run;

    {
      std::lock_guard _lg { spinlock_shared_adapter::adapt(frame_entries_lock) };

      if (frame_alloc_index >= frame_entries.size())
        return;

      frame_allocations_t& fa = frame_entries[frame_alloc_index];

      // No need to have two threads working on this.
      // (this can happen when deletion takes more time than the cpu frame itself)
      if (!fa.lock.try_lock())
      {
        if (!use_tasks)
        {
          fa.lock.lock();
        }
        else
        {
          return;
        }
      }
      std::lock_guard _fal { fa.lock, std::adopt_lock };


      // maybe a frame finished before we could run and our index was recycled to the entry of the current frame
      if (!fa.is_submit)
        return;

      if (!fa.has_remaining_entries())
        return; // weird.

      if (!!fa.raw_single_fence_entries || !fa.remaining_single_fence_entries.empty())
      {
        if (use_tasks)
        {
          [[likely]];
          // dispatch the single-fence task if needed
          hctx.tm.get_task([this, frame_alloc_index]
          {
            process_single_fence_frame(true, frame_alloc_index);
          });
        }
        else
        {
          process_single_fence_frame(false, frame_alloc_index);
        }
      }

      // compute mask change, return if nothing changed:
      {
        uint32_t completed_fences = 0;
        for (auto&& it : fa.queue_fences)
        {
          if (it.second.is_signaled())
            completed_fences |= 1u << it.first;
        }
        if (completed_fences == 0 && !fa.queue_fences.empty())
          return; // no changes, nothing to do

        // remove fences that matches completed fences
        fa.queue_fences.erase(std::remove_if(fa.queue_fences.begin(), fa.queue_fences.end(), [completed_fences](const auto& it)
        {
          return ((1u << it.first) & completed_fences) != 0;
        }), fa.queue_fences.end());

        // add it to the completed mask
        fa.completed_queues_mask |= completed_fences;
      }

      // iterate over the entries, and put the entries to run in the corresponding vector:
      if (fa.raw_frame_entries)
      {
        // we have something coming from the raw allocator, use this:
        const typename frame_entry_frame_alloc_t::allocator_state state = std::move(fa.raw_frame_entries.value());
        fa.raw_frame_entries.reset();

        for (uint32_t i = 0; i < state.size(); ++i)
        {
          if (frame_entry_t* it = state.get_entry<frame_entry_t>(i); it != nullptr)
          {
            if ((it->queue_mask & fa.completed_queues_mask) == it->queue_mask)
              entries_to_run.push_back(std::move(*it));
            else
              fa.remaining_frame_entries.push_back(std::move(*it));

            // NOTE: destructor will never be called, we must do it ourselves
            it->~frame_entry_t();
          }
        }
      }
      else
      {
        std::mtc_vector<frame_entry_t> remaining_entries;
        std::swap(fa.remaining_frame_entries, remaining_entries);
        const uint32_t entry_count = (uint32_t)remaining_entries.size();

        for (uint32_t i = 0; i < entry_count; ++i)
        {
          auto& it = remaining_entries[i];
          if ((it.queue_mask & fa.completed_queues_mask) == it.queue_mask)
            entries_to_run.push_back(std::move(it));
          else
            fa.remaining_frame_entries.push_back(std::move(it));
        }
      }
    }

    // run the functions:
    if (use_tasks)
    {
      [[likely]];
      constexpr size_t k_entry_per_dispatch = 8;
      threading::for_each(hctx.tm, hctx.tm.get_current_group(), entries_to_run, [](auto& it, size_t /*index*/)
      {
        TRACY_SCOPED_ZONE;
        it.function();
      }, k_entry_per_dispatch);
    }
    else
    {
      for (auto& it : entries_to_run)
      {
        it.function();
      }
    }
  }

  uint32_t deferred_fence_execution::queue_index(const vk::queue& q)
  {
    uint32_t prev_queue_count;
    {
      std::lock_guard _lg { spinlock_shared_adapter::adapt(queue_list_lock) };
      prev_queue_count = queue_count;

      for (uint32_t i = 0; i < prev_queue_count; ++i)
      {
        if (queues[i] == &q) return i;
      }
    }

    [[unlikely]];

    {
      std::lock_guard _lg { spinlock_exclusive_adapter::adapt(queue_list_lock) };
      if (prev_queue_count != queue_count)
      {
        for (uint32_t i = prev_queue_count; i < queue_count; ++i)
        {
          if (queues[i] == &q) return i;
        }
      }

      check::debug::n_assert(queue_count != k_max_queue_count, "deferred_fence_execution: queue_count reached its max ({}) yet there's still queues to add", k_max_queue_count);

      queues[queue_count] = &q;
      const uint32_t index = queue_count;
      queue_count += 1;
      return index;
    }
  }
}
