
//
// created by : Timothée Feuillet
// date: 2024-3-11
//
//
// Copyright (c) 2024 Timothée Feuillet
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

// NOTE: TEMPLATE IMPLEM FILE, Only include in the CPP files that require the template code.

#pragma once

#include "resource_array.hpp"

namespace neam::hydra::utilities
{
  template<typename ElementType>
  uint32_t resource_array<ElementType>::find_or_create_new_entry(uint32_t max_array_size, uint32_t size_increase_count, uint64_t evict_before_resize_frame_count)
  {
    uint32_t index = k_invalid_index;

    std::lock_guard _l {spinlock_shared_adapter::adapt(entries_lock)};
      std::lock_guard _lh(list_header_lock);

    // free-list:
    if (first_free_entry != k_invalid_index)
    {
      if (first_free_entry != k_invalid_index) [[likely]]
      {
        index = first_free_entry;
        first_free_entry = entries[index].next;

        entries[index].next = k_invalid_index;
        entries[index].prev = k_invalid_index;
        entries[index].entry_state = resource_array_entry_state_t::in_use;
        return index;
      }
    }
    // unused list:
    if (first_unused_entry != k_invalid_index)
    {
      // std::lock_guard _l(list_header_lock);
      if (first_unused_entry != k_invalid_index) [[likely]]
      {
        // above the threshold for preferring eviction to array resize:
        const uint64_t unused_frame_count = frame_counter - entries[first_unused_entry].last_frame_with_usage;
        if ((unused_frame_count > evict_before_resize_frame_count) || (entries.size() >= max_array_size && unused_frame_count > 2))
        {
          index = first_unused_entry;
          remove_entry_from_unused_list_unlocked(entries[index]);
          return index;
        }
      }
    }
    if (entries.size() < max_array_size)
    {
      {
        std::lock_guard _l {spinlock_shared_to_exclusive_adapter::adapt(entries_lock)};
        // std::lock_guard _lh(list_header_lock);
        if (first_free_entry == k_invalid_index) // check that no-one added entries in the list
        {
          // resize the arrays:
          index = (uint32_t)entries.size();
          entries.resize(entries.size() + size_increase_count);

          // add the new entries to the unused list:
          first_free_entry = index + 1;
          for (uint32_t i = index + 1; i < (uint32_t)entries.size(); ++i)
          {
            if ((i + 1) < (uint32_t)entries.size())
              entries[i].next = i + 1;
            else
              entries[i].next = k_invalid_index;

            entries[i].prev = k_invalid_index;
            entries[i].entry_state = resource_array_entry_state_t::free;
          }

          entries[index].next = k_invalid_index;
          entries[index].prev = k_invalid_index;
          entries[index].entry_state = resource_array_entry_state_t::in_use;
          return index;
        }

        // We got a race and someone actually added some entries instead of us
        {
          index = first_free_entry;
          first_free_entry = entries[index].next;

          entries[index].next = k_invalid_index;
          entries[index].prev = k_invalid_index;
          entries[index].entry_state = resource_array_entry_state_t::in_use;
          return index;
        }
      }
    }

    // Failed to find any space for the texture
    return k_invalid_index;
  }

  template<typename ElementType>
  void resource_array<ElementType>::remove_entry_from_unused_list(ElementType& entry)
  {
    std::lock_guard _l(list_header_lock);
    remove_entry_from_unused_list_unlocked(entry);
  }

  template<typename ElementType>
  void resource_array<ElementType>::remove_entry_from_unused_list_unlocked(ElementType& entry)
  {
    if (entry.prev != k_invalid_index)
      entries[entry.prev].next = entry.next;
    else
      first_unused_entry = entry.next;

    if (entry.next != k_invalid_index)
      entries[entry.next].prev = entry.prev;
    else
      last_unused_entry = entry.prev;

    entry.prev = k_invalid_index;
    entry.next = k_invalid_index;
    entry.entry_state = resource_array_entry_state_t::in_use;
  }

  template<typename ElementType>
  void resource_array<ElementType>::add_entry_to_unused_list(ElementType& entry, uint32_t index)
  {
    std::lock_guard _l(list_header_lock);

    entry.prev = last_unused_entry;
    entry.next = k_invalid_index;
    if (last_unused_entry == k_invalid_index)
      first_unused_entry = index;
    else
      entries[last_unused_entry].next = index;
    last_unused_entry = index;
    entry.entry_state = resource_array_entry_state_t::unused;
  }

  template<typename ElementType>
  void resource_array<ElementType>::add_entry_to_free_list(ElementType& entry, uint32_t index)
  {
    std::lock_guard _l(list_header_lock);

    add_entry_to_free_list_unlocked(entry, index);
  }

  template<typename ElementType>
  void resource_array<ElementType>::add_entry_to_free_list_unlocked(ElementType& entry, uint32_t index)
  {
    entry.next = first_free_entry;
    entry.prev = k_invalid_index;
    first_free_entry = index;
    entry.entry_state = resource_array_entry_state_t::free;
  }

  template<typename ElementType>
  template<typename Func>
  void resource_array<ElementType>::for_each_unused_entries_unlocked(Func&& func)
  {
    uint32_t next = k_invalid_index;
    for (uint32_t i = first_unused_entry; i != k_invalid_index; i = next)
    {
      next = entries[i].next;
      func(entries[i], i);
    }
  }

  template<typename ElementType>
  template<typename Func>
  void resource_array<ElementType>::start_frame(Func&& func)
  {
    const uint64_t original_frame_counter = frame_counter;
    frame_counter += 1;

    std::lock_guard _l {spinlock_shared_adapter::adapt(entries_lock)};
    const uint32_t size = (uint32_t)entries.size();
    // go over all the entries to check if they were in use last frame or if the need to be added to the unused list
    for (uint32_t i = 0; i < size; ++i)
    {
      auto& it = entries[i];
      if (it.entry_state == resource_array_entry_state_t::free)
        continue;

      if (it.last_frame_with_usage >= original_frame_counter) [[likely]] // was used recently
      {
        if (it.entry_state == resource_array_entry_state_t::unused)
          remove_entry_from_unused_list(it);

        func(it, i);

        continue;
      }

      if (it.entry_state != resource_array_entry_state_t::in_use)
        continue;

      add_entry_to_unused_list(it, i);
    }
  }

  template<typename ElementType>
  std::mtc_deque<ElementType> resource_array<ElementType>::clear()
  {
    std::lock_guard _le {spinlock_exclusive_adapter::adapt(entries_lock)};
    std::lock_guard _lh(list_header_lock);
    std::mtc_deque<ElementType> tmp = std::move(entries);

    first_free_entry = k_invalid_index;
    first_unused_entry = k_invalid_index;
    last_unused_entry = k_invalid_index;
    frame_counter = 1;

    return tmp;
  }
}
