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

#pragma once


#include <cstdint>
#include <cstddef>
#include <cstring>
#include <bitset>

#include "../hydra_debug.hpp"
#include "../vulkan/device.hpp"
#include "../vulkan/device_memory.hpp"

// #define HYDRA_ALLOCATOR_FAST_FREE

#if 0
namespace neam
{
  namespace hydra
  {
    /// \brief Dumb but fast allocator.
    /// The chunk has two modes:
    ///  - When it gets created, it first start as a 'append' allocator, where new allocation are placed right after the last allocation
    ///  - If there's not enough free space, it then goes on a 'please destroy me' mode if the chain is transient (refusing new allocations). This keep transient allocation as fast as possible.
    ///  - If the chain is not transient, then
    /// \note Expect fragmentation.
    template<unsigned ChunkAllocationSize = 32 * 1024 * 1024>
    class memory_allocator_chunk
    {
      private:
        unsigned align(unsigned alignment, unsigned value)
        {
          // Magic from glibc++
          return (value - 1u + alignment) & -alignment;
        }

      public:
        constexpr static unsigned chunk_allocation_size = ChunkAllocationSize;       // must be a power of 2
        constexpr static unsigned invalid_allocation = ~0u;

        /// \brief Should be set before any allocation has been performed. Will disable calls to free and memory tracking.
        /// If set to true, he resulting allocator will be extremly fast but will expect elements to be very short lived as the way
        /// it has to avoid fragmentation it to free the whole block. (only fast allocation will work).
        bool fast_allocation_only = true;

        /// \brief Return the free memory (beware of memory fragmentation)
        unsigned get_total_free_memory() const { return free_space; }

        /// \brief A fast check to see if the chunk can perform a fast allocation of said size+alignment
        /// \note Is as slow as doing the fast allocation. So it may preferable to simply call fast_allocate instead.
        bool can_fast_allocate(unsigned size, unsigned alignment) const
        {
          const unsigned aligned_offset = align(alignment, fast_allocation_offset);
          if (aligned_offset + size <= chunk_allocation_size)
            return true;
          return false;
        }

        /// \brief Try to fast allocate an element of size+alignment
        /// \return invalid_allocation on failure, otherwise return the offset within the chunk.
        /// \note It will not try to perform a slow allocation
        unsigned fast_allocate(unsigned size, unsigned alignment)
        {
          const unsigned aligned_offset = align(alignment, fast_allocation_offset);
          if (aligned_offset + size > chunk_allocation_size)
            return invalid_allocation;
          free_space -= (aligned_offset + size) - fast_allocation_offset;
          fast_allocation_offset = aligned_offset + size;
          ++allocation_count;
          return aligned_offset;
        }


        unsigned allocate(unsigned size, unsigned alignment)
        {
          if (size > free_space)
            return invalid_allocation;

          // deals with the fast allocation first:
          {
            const unsigned alloc = fast_allocate(size, alignment);
            if (alloc != invalid_allocation || fast_allocation_only)
              return alloc;
          }

          // slow allocation
          return invalid_allocation;
        }

        void free(size_t offset, size_t size)
        {
#ifndef HYDRA_DISABLE_OPTIONAL_CHECKS
          check::on_vulkan_error::n_assert(allocation_count > 0, "m_a_c::free(): invalid free (no allocation remaining)");
#endif

          --allocation_count;
          if (!allocation_count)
          {
            fast_allocation_offset = 0;
            free_space = chunk_allocation_size;
          }

          // fast allocation does not supports deallocation.
          if (fast_allocation_only)
            return;
        }

        bool is_empty() const { return allocation_count == 0; }

      private:
        unsigned fast_allocation_offset = 0;
        unsigned free_space = chunk_allocation_size;

        unsigned allocation_count = 0;
    };
    
    
    
    
    
    
    
    
    
