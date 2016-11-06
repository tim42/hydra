//
// file : auto_buffer.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/auto_buffer.hpp
//
// created by : Timothée Feuillet
// date: Sat Sep 03 2016 17:59:49 GMT+0200 (CEST)
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

#ifndef __N_13922229531440532597_2242118804_AUTO_BUFFER_HPP__
#define __N_13922229531440532597_2242118804_AUTO_BUFFER_HPP__

#include <deque>

#include "../hydra_exception.hpp"

#include "../vulkan/buffer.hpp"
#include "../vulkan/command_pool.hpp"
#include "../vulkan/command_buffer.hpp"
#include "../vulkan/queue.hpp"
#include "../vulkan/submit_info.hpp"

// If you define HYDRA_AUTO_BUFFER_NO_SMART_SYNC
// it will make a full sync, and not search changes in small areas
// and force a full refresh of the buffer every time
//
// If you define HYDRA_AUTO_BUFFER_SMART_SYNC_FORCE_TRANSFER
// it will only perform transfer operations and not buffer updates on small areas

namespace neam
{
  namespace hydra
  {
    /// \brief Manage a buffer
    /// \todo Use persistence for that. That would awesome (and not so slow)
    class auto_buffer
    {
      private:
        struct data_range
        {
          const void *data;
          size_t size;
          size_t offset;

          // TODO: add optional mutexes
        };

      public:
        constexpr static size_t max_update_size = 4 * 1024;

      public:
        auto_buffer(vk::buffer &&_buf, size_t offset = 0, size_t size = 0)
          : buf(std::move(_buf))
        {
          offset_in_buf = offset;
          if (size > 0)
            size_of_buf = size;
          else
            size_of_buf = buf.size() - offset;

          if (data_cpy)
            delete [] data_cpy;
          data_cpy = new int8_t[size_of_buf + 4];
          memset(data_cpy, 0, size_of_buf);

          watched_data.clear();
        }

        ~auto_buffer()
        {
          if (data_cpy)
            delete data_cpy;
          data_cpy = nullptr;
        }

        void set_buffer(vk::buffer &&_buf, size_t offset = 0, size_t size = 0)
        {
          check::on_vulkan_error::n_assert(offset + size < _buf.size(), "set_buffer: out of buffer bounds");

          buf = std::move(_buf);

          offset_in_buf = offset;
          if (size > 0)
            size_of_buf = size;
          else
            size_of_buf = buf.size() - offset;

          if (data_cpy)
            delete [] data_cpy;
          data_cpy = new int8_t[size_of_buf + 4];
          memset(data_cpy, 0, size_of_buf);

          watched_data.clear();
#ifndef HYDRA_AUTO_BUFFER_NO_SMART_SYNC
          sync_start_offset = 0;
          sync_end_offset = size_of_buf;
#endif
        }

        /// \brief Set some infromation about transfers
        void set_transfer_info(batch_transfers &_btransfers, vk::queue &_subqueue, vk::command_pool &_cmd_pool)
        {
          btransfers = &_btransfers;
          subq = &_subqueue;
          cmd_pool = &_cmd_pool;
        }

        /// \brief Set the semaphore to signal when the transfer is complete
        /// Can be nullptr to disable the feature
        void set_semaphore_to_signal(vk::semaphore *_sema)
        {
          sig_sema = _sema;
        }

        /// \brief Add a value to watch (should be a structure)
        /// \param offset is the position inside the buffer
        template<typename Type>
        void watch(const Type &value, size_t offset)
        {
          watch((const void *)value, sizeof(value), offset);
        }

        /// \brief Add a value to watch (should be a structure)
        /// \param offset is the position inside the buffer
        void watch(const void *data, size_t data_size, size_t offset)
        {
          check::on_vulkan_error::n_assert(offset + data_size < size_of_buf, "watch: trying to write data out of bounds");
          watched_data.push_back({ data, data_size, offset });
        }

        /// \brief Set the data at offset
        /// \note The data is copied
        template<typename Type>
        void set_data(const Type &value, size_t offset)
        {
          set_data((const void *)value, sizeof(value), offset);
        }

        /// \brief Set the data at offset
        /// \note The data is copied
        void set_data(const void *data, size_t data_size, size_t offset)
        {
          check::on_vulkan_error::n_assert(offset + data_size < size_of_buf, "set_data: trying to write data out of bounds");
          memcpy(data_cpy + offset, data, data_size);
#ifndef HYDRA_AUTO_BUFFER_NO_SMART_SYNC
          if (offset < sync_start_offset)
            sync_start_offset = offset;
          if (offset + data_size > sync_end_offset)
            sync_end_offset = offset + data_size;
#endif

        }

