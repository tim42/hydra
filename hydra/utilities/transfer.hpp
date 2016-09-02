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

#include "../tools/spinlock.hpp"

#include "../hydra_exception.hpp"
#include "../vulkan/vulkan.hpp"

#include "memory_allocator.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief Batch async data transfers (using staging buffers, ...).
    /// It can transfer buffers and images
    class batch_transfers
    {
      private:
        // buffer stuff
        struct _data_transfer
        {
          vk::buffer *buf;
          size_t buf_offset;
          int8_t *data;
          size_t data_size;
          size_t data_offset;

          vk::semaphore *signal_semaphore;
          vk::fence *signal_fence;
        };

        struct _img_data_transfer
        {
          vk::image *img;
          VkImageLayout layout;
          glm::ivec3 offset;
          glm::uvec3 size;

          int8_t *data;
          size_t data_size;

          vk::semaphore *signal_semaphore;
          vk::fence *signal_fence;
          VkImageLayout final_layout;
        };

      public:
        batch_transfers(vk::device &_dev, vk::queue &_tqueue, vk::command_pool &_cmd_pool, size_t transfer_window_size = 20 * 1024 * 1024)
          : dev(_dev), tqueue(_tqueue), cmd_pool(_cmd_pool), cmd_buf(cmd_pool.create_command_buffer()),
            staging_buffer(dev, transfer_window_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
            end_fence(dev)
        {
          transfer_in_progress = false;
        }

        ~batch_transfers()
        {
          {
            std::lock_guard<spinlock> _u0(transfer_list_lock);

            for (_data_transfer &it : transfer_list)
              operator delete(it.data);
            transfer_list.clear();

            for (_img_data_transfer &it : img_transfer_list)
              operator delete(it.data);
            img_transfer_list.clear();
          }

          allocation.free();
        }

        /// \brief Return the total size of the data to transfer
        size_t get_total_size_to_transfer() const { return total_size; }

        /// \brief Return the size of the transfer window
        size_t get_transfer_window_size() const { return staging_buffer.size(); }

        /// \brief Return the memory requirements of the staging buffer
        VkMemoryRequirements get_memory_requirements() const { return staging_buffer.get_memory_requirements(); }

        /// \brief Return the number of transfer to perform
        size_t get_transfer_count() const { return transfer_list.size(); }

        /// \brief Add a buffer to be filled with some data
        /// \note the data will be copied
        /// \param signal_semaphore will be signaled when the whole buffer has been transfered
        /// \param signal_fence will be signaled when the whole buffer has been transfered
        void add_transfer(vk::buffer &buf, size_t buf_offset, size_t data_size, const void *_data, vk::semaphore *signal_semaphore = nullptr, vk::fence *signal_fence = nullptr)
        {
          void *data = operator new(data_size);
          memcpy(data, _data, data_size);

          std::lock_guard<spinlock> _u0(transfer_list_lock);
          total_size += data_size;

          transfer_list.push_back(_data_transfer
          {
            &buf, buf_offset, (int8_t *)data, data_size, 0,
            signal_semaphore, signal_fence
          });
        }

        /// \brief Add an image to be filled with some data
        /// \note the data will be copied
        /// \note you can't transfer data to images if the data size is bigger than get_transfer_window_size()
        /// \param final_layout Is the layout the image will be transitionned to after the transfer
        /// \param signal_semaphore will be signaled when the whole buffer has been transfered
        /// \param signal_fence will be signaled when the whole buffer has been transfered
        void add_transfer(vk::image &img, VkImageLayout img_layout, VkImageLayout final_layout, size_t data_size, const void *_data,
                          vk::semaphore *signal_semaphore = nullptr, vk::fence *signal_fence = nullptr)
        {
          void *data = operator new(data_size);
          memcpy(data, _data, data_size);

          std::lock_guard<spinlock> _u0(transfer_list_lock);
          total_size += data_size;

          img_transfer_list.push_back(_img_data_transfer
          {
            &img, img_layout,
            glm::ivec3(0, 0, 0), img.get_size(),
            (int8_t *)data, data_size,
            signal_semaphore, signal_fence,
            final_layout
          });
        }

        /// \brief Add a subregion of an image to be filled with some data
        /// \note the data will be copied
        /// \note you can't transfer data to images if the data size is bigger than get_transfer_window_size()
        /// \param final_layout Is the layout the image will be transitionned to after the transfer
        /// \param signal_semaphore will be signaled when the whole buffer has been transfered
        /// \param signal_fence will be signaled when the whole buffer has been transfered
        void add_transfer(vk::image &img, VkImageLayout img_layout, VkImageLayout final_layout, const glm::ivec3 &offset, const glm::uvec3 &size,
                          size_t data_size, const void *_data, vk::semaphore *signal_semaphore = nullptr, vk::fence *signal_fence = nullptr)
        {
          void *data = operator new(data_size);
          memcpy(data, _data, data_size);

          std::lock_guard<spinlock> _u0(transfer_list_lock);
          total_size += data_size;

          img_transfer_list.push_back(_img_data_transfer
          {
            &img, img_layout,
            offset, size,
            (int8_t *)data, data_size,
            signal_semaphore, signal_fence,
            final_layout
          });
        }

        /// \brief Allocate and bind some memory for the batch transfer
        /// The memory is then freed when the transfer object is destructed
        void allocate_memory(memory_allocator &mem_alloc)
        {
          _bind_memory_area(mem_alloc.allocate_memory(get_memory_requirements(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
        }

        /// \brief Affect some memory for the batch transfer
        /// \note The duty to free the memory is yours. Please use allocate_memory()
        ///       if you don't like that
        void _bind_memory_area(vk::device_memory &area, size_t offset)
        {
          check::on_vulkan_error::n_assert(area.get_size() >= staging_buffer.size(), "memory area too small");
          check::on_vulkan_error::n_assert(transfer_in_progress == false, "could not change the memory area while a transfer is in progress");

          allocation.free();
          allocation = memory_allocation();

          mem_area = &area;
          mem_offset = offset;
          staging_buffer.bind_memory(*mem_area, offset);
        }

        /// \brief Bind some memory for the batch transfer
        void _bind_memory_area(const memory_allocation &ma)
        {
          check::on_vulkan_error::n_assert(ma.size() >= staging_buffer.size(), "memory area too small");
          check::on_vulkan_error::n_assert(transfer_in_progress == false, "could not change the memory area while a transfer is in progress");
          allocation = std::move(ma);
          mem_area = ma.mem();
          mem_offset = ma.offset();
          staging_buffer.bind_memory(*ma.mem(), ma.offset());
        }

        /// \brief Start the transfer
        /// \note This is a non-blocking async transfer,
        ///       and from time to time you may want to call continue_transfer()
        ///       to continue the transfer (as the transfer window can be tiny
        ///       and some transfer may be added asynchronically)
        void start()
        {
          if (transfer_in_progress.exchange(true) == true) // we're already transferring !!
            return;

          if (transfer_list.empty())
            return;

          // fill the memory, +create the command buffer
          std::vector<vk::semaphore *> semaphore_vct;
          std::vector<vk::fence *> fence_vct;
          {
            int8_t *memory = (int8_t *)mem_area->map_memory(mem_offset, staging_buffer.size());
            vk::command_buffer_recorder cbr = cmd_buf.begin_recording();
            size_t current_offset = 0;
            int i;

            std::lock_guard<spinlock> _u0(transfer_list_lock);

            // transfer buffers //
            for (i = 0; i < (int)transfer_list.size(); ++i)
            {
              const size_t max_sz = current_offset - staging_buffer.size();
              const size_t cp_sz = (transfer_list[i].data_size - transfer_list[i].data_offset);
              if (max_sz >= cp_sz)
              {
                memcpy(memory + current_offset, transfer_list[i].data + transfer_list[i].data_offset, cp_sz);
                cbr.copy_buffer(staging_buffer, *transfer_list[i].buf, {{ current_offset, transfer_list[i].buf_offset, cp_sz }});

                operator delete(transfer_list[i].data);

                total_size -= cp_sz;
                current_offset += cp_sz;

                if (transfer_list[i].signal_semaphore)
                  semaphore_vct.push_back(transfer_list[i].signal_semaphore);
                if (transfer_list[i].signal_fence)
                  fence_vct.push_back(transfer_list[i].signal_fence);
                if (max_sz == cp_sz)
                  break;
              }
              else
              {
                if (max_sz > 100) // is there enough space ?
                {
                  memcpy(memory + current_offset, transfer_list[i].data + transfer_list[i].data_offset, max_sz);
                  cbr.copy_buffer(staging_buffer, *transfer_list[i].buf, {{ current_offset, transfer_list[i].buf_offset, max_sz }});

                  transfer_list[i].data_offset += max_sz;
                  transfer_list[i].buf_offset += max_sz;
                  total_size -= max_sz;
                }

                // can't copy the whole buffer, don't delete the entry and exit
                --i;
                break;
              }
            }

            // remove in-transfer entries
            if (i >= 0 && !transfer_list.empty())
              transfer_list.erase(transfer_list.begin(), transfer_list.begin() + i);

            // transfer images //
            for (i = 0; i < (int)img_transfer_list.size(); ++i)
            {
              const size_t max_sz = current_offset - staging_buffer.size();
              const size_t cp_sz = (img_transfer_list[i].data_size);
              if (max_sz >= cp_sz)
              {
                memcpy(memory + current_offset, img_transfer_list[i].data, cp_sz);
                if (img_transfer_list[i].layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                {
                  cbr.pipeline_barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                                       { vk::image_memory_barrier(*img_transfer_list[i].img, img_transfer_list[i].layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) });
                }
                cbr.copy_buffer_to_image(staging_buffer, *img_transfer_list[i].img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                         { current_offset, img_transfer_list[i].offset, img_transfer_list[i].size });
                if (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL != img_transfer_list[i].final_layout)
                {
                  cbr.pipeline_barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                                       { vk::image_memory_barrier(*img_transfer_list[i].img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, img_transfer_list[i].final_layout) });
                }

                operator delete(img_transfer_list[i].data);

                total_size -= cp_sz;
                current_offset += cp_sz;

                if (img_transfer_list[i].signal_semaphore)
                  semaphore_vct.push_back(img_transfer_list[i].signal_semaphore);
                if (img_transfer_list[i].signal_fence)
                  fence_vct.push_back(img_transfer_list[i].signal_fence);
                if (max_sz == cp_sz)
                  break;
              }
              else
              {
                --i;
                break; // next time
              }
            }

            // remove in-transfer entries
            if (i >= 0 && !img_transfer_list.empty())
              img_transfer_list.erase(img_transfer_list.begin(), img_transfer_list.begin() + i);
          }

          cmd_buf.end_recording();

          vk::submit_info si;

          si << cmd_buf;
          for (vk::semaphore *it : semaphore_vct)
            si >> *it;
          for (vk::fence *it : fence_vct)
            si >> *it;
          si >> end_fence;

          // start the transfer: flush and unmap
          mem_area->flush();
//           mem_area->unmap_memory();

          tqueue.submit(si);
        }

        /// \brief Continue the transfer (in the case of a multipass transfer)
        /// \return true while the transfer is not finished
        /// \note can be called from another thread if the queue is dedicated to
        ///       transfers
        bool continue_transfer()
        {
          if (end_fence.is_signaled())
          {
            end_fence.reset();
            transfer_in_progress = false;
            start();
            return true;
          }
          return false;
        }

      private:
        vk::device &dev;
        vk::queue &tqueue;
        vk::command_pool &cmd_pool;

        const vk::device_memory *mem_area = nullptr;
        size_t mem_offset = 0;
        std::atomic_bool transfer_in_progress;

        vk::command_buffer cmd_buf;
        vk::buffer staging_buffer;
        vk::fence end_fence;

        spinlock transfer_list_lock;
        std::deque<_data_transfer> transfer_list;
        std::deque<_img_data_transfer> img_transfer_list;
        size_t total_size = 0;

        memory_allocation allocation;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_247872795872822206_2274418095_TRANSFER_HPP__