    /// \brief Manage a chunk of memory (it does not perform anything by itself,
    /// if you plan to use it, it is just a tool to manage some memory)
    /// \note Not intended to be used directly. Instead please use a
    /// memory_allocator.
    /// \see class memory_allocator
    /// \note Reducing bitmap_entries may improve performance but will reduce the
    ///       granularity of the allocation (the default is to have a granularity of 512byte)
    template<size_t ChunkAllocationSize = 16 * 1024 * 1024, size_t BitmapEntries = 256>
    class __memory_allocator_chunk
    {
      public:
        constexpr static size_t chunk_allocation_size = ChunkAllocationSize;       // must be a power of 2
        constexpr static size_t bitmap_entries = BitmapEntries;                    // must be a power of 2

        constexpr static size_t second_level_entries = bitmap_entries * 2 / 64;
        constexpr static size_t first_level_bits = second_level_entries * 2;

        constexpr static size_t granularity = chunk_allocation_size / (bitmap_entries * 64);
        constexpr static size_t second_level_granularity = granularity * 32;
        constexpr static size_t first_level_granularity = second_level_granularity * 32;

        static_assert(first_level_bits <= 64, "second level entries can't fit in a 64bit integer");

      public:
        /// \brief Print information about the chunk
        static void print_nfo()
        {
//           cr::get_global_logger().log() << "chunk size: " << (chunk_allocation_size / (1024.f * 1024.f)) << " Mio with " << bitmap_entries << " bitmap entries" << cr::newline
//                         << "first level bits: " << first_level_bits << ", second level entries: " << second_level_entries << " (" << second_level_entries * 64 << " bits)" << cr::newline
//                         << "first_level_granularity: " << (first_level_granularity / 1024.f) << "Kio, second_level_granularity: " << (second_level_granularity / 1024.f) << "Kio" << cr::newline
//                         << "granularity: " << granularity << " bytes" << std::endl;
        }

        /// \brief Print some stats about the current chunk
        void print_stats()
        {
//           auto &x = cr::get_global_logger().log() << "chunk@" << this << ":" << cr::newline
//                    << "free memory:      " << (free_memory / (1024.f * 1024.f)) << " Mio" << cr::newline
//                    << "1st lvl bl:       " << std::bitset<first_level_bits>(first_level_bl) << cr::newline
//                    << "1st lvl wh:       " << std::bitset<first_level_bits>(first_level_wh) << cr::newline;
// 
//           for (size_t i = 0; i < second_level_entries; i += 2)
//           {
//             x << cr::newline;
//             x << "2nd lvl bl[" << i+1 << '/' << i << "]:  " << std::bitset<64>(second_level_bl[1 + i]) << ' ' << std::bitset<64>(second_level_bl[0 + i]) << cr::newline;
//             x << "2nd lvl wh[" << i+1 << '/' << i << "]:  " << std::bitset<64>(second_level_wh[1 + i]) << ' ' << std::bitset<64>(second_level_wh[0 + i]) << cr::newline;
//           }
// 
//           x << " -- --" << std::endl;
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
#ifndef HYDRA_ALLOCATOR_FAST_FREE
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
#endif
          // In doubt, return true
          return true;
        }

