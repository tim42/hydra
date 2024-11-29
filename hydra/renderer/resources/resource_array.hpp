//
// created by : Timothée Feuillet
// date: 2024-3-10
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

#pragma once

#include <ntools/mt_check/deque.hpp>
#include <ntools/spinlock.hpp>

/// \brief internal-ish utilities
namespace neam::hydra::utilities
{
  enum class resource_array_entry_state_t : uint8_t
  {
    free    = 0,
    unused  = 1,
    in_use  = 2,
  };
  struct resource_array_entry_base_t
  {
    uint32_t prev = ~0u;
    uint32_t next = ~0u;
    uint64_t last_frame_with_usage = 0;

    resource_array_entry_state_t entry_state = resource_array_entry_state_t::free;
  };
  /// \note ElementType should inherit from resource_array_entry_base_t
  template<typename ElementType>
  struct resource_array
  {
    ~resource_array() { clear(); }

    static constexpr uint32_t k_invalid_index = ~0u;

    /// \brief Find a free (or unused entry).
    /// Remove that entry from the list it currently belongs to + revert its state to the default
    /// Return k_invalid_index if none found or if it's impossible to increase the array size
    uint32_t find_or_create_new_entry(uint32_t max_array_size, uint32_t size_increase_count, uint64_t evict_before_resize_frame_count);

    /// \brief Remove the entry from the unused list
    void remove_entry_from_unused_list(ElementType& entry);
    /// \brief Remove the entry from the unused list
    void remove_entry_from_unused_list_unlocked(ElementType& entry);
    /// \brief Add an entry to the unused list
    void add_entry_to_unused_list(ElementType& entry, uint32_t index);

    /// \brief Add an entry to the free list
    void add_entry_to_free_list(ElementType& entry, uint32_t index);
    /// \brief Add an entry to the free list
    void add_entry_to_free_list_unlocked(ElementType& entry, uint32_t index);

    /// \brief increment the frame and add unused entries to the unused list
    template<typename Func>
    void start_frame(Func&& func);

    /// \brief Call func on all unused entries, starting from the older to the newest.
    /// Signature: func(entry, index)
    /// \note Removing the _current_ entry from the unused list is supported, modifying the list in any other way isn't.
    /// \warning Needs both list_header_lock and entries_lock to be held
    template<typename Func>
    void for_each_unused_entries_unlocked(Func&& func);

    std::mtc_deque<ElementType> clear();

    // NOTE: please lock shared when reading from entries_lock
    mutable shared_spinlock entries_lock;
    std::mtc_deque<ElementType> entries;

    // NOTE: Can most likely be made without a lock and atomic spins, but I can't be bothered to do that.
    //       Only if it becomes a bottleneck.
    spinlock list_header_lock; // also protects the prev/next from the structs
    uint32_t first_free_entry = k_invalid_index;

    uint32_t first_unused_entry = k_invalid_index;
    uint32_t last_unused_entry = k_invalid_index;

    uint64_t frame_counter = 1;
  };
}

