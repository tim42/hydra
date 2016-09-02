//
// file : memory_allocator_chunk.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/memory_allocator_chunk.hpp
//
// created by : Timothée Feuillet
// date: Thu Sep 01 2016 13:06:00 GMT+0200 (CEST)
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

#ifndef __N_25333212431235330807_348316347_MEMORY_ALLOCATOR_CHUNK_HPP__
#define __N_25333212431235330807_348316347_MEMORY_ALLOCATOR_CHUNK_HPP__

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <bitset>

#include "../hydra_exception.hpp"
#include "../vulkan/device.hpp"
#include "../vulkan/device_memory.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief A chunk of memory, allocated on the device
    /// \note Not intended to be used directly. Instead please use a
    /// memory_allocator.
    /// \see class memory_allocator
    /// \note Reducing bitmap_entries may improve performance but will reduce the
    ///       granularity of the allocation (the default is to have a granularity of 512byte)
    class memory_allocator_chunk
    {
      public:
        constexpr static size_t chunk_allocation_size = 8 * 1024 * 1024; // must be a power of 2
        constexpr static size_t bitmap_entries = 256;                    // must be a power of 2

        constexpr static size_t second_level_entries = bitmap_entries * 2 / 64;
        constexpr static size_t first_level_bits = second_level_entries * 2;

        constexpr static size_t granularity = chunk_allocation_size / (bitmap_entries * 64);
        constexpr static size_t second_level_granularity = granularity * 32;
        constexpr static size_t first_level_granularity = second_level_granularity * 32;

        static_assert(first_level_bits <= 64, "second level entries can't fit in a 64bit integer");

      public:
        /// \brief Construct the allocator chunk.
        memory_allocator_chunk(vk::device &dev, size_t _memory_type_index)
          : dmem(vk::device_memory::allocate(dev, chunk_allocation_size, _memory_type_index)),
            memory_type_index(_memory_type_index)
        {
          memset(bitmap.data(), 0, bitmap_entries * 8);
          memset(second_level_bl.data(), 0, second_level_entries * 8);
          memset(second_level_wh.data(), 0, second_level_entries * 8);
        }
        ~memory_allocator_chunk() {} // free the chunk

        /// \brief Print information about the chunk
        static void print_nfo()
        {
          cr::out.log() << LOGGER_INFO_TPL("memory_allocator_chunk", __LINE__) 
                        << "chunk size: " << (chunk_allocation_size / (1024.f * 1024.f)) << " Mio with " << bitmap_entries << " bitmap entries" << cr::newline
                        << "first level bits: " << first_level_bits << ", second level entries: " << second_level_entries << " (" << second_level_entries * 64 << " bits)" << cr::newline
                        << "first_level_granularity: " << (first_level_granularity / 1024.f) << "Kio, second_level_granularity: " << (second_level_granularity / 1024.f) << "Kio" << cr::newline
                        << "granularity: " << granularity << " bytes" << std::endl;
        }

        /// \brief Print some stats about the current chunk
        void print_stats()
        {
          auto &x = cr::out.log() << LOGGER_INFO_TPL("memory_allocator_chunk", __LINE__) << "chunk@" << this << ":" << cr::newline
                   << "free memory:      " << (free_memory / (1024.f * 1024.f)) << " Mio" << cr::newline
                   << "1st lvl bl:       " << std::bitset<first_level_bits>(first_level_bl) << cr::newline
                   << "1st lvl wh:       " << std::bitset<first_level_bits>(first_level_wh) << cr::newline;

          for (size_t i = 0; i < second_level_entries; i += 2)
          {
            x << cr::newline;
            x << "2nd lvl bl[" << i+1 << '/' << i << "]:  " << std::bitset<64>(second_level_bl[1 + i]) << ' ' << std::bitset<64>(second_level_bl[0 + i]) << cr::newline;
            x << "2nd lvl wh[" << i+1 << '/' << i << "]:  " << std::bitset<64>(second_level_wh[1 + i]) << ' ' << std::bitset<64>(second_level_wh[0 + i]) << cr::newline;
          }

          x << " -- --" << std::endl;
        }

        /// \brief Check if the chunk is fully empty
        bool is_empty() const { return free_memory == chunk_allocation_size; }

        /// \brief Check if the chunk is fully full
        bool is_full() const { return !free_memory; }

        /// \brief Return the free memory (beware of memory fragmentation)
        size_t get_total_free_memory() const { return free_memory; }

        /// \brief A fast, with no false negative but with lots of false positive
        /// way to check if you can allocate some memory.
        /// \note Works better with big sizes
        bool can_allocate(size_t size)
        {
          constexpr uint64_t mask = ~0;

          // There's no way it can fit in memory
          if (size > free_memory)
            return false;

          // First level heuristics (512kio with default values)
          {
            uint64_t fl_mask = mask >> (64 - first_level_bits);

            // This is a fast one, as we have at least one white first level entry at 0,
            // so for things that are smaller or equal to the first_level_granularity
            // we have an empty slot for them !
            if (size <= first_level_granularity && ((first_level_wh & fl_mask) != fl_mask))
              return true;

            // Same thing, but this time with bigger requests (that would need a free
            // slot) but don't have it
            if (size > first_level_granularity && ((first_level_wh & fl_mask) == fl_mask))
              return false;
          }

          // Second level heuristics (16kio with default values)
          {
            bool maybe = false;
            // Probably something like 8 entries, so that is fast
            for (size_t i = 0; i < second_level_entries; ++i)
            {
              if (size > second_level_granularity && ((second_level_wh[i] & mask) != mask))
                  maybe = true;
              else if (size <= second_level_granularity && ((second_level_wh[i] & mask) != mask))
                return true;
            }
            if (size > second_level_granularity)
              return maybe;
          }

          // In doubt, return true
          return true;
        }

        /// \brief Return a const reference to the device memory
        const vk::device_memory &get_device_memory() const { return dmem; }

        /// \brief Allocate something from the memory chunk. It either return -1
        /// If the allocation failed or the offset within the memory.
        /// The returned offset is aligned to granularity (default, 512byte)
        /// \note bigger allocation are faster (the fastest is for size > 2 * first_level_granularity)
        /// \note the algorythm used is a kind of first-fit. (beware the fragmentation)
        int allocate(size_t size)
        {
          check::on_vulkan_error::n_assert(size <= chunk_allocation_size, "m_a_c::allocate(): size is greater than the chunk");

          if (size > free_memory || size == 0) // fast exit
            return -1;

          // re-align size
          if (size % granularity) size += granularity - (size % granularity);

          int offset = 0; // base offset

#ifndef HYDRA_ALLOCATOR_FAST_FREE
          // super-fast search (using black levels) //
          if (size >= second_level_granularity * 2)
          {
            cr::out.log() << LOGGER_INFO_TPL("allocate", __LINE__) << "using fast free space search" << std::endl;
            offset = fast_free_space_search(0, size);
            if (offset != -1) // found !
              mark_space_as_used(offset, size);
            return offset;
          }
#endif
          // fastidious search
          cr::out.log() << LOGGER_INFO_TPL("allocate", __LINE__) << "using fast forward" << std::endl;
          while (offset + size < chunk_allocation_size)
          {
            const int old_offset = offset;
            offset = fast_forward(offset); // skip used contents
            cr::out.log() << LOGGER_INFO_TPL("allocate", __LINE__) << "ff: " << offset << " vs " << old_offset << std::endl;
            if (offset + size > chunk_allocation_size)
              return -1; // there's isn't anything we can do :/
            size_t avail_free_space = get_free_space_at_offset(offset, size);
            cr::out.log() << LOGGER_INFO_TPL("allocate", __LINE__) << "free space: " << avail_free_space << std::endl;
            if (avail_free_space >= size)
              break;
            offset += avail_free_space;
          }

          cr::out.log() << LOGGER_INFO_TPL("allocate", __LINE__) << "offset: " << offset << std::endl;
          mark_space_as_used(offset, size);
          return offset;
        }

        /// \brief Free the memory at offset offset and size size.
        /// \note May throw if something bad is happening
        void free(size_t offset, size_t size)
        {
          // re-align size
          if (size % granularity) size += granularity - (size % granularity);

          check::on_vulkan_error::n_assert(offset + size <= chunk_allocation_size, "m_a_c::free(): invalid free (offset + size bigger than total memory)");

#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
          check::on_vulkan_error::n_assert((offset % granularity) == 0, "m_a_c::free(): invalid free (invalid offset)");
#endif

          constexpr uint64_t mask = ~0;

          unsigned bm_index = offset / (granularity * 64);
          const unsigned bit_index = (offset / granularity) % 64;
          unsigned bit_count = size / granularity;

          if (bit_index != 0) // the first bits
          {
            unsigned cnt = std::min(64 - bit_index, bit_count);
            uint64_t tmask = (mask >> (64 - (bit_index + cnt))) & (mask << bit_index);

#ifdef HYDRA_ENABLE_DEBUG_CHECKS
            check::on_vulkan_error::n_assert((bitmap[bm_index] & tmask) == tmask, "m_a_c::free(): double free (freeing already freed memory)");
#endif
            bitmap[bm_index] &= ~tmask;
            update_hierarchy(bm_index);

            ++bm_index;
            bit_count -= cnt;
          }

          // The fast loop
          for (; bit_count >= 64 && bm_index < bitmap_entries; bit_count -= 64, ++bm_index)
          {
#ifdef HYDRA_ENABLE_DEBUG_CHECKS
            check::on_vulkan_error::n_assert(bitmap[bm_index] == mask, "m_a_c::free(): double free (freeing already freed memory)");
#endif
            bitmap[bm_index] = 0; // clean the bitmap

            update_hierarchy(bm_index);
          }

#ifdef HYDRA_ENABLE_DEBUG_CHECKS
          check::on_vulkan_error::n_assert(bm_index >= bitmap_entries && (bit_count > 0), "m_a_c::free(): invalid free (size out of bounds)");
#endif
          if (bit_count > 0 && bm_index < bitmap_entries) // the last bits
          {
            uint64_t tmask = mask >> (64 - bit_count);
#ifdef HYDRA_ENABLE_DEBUG_CHECKS
            check::on_vulkan_error::n_assert((bitmap[bm_index] & tmask) == tmask, "m_a_c::free(): double free (freeing already freed memory)");
#endif
            bitmap[bm_index] &= ~tmask;
            update_hierarchy(bm_index);
          }

          // update stats
          free_memory += size;
        }

        /// \brief Faster than allocation, it grow (if possible) a previously
        /// allocated area.
        /// Return true in case of success, false if that's not possible
//         bool grow_memory(size_t offset, size_t size, size_t additional_size)
//         {
//           return false;
//         }

      private:
        /// \brief Skip already allocated memory
        /// If not disabled, it has the ability to perform 512kio (1024 bitmap entries) and 16kio (32 bitmap entries) jumps
        size_t fast_forward(size_t offset)
        {
#ifndef HYDRA_ALLOCATOR_FAST_FREE
          // fast discard (using the white levels -> if a white level bit is set to 1, the whole area is used) //

          bool from_first_level = false;
          bool from_second_level = false;

          // first level (512kio discard)
          if (offset % first_level_granularity == 0)
          {
          _first_level_search:
            from_first_level = true;
            cr::out.log() << LOGGER_INFO_TPL(__PRETTY_FUNCTION__, __LINE__) << "using fast forward / first level" << std::endl;
            unsigned fl_bidx = offset / first_level_granularity;
            for (; fl_bidx < first_level_bits && (((first_level_wh >> fl_bidx) & 1) != 0); ++fl_bidx, offset += first_level_granularity)
            {}
          }

          // second level (16kio discard)
          if (offset % second_level_granularity == 0)
          {
          _second_level_search:
            from_second_level = true;
            cr::out.log() << LOGGER_INFO_TPL(__PRETTY_FUNCTION__, __LINE__) << "using fast forward / second level" << std::endl;
            unsigned sl_idx = offset / (second_level_granularity * 64);
            unsigned sl_bidx = 0;
            uint64_t tmp = second_level_wh[sl_idx];
            for (; ((tmp >> sl_bidx) & 1) != 0; sl_bidx = (sl_bidx + 1) % 64, offset += second_level_granularity)
            {
              if (offset % first_level_granularity == 0 && !from_first_level) goto _first_level_search;
              from_first_level = false;
              if (sl_bidx == 63)
              {
                if (sl_idx + 1 == second_level_entries)
                  break;
                tmp = second_level_wh[++sl_idx];
              }
            }
          }

          // slower discard (bitmap-based, 512o) //
          {
            cr::out.log() << LOGGER_INFO_TPL(__PRETTY_FUNCTION__, __LINE__) << "using fast forward / bitmap" << std::endl;
            unsigned bm_idx = offset / (granularity * 64);
            unsigned bm_bidx = (offset / granularity) % 64;
            uint64_t tmp = bitmap[bm_idx];
            for (; ((tmp >> bm_bidx) & 1) != 0; bm_bidx = (bm_bidx + 1) % 64, offset += granularity)
            {
              if (offset % first_level_granularity == 0 && !from_first_level) goto _first_level_search;
              if (offset % second_level_granularity == 0 && !from_second_level) goto _second_level_search;
              from_first_level = false;
              from_second_level = false;
              if (bm_bidx == 63)
              {
                if (bm_idx + 1 == bitmap_entries)
                  break;
                tmp = bitmap[++bm_idx];
              }
            }
          }
#else // FAST_FREE / SLOW_ALLOC
          // slower discard (bitmap-based, 512o) //
          {
            unsigned bm_idx = offset / (granularity * 64);
            unsigned bm_bidx = (offset / granularity) % 64;
            uint64_t tmp = bitmap[bm_idx];
            for (; ((tmp >> bm_bidx) & 1) != 0; bm_bdix = (bm_bdix + 1) % 64, offset += granularity)
            {
              if (bm_bidx == 63)
              {
                if (bm_idx + 1 == bitmap_entries)
                  break;
                tmp = bitmap_entries[++bm_idx];
              }
            }
          }
#endif
          return offset;
        }

        /// \brief Return how many free space there is
        /// If not disabled, it has the ability to perform 512kio (1024 bitmap entries) and 16kio (32 bitmap entries) jumps
        size_t get_free_space_at_offset(size_t offset, size_t size)
        {
          const size_t orig_offset = offset;
#ifndef HYDRA_ALLOCATOR_FAST_FREE
          // fast count (using the black levels (if a black level is set to 0, the whole area is free) //

          // first level (512kio discard)
          if (offset % first_level_granularity == 0)
          {
          _first_level_search:
            unsigned fl_bidx = offset / first_level_granularity;
            for (; fl_bidx < first_level_bits && (((first_level_bl >> fl_bidx) & 1) == 0); ++fl_bidx, offset += first_level_granularity)
            {}
            if (offset - orig_offset > size)
              return offset - orig_offset;
          }

          // second level (16kio discard)
          if (offset % second_level_granularity == 0)
          {
          _second_level_search:
            unsigned sl_idx = offset / (second_level_granularity * 64);
            unsigned sl_bidx = (offset / second_level_granularity) % 64;
            uint64_t tmp = second_level_bl[sl_idx];
            for (; ((tmp >> sl_bidx) & 1) == 0; sl_bidx = (sl_bidx + 1) % 64, offset += second_level_granularity)
            {
              if (offset - orig_offset > size)
                return offset - orig_offset;
              if (offset % first_level_granularity == 0) goto _first_level_search;
              if (sl_bidx == 63)
              {
                if (sl_idx + 1 == second_level_entries)
                  break;
                tmp = second_level_bl[++sl_idx];
              }
            }
          }

          // slower discard (bitmap-based, 512o) //
          {
            unsigned bm_idx = offset / (granularity * 64);
            unsigned bm_bidx = (offset / granularity) % 64;
            uint64_t tmp = bitmap[bm_idx];
            for (; ((tmp >> bm_bidx) & 1) == 0; bm_bidx = (bm_bidx + 1) % 64, offset += granularity)
            {
              if (offset % first_level_granularity == 0) goto _first_level_search;
              if (offset % second_level_granularity == 0) goto _second_level_search;
              if (bm_bidx == 63)
              {
                if (offset - orig_offset > size)
                  break;
                if (bm_idx + 1 == bitmap_entries)
                  break;
                tmp = bitmap[++bm_idx];
              }
            }
          }

          return offset - orig_offset;
#else // FAST_FREE / SLOW_ALLOC
          // slower discard (bitmap-based, 512o) //
          {
            unsigned bm_idx = offset / (granularity * 64);
            unsigned bm_bidx = (offset / granularity) % 64;
            uint64_t tmp = bitmap[bm_idx];
            for (; ((tmp >> bm_bidx) & 1) == 0; bm_bdix = (bm_bdix + 1) % 64, offset += granularity)
            {
              if (offset - orig_offset > size)
                break;
              if (bm_bidx == 63)
              {
                if (bm_idx + 1 == bitmap_entries)
                  break;
                tmp = bitmap_entries[++bm_idx];
              }
            }

            return offset - orig_offset;
          }
#endif
          return offset;
        }

        /// \brief Used to find big chunks of free space (> 32kio)
        /// \note Disabled if using the fast-free/slow-alloc switch
        long fast_free_space_search(size_t offset, size_t size)
        {
          size_t found_sz = 0;
          // super-fast search (using black levels) //
          if (size >= first_level_granularity * 2)
          {
            // We should find at least bit_count unmarked bit
            unsigned bit_count = size / (first_level_granularity * 2) - 1;
            unsigned fl_bidx = offset / first_level_granularity;
            if (offset % first_level_granularity != 0)
              ++fl_bidx;

            while (fl_bidx < first_level_bits)
            {
              for (; fl_bidx < first_level_bits && (((first_level_bl >> fl_bidx) & 1) != 0); ++fl_bidx, offset += first_level_granularity);
              unsigned count = 0;
              for (; fl_bidx < first_level_bits && (((first_level_bl >> fl_bidx) & 1) == 0) && count < bit_count; ++fl_bidx, ++count);

              if (count == bit_count) // found
                break;
              offset += count * first_level_granularity; // not found
            }
            if (fl_bidx >= first_level_bits)
              return -1; // nothing can be found, and I can guarantee that there's not enough free space for it
            found_sz = bit_count * first_level_granularity;
          }
          else if (size >= second_level_granularity)
          {
            // We should find at least bit_count unmarked bit
            unsigned bit_count = size / (second_level_granularity * 2) - 1;
            unsigned sl_idx = (offset / second_level_granularity) / 64;
            unsigned sl_bidx = (offset / second_level_granularity) % 64;
            if (offset % second_level_granularity != 0)
            {
              ++sl_bidx;
              if (sl_bidx == 64)
              {
                sl_bidx = 0;
                ++sl_idx;
              }
            }

            while (sl_idx < second_level_entries)
            {
              for (; sl_idx < second_level_entries && (((second_level_bl[sl_idx] >> sl_bidx) & 1) != 0); sl_bidx = (sl_bidx + 1) % 64, offset += second_level_granularity)
              {
                if (sl_bidx == 63) ++sl_idx;
              }
              unsigned count = 0;
              for (; sl_idx < second_level_entries && (((second_level_bl[sl_idx] >> sl_bidx) & 1) == 0) && count < bit_count; sl_bidx = (sl_bidx + 1) % 64, ++count)
              {
                if (sl_bidx == 63) ++sl_idx;
              }

              if (count == bit_count) // not found
                break;
              offset += count * second_level_granularity; // found
            }
            if (sl_idx >= second_level_entries)
              return -1; // nothing can be found, and I can guarantee that there's not enough free space for it
            found_sz = bit_count * second_level_granularity;
          }
          else
            return -1; /* :( function not used correctly */

          // reverse search (reduces fragmentation)
          unsigned bm_idx = offset / (granularity * 64);
          unsigned bm_bidx = (offset / granularity) % 64;
          uint64_t tmp = bitmap[bm_idx];

          for (; ((tmp >> bm_bidx) & 1) == 0; --bm_bidx, offset -= granularity, found_sz += granularity)
          {
            if (bm_bidx == 0)
            {
              if (bm_idx == 0)
                break;
              tmp = bitmap[--bm_idx];
              bm_bidx = 64;
            }
          }

          // forward search
          bm_idx = (offset + found_sz) / (granularity * 64);
          bm_bidx = ((offset + found_sz) / granularity) % 64;
          tmp = bitmap[bm_idx];

          for (; ((tmp >> bm_bidx) & 1) == 0 && found_sz < size; bm_bidx = (bm_bidx + 1) % 64, found_sz += granularity)
          {
            if (bm_bidx == 63)
            {
              if (bm_idx == bitmap_entries)
                break;
              tmp = bitmap[++bm_idx];
            }
          }

          if (found_sz >= size) // Yay ! found something
            return offset;

          // Not enough free space ! (tail recursion !)
          return fast_free_space_search(offset + found_sz, size);
        }

        /// \brief Update the bitmap and its herarchy to tell the space has been allocated
        void mark_space_as_used(size_t offset, size_t size)
        {
          constexpr uint64_t mask = ~0;

          unsigned bm_index = offset / (granularity * 64);
          const unsigned bit_index = (offset / granularity) % 64;
          unsigned bit_count = size / granularity;

          if (bit_index != 0) // the first bits
          {
            unsigned cnt = std::min(64 - bit_index, bit_count);
            uint64_t tmask = (mask >> (64 - (bit_index + cnt))) & (mask << bit_index);

            bitmap[bm_index] |= tmask;
            update_hierarchy(bm_index);

            ++bm_index;
            bit_count -= cnt;
          }

          // The fast loop
          for (; bit_count >= 64 && bm_index < bitmap_entries; bit_count -= 64, ++bm_index)
          {
            bitmap[bm_index] = mask;
            update_hierarchy(bm_index);
          }

          if (bit_count > 0 && bm_index < bitmap_entries) // the last bits
          {
            uint64_t tmask = mask >> (64 - bit_count);
            bitmap[bm_index] |= tmask;
            update_hierarchy(bm_index);
          }

          // update stats
          free_memory -= size;
        }

        /// \brief Update the bitmap hierarchy (only used if HYDRA_ALLOCATOR_FAST_FREE is not defined)
        void update_hierarchy(unsigned bm_index)
        {
#ifndef HYDRA_ALLOCATOR_FAST_FREE
          uint64_t tmask;
          const unsigned second_level_index = (bm_index * 2) / 64;
          const unsigned second_level_bit = (bm_index * 2) % 64;

          tmask = (((bitmap[bm_index] >> 32) == 0xFFFFFFFF) ? 2 : 0)
                  | (((bitmap[bm_index] & 0xFFFFFFFF) == 0xFFFFFFFF) ? 1 : 0);
          tmask <<= second_level_bit;
          second_level_wh[second_level_index] = (second_level_wh[second_level_index] & ~(3ul << second_level_bit)) | tmask; // update the second level

          tmask = (((bitmap[bm_index] >> 32) != 0) ? 2 : 0)
                  | (((bitmap[bm_index] & 0xFFFFFFFF) != 0) ? 1 : 0);
          tmask <<= second_level_bit;
          second_level_bl[second_level_index] = (second_level_bl[second_level_index] & ~(3ul << second_level_bit)) | tmask; // update the second level


          const unsigned first_level_bit = (second_level_index * 2);
          tmask = (((second_level_wh[second_level_index] >> 32) == 0xFFFFFFFF) ? 2 : 0)
                  | (((second_level_wh[second_level_index] & 0xFFFFFFFF) == 0xFFFFFFFF) ? 1 : 0);
          tmask <<= first_level_bit;
          first_level_wh = (first_level_wh & ~(3ul << first_level_bit)) | tmask;

          tmask = (((second_level_bl[second_level_index] >> 32) != 0) ? 2 : 0)
                  | (((second_level_bl[second_level_index] & 0xFFFFFFFF) != 0) ? 1 : 0);
          tmask <<= first_level_bit;
          first_level_bl = (first_level_bl & ~(3ul << first_level_bit)) | tmask;
#endif
        }

      private:
        // the memory //
        vk::device_memory dmem;
        size_t memory_type_index;

        size_t free_memory = chunk_allocation_size;

        // the hierarchical bitmap //
        uint64_t first_level_bl = 0; // bit set to 1 as soon as there's an allocation
        uint64_t first_level_wh = 0; // bit set to 1 when the 32 entries are allocated
        std::array<uint64_t, second_level_entries> second_level_bl = {{0}}; // bit set to 1 as soon as there's an allocation
        std::array<uint64_t, second_level_entries> second_level_wh = {{0}}; // bit set to 1 when the 32 entries are allocated
        std::array<uint64_t, bitmap_entries> bitmap = {{0}};
    };
  } // namespace hydra
} // namespace neam


#endif // __N_25333212431235330807_348316347_MEMORY_ALLOCATOR_CHUNK_HPP__

