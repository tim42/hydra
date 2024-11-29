//
// file : command_pool.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/command_pool.hpp
//
// created by : Timothée Feuillet
// date: Thu Apr 28 2016 14:38:04 GMT+0200 (CEST)
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


#include <vulkan/vulkan.h>
#include <ntools/mt_check/mt_check_base.hpp>

#include "../hydra_types.hpp"

#include "device.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      class command_buffer;

      /// \brief Wraps more or less a vulkan command pool object
      class command_pool : public cr::mt_checked<command_pool>
      {
        public: // advanced
          command_pool(device& _dev, VkCommandPool _cmd_pool)
            : dev(_dev), cmd_pool(_cmd_pool)
          {
          }

        public:
          /// \brief Move constructor
          command_pool(command_pool&& o)
            : dev(o.dev), cmd_pool(o.cmd_pool)
          {
            check::debug::n_assert(o.get_allocated_buffer_count() == 0, "command_buffer_pool: moving a pool that is still tracked by command buffers");
            o.cmd_pool = nullptr;
#if !N_DISABLE_CHECKS
            queue = o.queue;
#endif
          }

          ~command_pool()
          {
            check::debug::n_assert(get_allocated_buffer_count() == 0, "command_buffer_pool: pool is to be destructed but some ({}) command buffer are still allocated", get_allocated_buffer_count());
            if (cmd_pool)
              dev._vkDestroyCommandPool(cmd_pool, nullptr);
          }

          /// \brief Create a single command buffer
          /// Implemented in command_buffer.hpp
          [[nodiscard]] command_buffer create_command_buffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

          /// \brief Create multiple command buffer
          [[nodiscard]] std::vector<command_buffer> create_command_buffers(uint32_t count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

          /// \brief Reset the command pool
          /// This will also reset command buffers
          void reset()
          {
            N_MTC_WRITER_SCOPE;
            dev._vkResetCommandPool(cmd_pool, 0);
            allocated_buffer_counter = 0;
          }

          /// \brief Reset the command pool and return the pool's memory to the system
          /// This will also reset command buffers
          void reset_and_free_memory()
          {
            N_MTC_WRITER_SCOPE;
            dev._vkResetCommandPool(cmd_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
            allocated_buffer_counter = 0;
          }

        public: // advanced
          /// \brief Return the vulkan command pool
          VkCommandPool _get_vulkan_command_pool() const
          {
            return cmd_pool;
          }

          void _set_debug_name(const std::string& name)
          {
            dev._set_object_debug_name((uint64_t)cmd_pool, VK_OBJECT_TYPE_COMMAND_POOL, name);
          }

          void _free_command_buffer(VkCommandBuffer cmd_buf)
          {
            N_MTC_WRITER_SCOPE;
            dev._vkFreeCommandBuffers(cmd_pool, 1, &cmd_buf);
            allocated_buffer_counter.fetch_sub(1, std::memory_order_release);
          }

          uint32_t get_allocated_buffer_count() const { return allocated_buffer_counter.load(std::memory_order_acquire); }

        private:
          device &dev;
          VkCommandPool cmd_pool;
#if !N_DISABLE_CHECKS
          VkQueue queue = nullptr;
          friend class queue;
#endif
          std::atomic<uint32_t> allocated_buffer_counter;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



