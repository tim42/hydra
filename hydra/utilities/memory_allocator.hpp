//
// created by : Timothée Feuillet
// date: 2021-11-19
//
//
// Copyright (c) 2021 Timothée Feuillet
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

#include <list>
#include <cstdint>
#include <ntools/mt_check/unordered_map.hpp>
#include <ntools/mt_check/map.hpp>
#include <ntools/mt_check/deque.hpp>

#include <ntools/tracy.hpp>

#include "../hydra_debug.hpp"
#include "../vulkan/device.hpp"
#include "../vulkan/device_memory.hpp"

#include "memory_allocation.hpp"
// #include "memory_allocator_chunk.hpp"
// #include "memory_allocator_chunk_chain.hpp"

#include "allocator/allocator_scope.hpp"
#include "allocator/allocator_set.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief An utility to manage GPU memory and perform less allocations
    /// \note It may not be ultra-efficient but should give correct results
    ///
    /// There are multiple internal pools. Those pools forms a set, and each allocation modes (host-visible, host-visible+memory-mapped, device-local, ...) have their own set.
    /// Pools can be empty or invalid (like the pass-local/host-visible pool) and their allocation size can be different (and specific to the allocation mode).
    /// A pool/pool-sets with no allocation will not allocate any memory. Memory usage is drectly tied to the number of raw allocation performed by the pool-set.
    ///
    /// The pass-local pools are fully cleared at the end of their scopes, moreover, subscopes can be created (and they will be fully cleared at their end).
    /// The current pools in a set are:
    ///  - persistent [slower to de/allocate, but try to minimise fragmentation]
    ///  - transient/short-lived [super fast, but expecting the allocations to live at most a few frames]
    ///                          Always allocate at the end of a block, free them when there's no allocation left on it
    ///                          WARNING: DO NOT ALLOCATE IF THE DURATION IS MORE THAN A FEW FRAMES
    ///  - pass-local [super fast, allocations are scoped to the pass]
    class memory_allocator /*: public allocator_interface*/
    {
      public:
        static constexpr unsigned align(unsigned alignment, unsigned value)
        {
          // Magic from glibc++
          return (value - 1u + alignment) & -alignment;
        }

        // The allocator itself only handle big allocations and subdivise them in blocks of a given size
        // Allocations bigger than 75% of a block will be given their own blocks
        // Allocations bigger than a raw allocation wil receive their own raw allocation.

        static constexpr size_t allocation_block_size = 8 * 1024 * 1024;
        static constexpr size_t raw_allocation_size = 512 * 1024 * 1024;
        static constexpr size_t minimum_block_for_allocators = 16;
        static constexpr size_t raw_allocation_block_count = raw_allocation_size / allocation_block_size;

        static_assert(raw_allocation_size % allocation_block_size == 0, "the raw allocation size must be a multiple of the block size");
        static_assert(raw_allocation_block_count == 64, "the raw allocation block count must be 64");
        static_assert(minimum_block_for_allocators < raw_allocation_block_count, "allocators cannot have more blocks than the raw allocation");

        // Above this, the two will be allocated separately and won't be mixed
        static constexpr size_t maximum_buffer_image_granularity = 256;


      public:
        /// \brief Create the memory allocator
        memory_allocator(vk::device &_dev) : dev(_dev)
        {
#ifndef HYDRA_NO_MESSAGES
          // A lovely print
//           memory_allocator_chunk<>::print_nfo();
#endif

          // Check if we need to have separate chains for optimal images and buffers
          // (some devices will need this, some others not)
          // NVIDIA gpu requires images to be page aligned (64kio) if the allocation is shared between buffers and images
          // In this case we split images and buffer into two separate chains
          check::on_vulkan_error::n_assert(dev.get_physical_device().get_limits().bufferImageGranularity <= allocation_block_size, "Allocator will not function properly as buffer image granularity is greater than the block size");

          buffer_image_granularity = dev.get_physical_device().get_limits().bufferImageGranularity;
          if (buffer_image_granularity > maximum_buffer_image_granularity)
            separate_buffer_image_allocations = true;
          else
            separate_buffer_image_allocations = false;

          is_unified_memory_system = false;

          // create all the different heaps (avoid creating them on the fly)
          const auto& memory_properties = dev.get_physical_device().get_memory_property();
          const uint32_t memory_type_count = memory_properties.memoryTypeCount;
          for (uint32_t memory_type_index = 0; memory_type_index < memory_type_count; ++memory_type_index)
          {
            const VkMemoryPropertyFlags flags = memory_properties.memoryTypes[memory_type_index].propertyFlags;

            if ((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0 && (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
              is_unified_memory_system = true;

            for (uint32_t optimal_image = 0; optimal_image < 2; ++optimal_image)
            {
              // normal:
              {
                const uint64_t allocator_key = ((uint64_t)memory_type_index) << 32 | (optimal_image) | (/*is_maped_memory*/ 0);
                heaps.emplace(std::piecewise_construct, std::tuple{allocator_key}, std::tuple<vk::device&, uint32_t, bool>{dev, memory_type_index, false});
              }

              // mapped (only create an entry for host-visible stuff):
              if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
              {
                const uint64_t allocator_key = ((uint64_t)memory_type_index) << 32 | (optimal_image) | (/*is_maped_memory*/ 2);
                heaps.emplace(std::piecewise_construct, std::tuple{allocator_key}, std::tuple<vk::device&, uint32_t, bool>{dev, memory_type_index, true});
              }

              if (!separate_buffer_image_allocations)
                break;
            }
          }
          // https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
          cr::out().debug("memory_allocator: image / buffer will have separate heaps: {}", separate_buffer_image_allocations);
          cr::out().debug("memory_allocator: created {} gpu-allocation heaps (number of gpu heaps: {})", heaps.size(), memory_type_count);
          cr::out().debug("memory_allocator: has unified memory: {}", is_unified_memory_system);
          if (cr::get_global_logger().can_log(cr::logger::severity::debug))
          {
            cr::out().debug("memory_allocator: memory types:");
            for (uint32_t memory_type_index = 0; memory_type_index < memory_type_count; ++memory_type_index)
            {
              const auto& it = memory_properties.memoryTypes[memory_type_index];
              std::vector<std::string_view> flags;
#define N_VK_PROPFLAG_TEST(X) if (it.propertyFlags & VK_MEMORY_PROPERTY_ ## X) flags.emplace_back(#X)
              N_VK_PROPFLAG_TEST(DEVICE_LOCAL_BIT);
              N_VK_PROPFLAG_TEST(HOST_VISIBLE_BIT);
              N_VK_PROPFLAG_TEST(HOST_COHERENT_BIT);
              N_VK_PROPFLAG_TEST(HOST_CACHED_BIT);
              N_VK_PROPFLAG_TEST(LAZILY_ALLOCATED_BIT);
              N_VK_PROPFLAG_TEST(PROTECTED_BIT);
              N_VK_PROPFLAG_TEST(DEVICE_COHERENT_BIT_AMD);
              N_VK_PROPFLAG_TEST(DEVICE_UNCACHED_BIT_AMD);
              N_VK_PROPFLAG_TEST(RDMA_CAPABLE_BIT_NV);
#undef N_VK_PROPFLAG_TEST
              cr::out().debug("  {:2}: size: {}MiB, flags: {}", memory_type_index, memory_properties.memoryHeaps[it.heapIndex].size / 1024 / 1024, fmt::join(flags, ", "));
            }
          }
        }

        /// \brief Free the memory, destroy the allocator
        ~memory_allocator() {}

        allocator::scope push_scope()
        {
          return {*this};
        }

        allocator::scope push_root_scope()
        {
          return {*this, nullptr};
        }

        allocator::pool_set* get_pool_set(VkMemoryPropertyFlags flags, allocation_type at)
        {
          const uint32_t mti = vk::device_memory::get_memory_type_index(dev, flags, ~0u);

          return get_pool_set_mti(mti, at);
        }

        allocator::pool_set* get_pool_set_mti(uint32_t memory_type_index, allocation_type at)
        {
          const allocation_type masked_at = (at & ~allocation_type::_flags);
          const bool optimal_image = (at & allocation_type::optimal_image) != allocation_type::none;
          const bool is_maped_memory = (at & allocation_type::mapped_memory) != allocation_type::none;

          check::on_vulkan_error::n_assert(masked_at != allocation_type::none, "cannot get the `none` memory_pool");

          const uint64_t allocator_key = ((uint64_t)memory_type_index) << 32 | (separate_buffer_image_allocations && optimal_image ? 1 : 0) | (is_maped_memory ? 2 : 0);
          if (auto it = heaps.find(allocator_key); it != heaps.end())
          {
            return &(it->second);
          }
          return nullptr;
        }

        /// \brief Allocate some memory (throw on error)
        /// \param short_lived Indicate whether or not the memory will be freed soon.
        ///                    You may gain performance (and have less fragmentation) in the long term
        ///                    if you correctly set this flag
        memory_allocation allocate_memory(const VkMemoryRequirements &reqs, VkMemoryPropertyFlags flags, allocation_type at)
        {
          TRACY_SCOPED_ZONE_COLOR(0x7F307F);
          const uint32_t mti = vk::device_memory::get_memory_type_index(dev, flags, reqs.memoryTypeBits);

          check::on_vulkan_error::n_assert(mti != ~0u, "could not find a suitable memory type to allocate");

          return allocate_memory(reqs.size, reqs.alignment, mti, at);
        }

        /// \brief Allocate some memory
        memory_allocation allocate_memory(size_t size, uint32_t alignment, uint32_t memory_type_index, allocation_type at)
        {
          TRACY_SCOPED_ZONE_COLOR(0x7F307F);
          const allocation_type masked_at = (at & ~allocation_type::_flags);
          const bool optimal_image = (at & allocation_type::optimal_image) != allocation_type::none;
          const bool is_maped_memory = (at & allocation_type::mapped_memory) != allocation_type::none;

          check::on_vulkan_error::n_assert(masked_at != allocation_type::none, "cannot allocated memory the `none` pool");
          check::on_vulkan_error::n_assert(size != 0, "cannot allocate no memory");
          check::on_vulkan_error::n_assert(alignment <= allocation_block_size, "cannot align more than a block size");


          const uint64_t allocator_key = ((uint64_t)memory_type_index) << 32 | (separate_buffer_image_allocations && optimal_image ? 1 : 0) | (is_maped_memory ? 2 : 0);
          if (auto it = heaps.find(allocator_key); it != heaps.end())
          {
            return it->second.allocate(size, alignment, masked_at);
          }
          check::on_vulkan_error::n_assert(masked_at != allocation_type::none, "cannot allocate memory: pool does not exist (mti: {}, optimal_image: {}, mapped_memory: {})",
                                           memory_type_index, optimal_image, is_maped_memory);
          return {};
          // auto [it, b] = heaps.emplace(std::piecewise_construct, std::tuple{allocator_key}, std::tuple<vk::device&, uint32_t, bool>{dev, memory_type_index, is_maped_memory});
          // return it->second.allocate(size, alignment, masked_at);

#if 0
          if (masked_at != allocation_type::scoped)
            ++allocation_count;

          // unmanaged memory: (slower)
          if (masked_at == allocation_type::raw || size >= raw_allocation_size - allocation_block_size)
          {
            TRACY_SCOPED_ZONE;
            if (is_maped_memory)
            {
              const size_t align_to = dev.get_physical_device().get_limits().nonCoherentAtomSize;
              if (size % align_to)
                size += align_to - size % align_to;
            }
            unmanaged_memory_size += size;
            vk::device_memory mem = vk::device_memory::allocate(dev, size, memory_type_index);
            if (is_maped_memory)
              mem.map_memory();
            return memory_allocation(memory_type_index, this, std::move(mem));
          }

          // block level allocations:
          // (block level allocations ignore frame-local stuff as deallocation is constant-time and super fast anyway)
          if ((masked_at != allocation_type::scoped) || masked_at == allocation_type::block_level || size >= allocation_block_size * 3 / 4)
          {
            TRACY_SCOPED_ZONE;
            const uint32_t bit_count = (size + allocation_block_size - 1) / allocation_block_size;

            const uint64_t mask = (uint64_t(1) << bit_count) - 1;

            // search for an allocation with at least bit_count free blocks
            for (auto& block : allocated_blocks)
            {
              if (block.memory_type_index != memory_type_index || !block.allocated)
                continue;
              if (block.mapped_pool != is_maped_memory)
                continue;

              const uint32_t free_blocks = __builtin_popcountl(block.free_mask);
              if (free_blocks >= bit_count)
              {
                uint64_t itmask = block.free_mask;
                uint32_t shift = 0;
                while (itmask >= mask && itmask != 0)
                {
                  const uint32_t ctz = __builtin_ctzl(itmask);
                  itmask >>= ctz;
                  shift += ctz;
                  if ((itmask & mask) == mask)
                  {
                    // found a space big enough !
                    block.free_mask &= ~(mask << shift); // update the mask

                    return memory_allocation(memory_type_index, allocation_type::block_level, shift * allocation_block_size, bit_count * allocation_block_size, &block.mem, this, nullptr);
                  }
                  // advance by the number of set bit:
                  const uint32_t cts = __builtin_ctzl(~itmask);
                  itmask >>= cts;
                  shift += cts;
                }
              }
            }

            // not found: add a new block, and allocate from there:
            const uint32_t index = add_new_empty_block(memory_type_index);
            allocated_blocks[index].free_mask = ~mask;
            allocated_blocks[index].mapped_pool = is_maped_memory;
            if (is_maped_memory)
            {
              allocated_blocks[index].mem.map_memory();
            }
            return memory_allocation(memory_type_index, allocation_type::block_level, 0, bit_count * allocation_block_size, &allocated_blocks[index].mem, this, nullptr);
          }

          // smaller allocations. The code _relies_ on the fact that block-level can be _less_ than a block.
          {
            TRACY_SCOPED_ZONE;
            // find an allocation with the correct allocator
            if (masked_at == allocation_type::scoped)
            {
              ++frame_allocation_count;

              // Single-frame allocator is handled a bit differently because of its simplicity.
              // We have the data in the allocations directly as freeing is basically free and allocating is quick
              // Frame-allocating huge chunks of memory can be costly tho, as for > raw_allocation_size
              // we use the driver allocator and the allocation becomes unmanaged.
              // For everything else (and particularly medium sized allocations (block-level stuff)) it's quite fast.

              // Because on how we handle memory we have to do a quick search on the allocations.
              // There shouldn't be more than a few allocations anyway:
              // a GPU with 24GB of ram can have at most 48 allocations
              //
              // The frame-allocator is super wastefull, but fast as fragmentation is only for the current frame.
              for (auto& block : allocated_blocks)
              {
                // super fast search to find a matching allocation:
                if (block.memory_type_index != memory_type_index || !block.allocated)
                  continue;
                if (block.mapped_pool != is_maped_memory)
                  continue;

                const uint32_t current_alignment = (optimal_image == block.frame_allocator.previous_was_image) ? alignment : std::max(alignment, buffer_image_granularity);
                const uint32_t current_block_ofset = align(block.frame_allocator.offset_in_block, current_alignment);
                // check if we can append to the current allocations:
                if (block.frame_allocator.mask != 0 &&
                  // we fit in the current block:
                  ((current_block_ofset + size <= allocation_block_size) ||
                  // there's a free block right after this one anyway:
                  (block.frame_allocator.block != 63 && (block.free_mask & uint64_t(1) << (block.frame_allocator.block + 1)) != 0))
                )
                {
                  block.frame_allocator.offset_in_block += current_block_ofset + size;
                  block.frame_allocator.previous_was_image = optimal_image;
                  const uint64_t offset = block.frame_allocator.block * allocation_block_size + current_block_ofset;
                  if (block.frame_allocator.offset_in_block > allocation_block_size)
                  {
                    const uint64_t block_mask = uint64_t(1) << (block.frame_allocator.block + 1);
                    block.frame_allocator.mask |= block_mask;
                    block.free_mask &= ~block_mask;
                    block.frame_allocator.offset_in_block -= allocation_block_size;
                  }
                  // return the allocation:
                  return memory_allocation(memory_type_index, at, offset, size, &block.mem, this, reinterpret_cast<void*>(frame_allocation_marker));
                }
                // check if the block has any remaining free-space that could be used:
                if (block.free_mask != 0)
                {
                  const uint32_t block_index = __builtin_ctzl(block.free_mask);
                  block.frame_allocator.mask |= uint64_t(1) << block_index;
                  block.free_mask &= ~(uint64_t(1) << block_index);
                  block.frame_allocator.block = block_index;
                  block.frame_allocator.offset_in_block = size;
                  block.frame_allocator.previous_was_image = optimal_image;
                  return memory_allocation(memory_type_index, at, block_index * allocation_block_size, size, &block.mem, this, reinterpret_cast<void*>(frame_allocation_marker));
                }
              }

              // no space found, add a new raw allocation:
              {
                const uint32_t index = add_new_empty_block(memory_type_index);
                auto& block = allocated_blocks[index];
                allocated_blocks[index].mapped_pool = is_maped_memory;
                if (is_maped_memory)
                {
                  allocated_blocks[index].mem.map_memory();
                }

                const uint32_t block_index = 0;
                block.frame_allocator.mask |= uint64_t(1) << block_index;
                block.free_mask &= ~(uint64_t(1) << block_index);
                block.frame_allocator.block = block_index;
                block.frame_allocator.offset_in_block = size;
                block.frame_allocator.previous_was_image = optimal_image;

                return memory_allocation(memory_type_index, at, 0, size, &block.mem, this, reinterpret_cast<void*>(frame_allocation_marker));
              }
            }
          }
          // Failed:
          return memory_allocation();
#endif
        }

        /// \brief Return a chunk of memory previously allocated
        /// \note This isn't done automatically by vk:: objects, so you may need
        /// to do that by yourself when the object is destroyed or another memory
        /// is bound to it
        /// \see memory_allocation::free()
//         void free_allocation(const memory_allocation &mem) override { free_memory(mem); }
//         void free_memory(const memory_allocation &mem)
//         {
//           TRACY_SCOPED_ZONE_COLOR(0x307F30);
//           check::on_vulkan_error::n_assert(mem.allocator() == this, "wrong allocator used to deallocate the memory");
//
//           if ((mem._get_allocation_type() != allocation_type::scoped)
//             && (mem._get_allocation_type() != allocation_type::scoped_optimal_image))
//           {
//             --allocation_count;
//           }
//
//           if (mem._get_allocation_type() == allocation_type::raw)
//           {
//             // update stats:
//             unmanaged_memory_size -= mem.mem()->get_size();
//             return;
//           }
//
//           if (mem._get_allocation_type() == allocation_type::block_level)
//           {
//             const uint32_t bit_count = mem.size() / allocation_block_size;
//             const uint32_t shift = mem.offset() / allocation_block_size;
//
//             // most unholly conversion: both a const-cast and a reinterpret_cast in the same operation !
//             // but hey. Avoiding a search with a single cast operation is awesome.
//             block_allocation& block = *(block_allocation*)(mem.mem());
//
//             // update the mask to free the blocks:
//             block.free_mask |= ((uint64_t(1) << bit_count) - 1) << shift;
//
//             // done !
//             return;
//           }
//
//           if ((mem._get_allocation_type() == allocation_type::scoped)
//             || (mem._get_allocation_type() == allocation_type::scoped_optimal_image))
//           {
//             check::on_vulkan_error::n_assert(reinterpret_cast<uint64_t>(mem._get_payload()) == frame_allocation_marker, "single_frame allocation outlived its frame");
//
//             return; // nothing to do !
//           }
//         }

        // Frame end
        void flush_empty_allocations()
        {
#if 0
          TRACY_SCOPED_ZONE;
          frame_allocation_count = 0;
          ++frame_allocation_marker;

          // loop over all the blocks, freeing those without allocation:
          for (unsigned i = 0; i < allocated_blocks.size(); ++i)
          {
            auto& block = allocated_blocks[i];
            if (block.allocated)
            {
              // clear the frame allocator blocks:
              block.free_mask |= block.frame_allocator.mask;
              block.frame_allocator = {}; // reset the frame allocator

              // if the block is empty, free it (FIXME:)
//               if (block.free_mask == ~uint64_t(0))
//                 free_block_allocation(i);
            }
          }

          // TODO: trim trailling blocks and re-order the free-block chained-list.
          //       (there shouldn't be many entries anyway)
#endif
        }

        size_t get_used_memory() const
        {
          return get_reserved_memory(); // fixme!
        }

        size_t get_reserved_memory() const
        {
          size_t res = 0;
          for (auto&& it : heaps)
          {
            res += it.second.get_allocated_memory();
          }
//           for (const auto& block : allocated_blocks)
//           {
//             if (block.allocated)
//               res += block.mem.get_size();
//           }
          return res /*+ unmanaged_memory_size*/;
        }

        uint32_t get_free_block_count() const
        {
          uint32_t free_block_count = 0;
//           for (const auto& block : allocated_blocks)
//           {
//             if (block.allocated)
//               free_block_count += __builtin_popcountl(block.free_mask);
//           }
          return free_block_count;
        }


        size_t get_allocation_count() const
        {
          return 0;//allocation_count + frame_allocation_count;
        }

        /// \brief print memory stats for the different kind of pools
        void print_stats() const
        {
          cr::out().log("-- [GPU memory stats] --\n"
                        "total reserved memory: {0} Mio\n"
                        "total allocation count: {1} Mio\n"
                        "free blocks: {2} Mio\n"
                        "-- [GPU memory stats] --\n",
                        (get_reserved_memory() / (1024.f * 1024.f)),
                        get_allocation_count(),
                        (get_free_block_count() * allocation_block_size / (1024.f * 1024.f)));
        }

      private:
#if 0
        uint32_t add_new_empty_block(uint32_t memory_type_index)
        {
          TRACY_SCOPED_ZONE;
          // no free entries, add a new one:
          if (first_free_index == ~0u)
          {
            allocated_blocks.push_back(
              {
                vk::device_memory::allocate(dev, raw_allocation_size, memory_type_index),
                ~uint64_t(0),
                {}, // frame allocator
                memory_type_index,
                true,
              });
            return allocated_blocks.size() - 1;
          }

          // re-use a block:
          const uint32_t index = first_free_index;
          first_free_index = allocated_blocks[index].memory_type_index; // the next free block is in memory_type_index
          allocated_blocks[index] =
          {
            vk::device_memory::allocate(dev, raw_allocation_size, memory_type_index),
            ~uint64_t(0),
            {}, // frame allocator
            memory_type_index,
            true,
          };
          return index;
        }

        void free_block_allocation(uint32_t index)
        {
          TRACY_SCOPED_ZONE;
          allocated_blocks[index] =
          {
            vk::device_memory(dev),
            0,
            {}, // frame allocator
            first_free_index, // we use memory_type_index as the next free block index
            false,
          };
          first_free_index = index;
        }
#endif
      private:
        vk::device &dev;
        uint32_t buffer_image_granularity;
        bool separate_buffer_image_allocations = true;
        bool is_unified_memory_system = false;

        std::mtc_map<uint64_t, allocator::pool_set> heaps;

#if 0
        // FIXME: CHANGE THE WAY IT'S HANDLE (a dequeue of just the frame allocation stuff so it's faster)
        struct frame_allocator_data
        {
          uint64_t mask = 0; // Freeing stuff is as simple as |= with free_mask
          uint32_t offset_in_block = 0;
          uint16_t block = 0;
          bool previous_was_image = false;
        };

        struct block_allocation
        {
          vk::device_memory mem; // MUST be the first field !!
          uint64_t free_mask;

          // frame allocation:
          frame_allocator_data frame_allocator;

          uint32_t memory_type_index;
          bool mapped_pool = false;
          bool allocated = true;
        };

        std::deque<block_allocation> allocated_blocks;
        uint32_t first_free_index = ~0u;
        // Validation info:
        size_t frame_allocation_marker = 0;

        // stats:
        size_t allocation_count = 0;
        size_t frame_allocation_count = 0;
        size_t unmanaged_memory_size = 0;
#endif

        friend allocator::scope;
    };


  } // namespace hydra
} // namespace neam