        /// \brief Allocate something from the memory chunk. It either return -1
        /// If the allocation failed or the offset within the memory.
        /// The returned offset is aligned to granularity (default, 512byte)
        /// \note bigger allocation are faster (the fastest is for size > 2 * first_level_granularity)
        /// \note the algorythm used is a kind of first-fit. (beware the fragmentation)
        int allocate(size_t size, size_t alignment)
        {
          check::on_vulkan_error::n_assert(size <= chunk_allocation_size, "m_a_c::allocate(): size is greater than the chunk");

          // re-align size
          if (size % granularity) size += granularity - (size % granularity);

          if (size > free_memory || size == 0) // fast exit
            return -1;

          int offset = 0; // base offset
          int aoffset = 0; // aligned offset

          size_t asize = size;

#ifndef HYDRA_ALLOCATOR_FAST_FREE
          // super-fast search (using black levels) //
          if (size >= second_level_granularity * 2)
          {
            aoffset = fast_free_space_search(0, size, alignment);
            if (aoffset == -1) // not found !
              return -1;

            offset = aoffset - aoffset % granularity;
            asize = size + (aoffset - offset);
            if (asize % granularity) size += granularity - (asize % granularity);
            mark_space_as_used(offset, asize);
            return aoffset;
          }
#endif
          // fastidious search
          while (offset + size < chunk_allocation_size)
          {
            offset = fast_forward(offset); // skip used contents
            aoffset = offset;
            asize = size;
            // align offset
            if (offset % alignment != 0)
            {
              aoffset += alignment - (offset % alignment);
              offset = aoffset - (aoffset % granularity);
              asize += aoffset - offset;
              if (asize % granularity) asize += granularity - (asize % granularity);
            }
            if (offset + asize > chunk_allocation_size)
              return -1; // there's isn't anything we can do :/
            size_t avail_free_space = get_free_space_at_offset(offset, asize);
            if (avail_free_space >= asize)
              break;
            offset += avail_free_space;
          }

          if (offset + asize > chunk_allocation_size)
            return -1; // there's isn't anything we can do :/

          mark_space_as_used(offset, asize);
          return aoffset;
        }

