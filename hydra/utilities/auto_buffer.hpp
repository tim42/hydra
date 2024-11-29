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

#pragma once


#include <deque>
#include <type_traits>

#include "../hydra_debug.hpp"

#include "../vulkan/device.hpp"
#include "../vulkan/buffer.hpp"
#include "../vulkan/command_pool.hpp"
#include "../vulkan/command_buffer.hpp"
#include "../vulkan/queue.hpp"
#include "../vulkan/submit_info.hpp"
#include "layout.hpp"

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
    /// \fixme: it's broken. fix it. >:(
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
        auto_buffer(vk::device &_dev, vk::buffer &&_buf, size_t offset = 0, size_t size = 0)
          : dev(_dev), buf(std::move(_buf))
        {
          offset_in_buf = offset;
          if (size > 0)
            size_of_buf = size;
          else
            size_of_buf = buf.size() - offset;

          data_cpy = raw_data::allocate(size_of_buf);
          memset(data_cpy, 0, size_of_buf);

          watched_data.clear();
        }

        ~auto_buffer() = default;

        void set_buffer(vk::buffer &&_buf, size_t offset = 0, size_t size = 0)
        {
          check::on_vulkan_error::n_assert(offset + size < _buf.size(), "set_buffer: out of buffer bounds");

          buf = std::move(_buf);

          offset_in_buf = offset;
          if (size > 0)
            size_of_buf = size;
          else
            size_of_buf = buf.size() - offset;

          data_cpy = raw_data::allocate(size_of_buf);
          memset(data_cpy, 0, size_of_buf);

          watched_data.clear();
#ifndef HYDRA_AUTO_BUFFER_NO_SMART_SYNC
          sync_start_offset = 0;
          sync_end_offset = size_of_buf;
#endif
        }

        /// \brief Set some infromation about transfers
        void set_transfer_info(vk::queue& _subqueue)
        {
          subq = &_subqueue;
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
        size_t watch(const Type &value, size_t offset, buffer_layout layout = buffer_layout::packed)
        {
          return watch((const void *)&value, sizeof(value), align<Type>(layout, offset));
        }

        /// \brief Add a value to watch (should be a structure)
        /// \param offset is the position inside the buffer
        /// \note no alignment performed
        size_t watch(const void *data, size_t data_size, size_t offset)
        {
          check::on_vulkan_error::n_assert(offset + data_size < size_of_buf, "watch: trying to write data out of bounds");
          watched_data.push_back({ data, data_size, offset });
          return offset + data_size;
        }

        /// \brief Set the data at offset
        /// \note The data is copied
        /// \note alignment is done for vector/matrices types (for floats)
        template<typename Type>
        void set_data(const Type &value, size_t offset, buffer_layout layout = buffer_layout::packed)
        {
          set_data((const void *)value, sizeof(value), align<Type>(layout, offset));
        }

        /// \brief Set the data at offset
        /// \note The data is copied
        void set_data(const void *data, size_t data_size, size_t offset)
        {
          check::on_vulkan_error::n_assert(offset + data_size < size_of_buf, "set_data: trying to write data out of bounds");
          memcpy((uint8_t*)data_cpy.get() + offset, data, data_size);
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
        void sync(transfer_context& txctx, bool force_refresh = false)
        {
#ifndef HYDRA_AUTO_BUFFER_NO_SMART_SYNC
          if (!force_refresh)
          {
            for (const data_range &it : watched_data)
            {
              const int res = memcmp(it.data, (uint8_t*)data_cpy.get() + it.offset, it.size);
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

          apply();

#ifndef HYDRA_AUTO_BUFFER_SMART_SYNC_FORCE_TRANSFER
          if (sync_end_offset - sync_start_offset < max_update_size/* || true*/)
          {
            sync_start_offset -= sync_start_offset % 4;
            size_t sz = sync_end_offset - sync_start_offset;
            if (sz % 4) sz += 4 - (sz % 4);

            vk::command_buffer cmdb(cmd_pool->create_command_buffer());
            auto rec = cmdb.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            rec.update_buffer(buf, offset_in_buf + sync_start_offset, sz, (uint32_t *)((uint8_t*)data_cpy.get() + sync_start_offset));
            cmdb.end_recording();

            vk::submit_info si;
            si << cmdb;
            if (sig_sema)
              si >> *sig_sema;
            vk::fence end_fence(dev);
            si >> end_fence;
            subq->submit(si);
            vrd->postpone_destruction(*subq, std::move(end_fence), std::move(cmdb));
          }
          else
#endif
          {
            btransfers->add_transfer(buf, offset_in_buf + sync_start_offset, sync_end_offset - sync_start_offset, (uint8_t*)data_cpy.get() + sync_start_offset, subq->get_queue_familly_index(), sig_sema);
          }

          // reset ranges
          sync_start_offset = ~0;
          sync_end_offset = 0;

#else // no sync, only transfer
          (void)force_refresh;
          apply();
          txctx.acquire(buf, *subq);
          txctx.transfer(buf, data_cpy.duplicate(), offset_in_buf);
          txctx.release(buf, *subq, sig_sema);
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
          data_cpy.reset();
          offset_in_buf = 0;
          size_of_buf = 0;
          watched_data.clear();
          return std::move(buf);
        }

        /// \brief clear the buffer
        void clear()
        {
          watched_data.clear();
#ifndef HYDRA_AUTO_BUFFER_NO_SMART_SYNC
          sync_start_offset = 0;
          sync_end_offset = size_of_buf;
#endif
          memset(data_cpy, 0, size_of_buf);
        }

        template<typename Type>
        static size_t align(buffer_layout layout, size_t offset)
        {
          if (layout == buffer_layout::packed)
            return offset;

          size_t align = 1;
          if (layout == buffer_layout::std140)
          {
            if constexpr (std::is_same_v<Type, glm::vec2> || std::is_same_v<Type, glm::uvec2> || std::is_same_v<Type, glm::ivec2>)
              align = sizeof(Type);
            else if constexpr (std::is_same_v<Type, glm::vec3> || std::is_same_v<Type, glm::vec4>
              || std::is_same_v<Type, glm::uvec3> || std::is_same_v<Type, glm::uvec4>
              || std::is_same_v<Type, glm::ivec3> || std::is_same_v<Type, glm::ivec4>)
              align = sizeof(Type::value_type * 4);
            else if constexpr (std::is_same_v<Type, glm::mat3> || std::is_same_v<Type, glm::mat4>)
              align = sizeof(Type::value_type * 4);
            else
              align = sizeof(Type);
            return ((offset + align - 1) / align) * align;
          }
          else if (layout == buffer_layout::std430)
          {
            if constexpr (std::is_same_v<Type, glm::vec2> || std::is_same_v<Type, glm::uvec2> || std::is_same_v<Type, glm::ivec2>)
              align = sizeof(Type);
            else if constexpr (std::is_same_v<Type, glm::vec3> || std::is_same_v<Type, glm::vec4>
              || std::is_same_v<Type, glm::uvec3> || std::is_same_v<Type, glm::uvec4>
              || std::is_same_v<Type, glm::ivec3> || std::is_same_v<Type, glm::ivec4>)
              align = sizeof(Type);
            else if constexpr (std::is_same_v<Type, glm::mat3>)
              align = sizeof(Type::value_type * 3);
            else if constexpr (std::is_same_v<Type, glm::mat4>)
              align = sizeof(Type::value_type * 4);
            else
              align = sizeof(Type);
            return ((offset + align - 1) / align) * align;
          }
          return offset;
        }

      private:
        void apply()
        {
          for (auto &it : watched_data)
          {
            memcpy((uint8_t*)data_cpy.get() + it.offset, it.data, it.size);
          }
        }

      private:
        // the buffer
        vk::device& dev;
        vk::buffer buf;
        size_t offset_in_buf = 0;
        size_t size_of_buf = 0;

        // the transfer
        vk::queue* subq;
        vk::semaphore* sig_sema = nullptr;

        // the data
        raw_data data_cpy; // a copy of the data
        std::deque<data_range> watched_data;
#ifndef HYDRA_AUTO_BUFFER_NO_SMART_SYNC
        size_t sync_start_offset = ~0;
        size_t sync_end_offset = 0;
#endif
    };
  } // namespace hydra
} // namespace neam



