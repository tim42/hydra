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

#ifndef __N_247872795872822206_2274418095_TRANSFER_HPP__
#define __N_247872795872822206_2274418095_TRANSFER_HPP__

#include <deque>
#include <cstring>
#include <mutex>
#include <atomic>

#include <ntools/spinlock.hpp>

#include "../hydra_debug.hpp"
#include "../vulkan/vulkan.hpp"

#include "memory_allocator.hpp"
#include "vk_resource_destructor.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief Batch data transfers (using staging buffers, ...).
    class batch_transfers
    {
      public:
        using transfer_func = void (*)(vk::command_buffer_recorder&);

      private:
        // buffer stuff
        struct _buffer_data_transfer
        {
          vk::buffer *buf;
          size_t buf_offset;
          int8_t *data;
          size_t data_size;
          size_t data_offset;
          size_t dst_queue_familly;

          vk::semaphore *signal_semaphore;
          vk::fence *signal_fence;

          transfer_func pre_transfert_func = nullptr;
          transfer_func post_transfert_func = nullptr;
        };

        struct _image_data_transfer
        {
          vk::image *img;
          VkImageLayout layout;
          glm::ivec3 offset;
          glm::uvec3 size;
          uint32_t image_alignment;

          int8_t *data;
          size_t data_size;

          vk::semaphore *signal_semaphore;
          vk::fence *signal_fence;
          VkImageLayout final_layout;

          transfer_func pre_transfert_func = nullptr;
          transfer_func post_transfert_func = nullptr;
        };

      public:
        batch_transfers(vk::device &_dev, vk::queue &_tqueue, vk_resource_destructor &_vrd)
          : dev(_dev), tqueue(_tqueue), cmd_pool(_tqueue.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)), vrd(_vrd)
        {
        }

        ~batch_transfers()
        {
          {
            std::lock_guard<spinlock> _u0(transfer_list_lock);

            buffer_transfer_list.clear();
            image_transfer_list.clear();
            local_memory.clear();
          }
        }

        size_t get_queue_familly_index() const { return tqueue.get_queue_familly_index(); }
        size_t get_queue_index() const { return tqueue.get_queue_index(); }

        /// \brief Return the total size of the data to transfer
        size_t get_total_size_to_transfer() const { return total_size; }

        /// \brief Return the number of transfer to perform
        size_t get_transfer_count() const { return buffer_transfer_list.size() + image_transfer_list.size(); }

        /// \brief Add a buffer to be filled with some data
        /// \note the data will be copied, so you can either free it or modify it without any risk
        /// \param signal_semaphore will be signaled when the whole buffer has been transfered
        /// \param signal_fence will be signaled when the whole buffer has been transfered
        void add_transfer(vk::buffer &buf, size_t buf_offset, size_t data_size, const void *_data, size_t dst_queue_familly, vk::semaphore *signal_semaphore = nullptr, vk::fence *signal_fence = nullptr,
                          transfer_func pre = nullptr, transfer_func post = nullptr)
        {
          const void* data;
          if (!is_memory_from_allocator(_data))
          {
            data = allocate_memory(data_size);
            memcpy(const_cast<void*>(data), _data, data_size);
          }
          else
          {
            data = _data;
          }

          std::lock_guard<spinlock> _u0(transfer_list_lock);
          total_size += data_size;

          buffer_transfer_list.push_back(_buffer_data_transfer
          {
            &buf, buf_offset, (int8_t *)data, data_size, 0,
            dst_queue_familly,
            signal_semaphore, signal_fence,
            pre, post,
          });
        }

        /// \brief Add an image to be filled with some data
        /// \note the data will be copied, so you can either free it or modify it without any risk
        /// \note you can't transfer data to images if the data size is bigger than get_transfer_window_size()
        /// \param final_layout Is the layout the image will be transitionned to after the transfer
        /// \param signal_semaphore will be signaled when the whole buffer has been transfered
        /// \param signal_fence will be signaled when the whole buffer has been transfered
        /// \param pre For images where initial layout isn't VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        ///            this function is mandatory and will be called before issuing the transfer command.
        ///            It must change the image layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL by issuing a pipeline_barrier()
        /// \param post For images where final_layout isn't VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        ///            this function is mandatory and will be called before issuing the transfer command.
        ///            It should change the image layout to final_layout by issuing a pipeline_barrier()
        void add_transfer(vk::image &img, VkImageLayout img_layout, VkImageLayout final_layout, size_t data_size, const void *_data,
                          vk::semaphore *signal_semaphore = nullptr, vk::fence *signal_fence = nullptr,
                          transfer_func pre = nullptr, transfer_func post = nullptr)
        {
          constexpr uint32_t image_alignment = 32;
          const void* data;
          if (!is_memory_from_allocator(_data))
          {
            data = allocate_memory(data_size);
            memcpy(const_cast<void*>(data), _data, data_size);
          }
          else
          {
            data = _data;
          }

          std::lock_guard<spinlock> _u0(transfer_list_lock);
          total_size += image_alignment + data_size;

          image_transfer_list.push_back(_image_data_transfer
          {
            &img, img_layout,
            glm::ivec3(0, 0, 0), img.get_size(),
            image_alignment,
            (int8_t *)data, data_size,
            signal_semaphore, signal_fence,
            final_layout,
            pre, post,
          });
        }

        /// \brief Add a subregion of an image to be filled with some data
        /// \note the data will be copied, so you can either free it or modify it without any risk
        /// \note you can't transfer data to images if the data size is bigger than get_transfer_window_size()
        /// \param final_layout Is the layout the image will be transitionned to after the transfer
        /// \param signal_semaphore will be signaled when the whole buffer has been transfered
        /// \param signal_fence will be signaled when the whole buffer has been transfered
        /// \param pre For images where initial layout isn't VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        ///            this function is mandatory and will be called before issuing the transfer command.
        ///            It must change the image layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL by issuing a pipeline_barrier()
        /// \param post For images where final_layout isn't VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        ///            this function is mandatory and will be called before issuing the transfer command.
        ///            It should change the image layout to final_layout by issuing a pipeline_barrier()
        void add_transfer(vk::image &img, VkImageLayout img_layout, VkImageLayout final_layout, const glm::ivec3 &offset, const glm::uvec3 &size,
                          size_t data_size, const void *_data, vk::semaphore *signal_semaphore = nullptr, vk::fence *signal_fence = nullptr,
                          transfer_func pre = nullptr, transfer_func post = nullptr)
        {
          constexpr uint32_t image_alignment = 32;
          const void* data;
          if (!is_memory_from_allocator(_data))
          {
            data = allocate_memory(data_size);
            memcpy(const_cast<void*>(data), _data, data_size);
          }
          else
          {
            data = _data;
          }

          std::lock_guard<spinlock> _u0(transfer_list_lock);
          total_size += image_alignment + data_size;

          image_transfer_list.push_back(_image_data_transfer
          {
            &img, img_layout,
            offset, size,
            image_alignment,
            (int8_t *)data, data_size,
            signal_semaphore, signal_fence,
            final_layout,
            pre, post,
          });
        }

        /// \brief Do the transfer
        bool transfer(memory_allocator& mem_alloc, std::vector<vk::semaphore*> sema = {}, vk::fence* fence = nullptr)
        {
          if (buffer_transfer_list.empty() && image_transfer_list.empty())
            return false;

          // create the stating buffer:
          vk::buffer staging_buffer = vk::buffer(dev, total_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
          memory_allocation alloc = mem_alloc.allocate_memory(staging_buffer.get_memory_requirements(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, allocation_type::block_level | allocation_type::mapped_memory);

          staging_buffer.bind_memory(*alloc.mem(), alloc.offset());

          // fill the memory, +create the command buffer
          vk::command_buffer cmd_buf(cmd_pool.create_command_buffer());
          {
            int8_t* memory = (int8_t*)alloc.mem()->map_memory(alloc.offset());
            vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            size_t current_offset = 0;

            std::lock_guard<spinlock> _u0(transfer_list_lock);

            // transfer buffers //
            for (auto& buffer_transfer : buffer_transfer_list)
            {
              const size_t cp_sz = (buffer_transfer.data_size - buffer_transfer.data_offset);

              memcpy(memory + current_offset, buffer_transfer.data + buffer_transfer.data_offset, cp_sz);

              if (buffer_transfer.pre_transfert_func)
                buffer_transfer.pre_transfert_func(cbr);

              cbr.copy_buffer(staging_buffer, *buffer_transfer.buf, {{ current_offset, buffer_transfer.buf_offset, cp_sz }});

              // release the buffer:
              cbr.pipeline_barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                    {
                                      vk::buffer_memory_barrier::queue_transfer(*buffer_transfer.buf, tqueue.get_queue_familly_index(), buffer_transfer.dst_queue_familly)
                                    });

              if (buffer_transfer.post_transfert_func)
                buffer_transfer.post_transfert_func(cbr);

              total_size -= cp_sz;
              current_offset += cp_sz;
            }

            // transfer images //
            for (auto& image_transfer : image_transfer_list)
            {
              if (current_offset % image_transfer.image_alignment)
                current_offset += image_transfer.image_alignment - current_offset % image_transfer.image_alignment;
              const size_t cp_sz = (image_transfer.data_size);
              memcpy(memory + current_offset, image_transfer.data, cp_sz);
              if (image_transfer.pre_transfert_func)
              {
                image_transfer.pre_transfert_func(cbr);
              }
              else if (image_transfer.layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
              {
                cbr.pipeline_barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                      {
                                        vk::image_memory_barrier(*image_transfer.img, image_transfer.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        0, VK_ACCESS_TRANSFER_WRITE_BIT)
                                      });
              }

              cbr.copy_buffer_to_image(staging_buffer, *image_transfer.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        { current_offset, image_transfer.offset, image_transfer.size });

              if (image_transfer.post_transfert_func)
              {
                image_transfer.post_transfert_func(cbr);
              }
              else if (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL != image_transfer.final_layout)
              {
                if (image_transfer.final_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                {
                  cbr.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0,
                                      { vk::image_memory_barrier(*image_transfer.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT) });
                }
              }

              total_size -= cp_sz;
              current_offset += cp_sz;
            }

            cmd_buf.end_recording();

            vk::submit_info si;

            si << cmd_buf;

            for (auto* s : sema)
              si >> *s;

            // add semaphores:
            for (const auto& transfer : buffer_transfer_list)
            {
              if (transfer.signal_semaphore != nullptr)
                si >> *transfer.signal_semaphore;
            }
            for (const auto& transfer : image_transfer_list)
            {
              if (transfer.signal_semaphore != nullptr)
                si >> *transfer.signal_semaphore;
            }

            // add fences:
            for (const auto& transfer : buffer_transfer_list)
            {
              if (transfer.signal_fence != nullptr)
                si >> *transfer.signal_fence;
            }
            for (const auto& transfer : image_transfer_list)
            {
              if (transfer.signal_fence != nullptr)
                si >> *transfer.signal_fence;
            }

            vk::fence transfer_end_fence(dev);
            if (fence != nullptr)
              si >> *fence;
            si >> transfer_end_fence;

            // start the transfer / flush
            alloc.mem()->flush(memory, staging_buffer.size(), true);

            tqueue.submit(si);

            // clear the state of everything:
            image_transfer_list.clear();
            buffer_transfer_list.clear();

            clear_local_allocator();

            vrd.postpone_destruction(tqueue, std::move(transfer_end_fence), std::move(cmd_buf), std::move(staging_buffer), std::move(alloc));
          }

          return true;
        }


        /// \brief Return whether the memory is managed by the instance of this class
        bool is_memory_from_allocator(const void* memory) const
        {
          for (const auto& it : local_memory)
          {
            if (it.base_address > memory && (void*)((const uint8_t*)it.base_address + it.size) < memory)
              return true;
          }
          return false;
        }

        /// \brief Allocate some memory. This avoids a copy/new allocation so it's only done once.
        /// Also it's a tiny bit faster than new
        /// \warning the memory will not have any constructor/destructor called and may be re-used as-is between transfers
        void* allocate_memory(size_t size)
        {
          if (size < default_entry_size)
          {
            for (auto& it : local_memory)
            {
              if (it.used + size <= it.size)
              {
                const size_t offset = it.used;
                it.used += size;
                return (uint8_t*)it.base_address + offset;
              }
            }
            // no space found: create a new entry with enough space
            local_memory.emplace_back(default_entry_size);
            auto& entry = local_memory.back();
            entry.used = size;
            return entry.base_address;
          }

          // no space found: create a new entry with enough space
          local_memory_oversized.emplace_back(size);
          auto& entry = local_memory_oversized.back();
          entry.used = size;
          return entry.base_address;
        }

      private:
        // Remove all allocations that are above the default size
        // Remove the last two entries to gradually reduce memory footprint
        void clear_local_allocator()
        {
          local_memory_oversized.clear();
          for (auto& it : local_memory)
          {
            it.used = 0;
          }
          if (local_memory.size() > 2)
          {
            local_memory.pop_back();
            local_memory.pop_back();
          }
        }

      private:
        vk::device &dev;
        vk::queue &tqueue;
        vk::command_pool cmd_pool;
        vk_resource_destructor &vrd;

        spinlock transfer_list_lock;
        std::deque<_buffer_data_transfer> buffer_transfer_list;
        std::deque<_image_data_transfer> image_transfer_list;
        size_t total_size = 0;

        // local allocator stuff:
        struct local_memory_entry
        {
          void* base_address = nullptr;
          size_t size;
          size_t used;

          local_memory_entry(size_t _size) : base_address(operator new (_size)), size(_size), used(0) {}
          local_memory_entry(local_memory_entry&& o) : base_address(o.base_address), size(o.size), used(o.used)
          {
            o.base_address = nullptr;
          }
          local_memory_entry& operator = (local_memory_entry&& o)
          {
            if (&o == this)
              return *this;
            base_address = (o.base_address);
            size = (o.size);
            used = (o.used);
            o.base_address = nullptr;
            return *this;
          }
          ~local_memory_entry() { operator delete(base_address); }
        };
        static constexpr size_t default_entry_size = 8 * 1024 * 1024; // default allocation size. Everything smaller will merged in a single allocation.
        std::deque<local_memory_entry> local_memory;
        std::deque<local_memory_entry> local_memory_oversized;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_247872795872822206_2274418095_TRANSFER_HPP__