        /// \brief Free the memory at offset offset and size size.
        /// \note May throw if something bad is happening
        void free(size_t offset, size_t size)
        {
          // re-align size
          if (size % granularity) size += granularity - (size % granularity);
          offset = offset - (offset % granularity);

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

          if (offset >= chunk_allocation_size)
            return chunk_allocation_size;

          // first level (512kio discard)
          if (offset % first_level_granularity == 0)
          {
          _first_level_search:
            if (offset >= chunk_allocation_size)
              return chunk_allocation_size;
            from_first_level = true;
            unsigned fl_bidx = offset / first_level_granularity;
            for (; fl_bidx < first_level_bits && (((first_level_wh >> fl_bidx) & 1) != 0); ++fl_bidx, offset += first_level_granularity)
            {}
          }

          // second level (16kio discard)
          if (offset % second_level_granularity == 0)
          {
          _second_level_search:
            if (offset >= chunk_allocation_size)
              return chunk_allocation_size;

            from_second_level = true;
            unsigned sl_idx = offset / (second_level_granularity * 64);
            unsigned sl_bidx = 0;
            uint64_t tmp = second_level_wh[sl_idx];
            for (; ((tmp >> sl_bidx) & 1) != 0; sl_bidx = (sl_bidx + 1) % 64)
            {
              offset += second_level_granularity;
              from_first_level = false;
              if (sl_bidx == 63)
              {
                if (sl_idx + 1 == second_level_entries)
                  break;
                tmp = second_level_wh[++sl_idx];
              }
              if (offset % first_level_granularity == 0 && !from_first_level) goto _first_level_search;
            }
          }

          // slower discard (bitmap-based, 512o) //
          {
            if (offset >= chunk_allocation_size)
              return chunk_allocation_size;

            unsigned bm_idx = offset / (granularity * 64);
            unsigned bm_bidx = (offset / granularity) % 64;
            uint64_t tmp = bitmap[bm_idx];
            for (; ((tmp >> bm_bidx) & 1) != 0; bm_bidx = (bm_bidx + 1) % 64)
            {
              offset += granularity;
              from_first_level = false;
              from_second_level = false;
              if (bm_bidx == 63)
              {
                if (bm_idx + 1 == bitmap_entries)
                  break;
                tmp = bitmap[++bm_idx];
              }
              if (offset % first_level_granularity == 0 && !from_first_level) goto _first_level_search;
              if (offset % second_level_granularity == 0 && !from_second_level) goto _second_level_search;
            }
          }
#else // FAST_FREE / SLOW_ALLOC
          // slower discard (bitmap-based, 512o) //
          {
            unsigned bm_idx = offset / (granularity * 64);
            unsigned bm_bidx = (offset / granularity) % 64;
            uint64_t tmp = bitmap[bm_idx];
            for (; ((tmp >> bm_bidx) & 1) != 0; bm_bidx = (bm_bidx + 1) % 64, offset += granularity)
            {
              if (bm_bidx == 63)
              {
                if (bm_idx + 1 == bitmap_entries)
                  break;
                tmp = bitmap[++bm_idx];
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
            for (; ((tmp >> sl_bidx) & 1) == 0; sl_bidx = (sl_bidx + 1) % 64)
            {
              offset += second_level_granularity;
              if (offset - orig_offset > size)
                return offset - orig_offset;
              if (sl_bidx == 63)
              {
                if (sl_idx + 1 == second_level_entries)
                  break;
                tmp = second_level_bl[++sl_idx];
              }
              if (offset % first_level_granularity == 0) goto _first_level_search;
            }
          }

          // slower discard (bitmap-based, 512o) //
          {
            unsigned bm_idx = offset / (granularity * 64);
            unsigned bm_bidx = (offset / granularity) % 64;
            uint64_t tmp = bitmap[bm_idx];
            for (; ((tmp >> bm_bidx) & 1) == 0; bm_bidx = (bm_bidx + 1) % 64)
            {
              offset += granularity;
              if (bm_bidx == 63)
              {
                if (offset - orig_offset > size)
                  break;
                if (bm_idx + 1 == bitmap_entries)
                  break;
                tmp = bitmap[++bm_idx];
              }
              if (offset % first_level_granularity == 0) goto _first_level_search;
              if (offset % second_level_granularity == 0) goto _second_level_search;
            }
          }

          return offset - orig_offset;
#else // FAST_FREE / SLOW_ALLOC
          // slower discard (bitmap-based, 512o) //
          {
            unsigned bm_idx = offset / (granularity * 64);
            unsigned bm_bidx = (offset / granularity) % 64;
            uint64_t tmp = bitmap[bm_idx];
            for (; ((tmp >> bm_bidx) & 1) == 0; bm_bidx = (bm_bidx + 1) % 64, offset += granularity)
            {
              if (offset - orig_offset > size)
                break;
              if (bm_bidx == 63)
              {
                if (bm_idx + 1 == bitmap_entries)
                  break;
                tmp = bitmap[++bm_idx];
              }
            }

            return offset - orig_offset;
          }
#endif
          return 0;
        }

        /// \brief Used to find big chunks of free space (> 32kio)
        /// \note Disabled if using the fast-free/slow-alloc switch
        long fast_free_space_search(size_t offset, size_t size, size_t alignment)
        {
          size_t found_sz = 0;
          // super-fast search (using black levels) //
          if (size >= first_level_granularity * 2)
          {
            // We should find at least bit_count unmarked bit
            unsigned bit_count = size / (first_level_granularity * 1) - 1;
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
          // rollback if we have been too far
          if (((tmp >> bm_bidx) & 1) != 0)
          {
            offset += granularity;
            found_sz -= granularity;
          }

          int aoffset = offset;
          size_t asize = size;
          // align offset
          if (offset % alignment != 0)
          {
            aoffset += alignment - (offset % alignment);
            offset = aoffset - (aoffset % granularity);
            asize += aoffset - offset;
            if (asize % granularity) asize += granularity - (asize % granularity);
          }

          // forward search
          bm_idx = (offset + found_sz) / (granularity * 64);
          bm_bidx = ((offset + found_sz) / granularity) % 64;
          tmp = bitmap[bm_idx];

          for (; ((tmp >> bm_bidx) & 1) == 0 && found_sz < asize; bm_bidx = (bm_bidx + 1) % 64, found_sz += granularity)
          {
            if (bm_bidx == 63)
            {
              if (bm_idx == bitmap_entries)
                break;
              tmp = bitmap[++bm_idx];
            }
          }

          if (found_sz >= asize) // Yay ! found something
            return aoffset;

          // Not enough free space ! (tail recursion !)
          return fast_free_space_search(offset + found_sz, size, alignment);
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

#endif


