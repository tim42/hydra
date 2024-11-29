//
// file : transfer.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/transfer.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 27 2016 15:53:51 GMT+0200 (CEST)
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

#include <deque>
#include <cstring>
#include <mutex>
#include <atomic>

#include <ntools/spinlock.hpp>
#include <ntools/tracy.hpp>
#include <hydra_glm.hpp>


#include "../hydra_debug.hpp"
#include "../vulkan/vulkan.hpp"

#include "memory_allocator.hpp"

namespace neam::hydra
{
  struct hydra_context;

  /// \brief Batch and dispatch cpu to gpu data transfers (using staging buffers, ...).
  ///
  /// REFACTO NOTES:
  ///  - create command buffers for transfers (+ submit infos). Can be contextualized.
  ///   - memcpy from cpu to mapped gpu memory should be done in a separate task.
  ///   - asynchronous transfers. memcpy and gpu->gpu copies should be done as early as possible, on another threads
  ///  - support the case where we have unified memory (with a switch to disable it, for testing purpose)
  /// https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
  /// Probably do something to help queue transfers
  class batch_transfers
  {
    public:
      batch_transfers(hydra_context &_hctx);

      ~batch_transfers() = default;

      /// \brief Return the total size of the data to transfer
      size_t get_total_size_to_transfer() const { return total_size; }

      /// \brief Return the number of transfer to perform
      size_t get_transfer_count() const { return buffer_transfer_list.size() + image_transfer_list.size(); }

      /// \brief Queue a transfer from a source queue to the transfer queue, if necessary
      void acquire(vk::buffer& buf, size_t src_queue_familly, vk::semaphore* wait_semaphore = nullptr, vk::semaphore* signal_semaphore = nullptr);

      /// \brief Queue a transfer from a source queue to the transfer queue, if necessary and perform a layout transition, if necessary
      void acquire(vk::image& img, size_t src_queue_familly, VkImageLayout source_layout, VkImageLayout dest_layout, VkAccessFlags dest_access, vk::semaphore* wait_semaphore = nullptr, vk::semaphore* signal_semaphore = nullptr);


      /// \brief Add a buffer to be filled with some data
      /// \note the data will be copied, so you can either free it or modify it without any risk
      void add_transfer(vk::buffer& buf, raw_data&& data, size_t buf_offset = 0);

      /// \brief Add an image to be filled with some data
      /// \note the data will be copied, so you can either free it or modify it without any risk
      void add_transfer(vk::image& img, raw_data&& data);

      /// \brief Add a subregion of an image to be filled with some data
      /// \note the data will be copied, so you can either free it or modify it without any risk
      void add_transfer(vk::image& img, raw_data&& data, const glm::uvec3& size, const glm::ivec3& offset = {0, 0, 0});

      bool has_transfers() const { return !(buffer_transfer_list.empty() && image_transfer_list.empty()); }

      /// \brief Do the transfers
      bool transfer(memory_allocator& mem_alloc, neam::hydra::vk::submit_info& tsi);

      /// \brief Acquire the resources (transfer the queue ownership)
      void acquire_resources(vk::command_buffer_recorder& cbr, size_t src_queue_familly);
      /// \brief Release the acquired resources
      void release_resources(vk::command_buffer_recorder& cbr, size_t dst_queue_familly);

      void clear_resources_to_release()
      {
        buffer_rel_list.clear();
        image_rel_list.clear();
      }

    private:
      // buffer stuff
      struct _buffer_data_transfer
      {
        vk::buffer* buf;
        size_t buf_offset;
        raw_data data;
      };

      struct _image_data_transfer
      {
        vk::image* img;
        glm::ivec3 offset;
        glm::uvec3 size;
        uint32_t image_alignment;

        raw_data data;
      };

      struct _buffer_acquisition
      {
        vk::buffer* buf;
        vk::semaphore* wait_semaphore;
        vk::semaphore* signal_semaphore;

        size_t src_queue_familly;
        size_t dst_queue_familly;
      };

      struct _image_acquisition
      {
        vk::image* img;
        vk::semaphore* wait_semaphore;
        vk::semaphore* signal_semaphore;

        size_t src_queue_familly;
        size_t dst_queue_familly;

        VkImageLayout source_layout;
        VkImageLayout dest_layout;
        VkAccessFlags dest_access;
      };

    private:
      hydra_context& hctx;

      spinlock transfer_list_lock;
      std::deque<_buffer_data_transfer> buffer_transfer_list;
      std::deque<_buffer_acquisition> buffer_acq_list;
      std::deque<_buffer_acquisition> buffer_rel_list;

      std::deque<_image_data_transfer> image_transfer_list;
      std::deque<_image_acquisition> image_acq_list;
      std::deque<_image_acquisition> image_rel_list;

      size_t total_size = 0;
  };
} // namespace neam::hydra