        /// \brief Either perform a transfer operation (if needed),
        /// a direct modification (via command buffer, if not so much has changed)
        /// or nothing, if the data hasn't changed
        /// \note Transfer operations are performed asynchronously and require the buffer
        ///       to not be used anywhere else
        void sync(bool force_refresh = false)
        {
#ifndef HYDRA_AUTO_BUFFER_NO_SMART_SYNC
          if (!force_refresh)
          {
            for (const data_range &it : watched_data)
            {
              const int res = memcmp(it.data, data_cpy + it.offset, it.size);
              if (res != 0)
              {
                if (it.offset < sync_start_offset)
                  sync_start_offset = it.offset;
                if (it.offset + it.size > sync_end_offset)
                  sync_end_offset = it.offset + it.size;
              }
            }
            // no need to sync !
            if (sync_start_offset >= sync_end_offset)
            {
              if (sig_sema) // always signal the semaphore
              {
                vk::submit_info si;
                si >> *sig_sema;
                subq->submit(si);
              }
              return;
            }
          }
          else
          {
            sync_start_offset = 0;
            sync_end_offset = size_of_buf;
          }

#ifndef HYDRA_AUTO_BUFFER_SMART_SYNC_FORCE_TRANSFER
          if (sync_end_offset - sync_start_offset < max_update_size)
          {
            sync_start_offset -= sync_start_offset % 4;
            size_t sz = sync_end_offset - sync_start_offset;
            if (sz % 4) sz += 4 - (sz % 4);

            vk::command_buffer cmdb = cmd_pool->create_command_buffer();
            auto rec = cmdb.begin_recording();
            rec.update_buffer(buf, offset_in_buf + sync_start_offset, sz, (uint32_t *)(data_cpy + sync_start_offset));
            cmdb.end_recording();

            vk::submit_info si;
            si << cmdb;
            if (sig_sema)
              si >> *sig_sema;
            subq->submit(si);
          }
          else
          {
#endif
            btransfers->add_transfer(buf, offset_in_buf + sync_start_offset, sync_end_offset - sync_start_offset, data_cpy + sync_start_offset, sig_sema);
#ifndef HYDRA_AUTO_BUFFER_SMART_SYNC_FORCE_TRANSFER
          }
#endif

          // reset ranges
          sync_start_offset = ~0;
          sync_end_offset = 0;

#else // no sync, only transfer
          (void)force_refresh;
          btransfers->add_transfer(buf, offset_in_buf, size_of_buf, data_cpy, sig_sema);
#endif
        }

        /// \brief Implicit cast to a vk::buffer:
        operator vk::buffer &() { return buf; }
        /// \brief Implicit cast to a vk::buffer:
        operator const vk::buffer &() const { return buf; }

        /// \brief Explicit grabbing a reference to the buffer
        vk::buffer &get_buffer() { return buf; }
        /// \brief Explicit grabbing a const reference to the buffer
        const vk::buffer &get_buffer() const { return buf; }

        /// \brief Return the start of the managed area in the buffer
        size_t get_buffer_offset() const { return offset_in_buf; }

        /// \brief Return the size of the managed area in the buffer
        size_t get_area_size() const { return size_of_buf; }

        /// \brief Return the buffer, ready to be attached somewhere else
        vk::buffer &&_extract_buffer()
        {
          delete [] data_cpy;
          data_cpy = nullptr;
          offset_in_buf = 0;
          size_of_buf = 0;
          watched_data.clear();
          return std::move(buf);
        }

        /// \brief
        void clear()
        {
          watched_data.clear();
          sync_start_offset = 0;
          sync_end_offset = size_of_buf;
          memset(data_cpy, 0, size_of_buf);
        }

      private:
        // the buffer
        vk::buffer buf;
        size_t offset_in_buf = 0;
        size_t size_of_buf = 0;

        // the transfer
        batch_transfers *btransfers;
        vk::queue *subq;
        vk::semaphore *sig_sema = nullptr;
        vk::command_pool *cmd_pool;

        // the data
        int8_t *data_cpy = nullptr; // a copy of the data
        std::deque<data_range> watched_data;
#ifndef HYDRA_AUTO_BUFFER_NO_SMART_SYNC
        size_t sync_start_offset = ~0;
        size_t sync_end_offset = 0;
#endif
    };
  } // namespace hydra
} // namespace neam

#endif // __N_13922229531440532597_2242118804_AUTO_BUFFER_HPP__

