//
// file : command_buffer.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/command_buffer.hpp
//
// created by : Timothée Feuillet
// date: Thu Apr 28 2016 17:28:58 GMT+0200 (CEST)
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

#include "../hydra_debug.hpp"

#include "device.hpp"
#include "command_pool.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      class command_buffer_recorder;
      class render_pass;
      class framebuffer;

      class command_buffer : public cr::mt_checked<command_buffer>
      {
        public: // advanced
          /// \brief Create the command buffer from a vulkan structure
          command_buffer(device& _dev, command_pool& _pool, VkCommandBuffer _cmd_buf)
            : dev(_dev), pool(&_pool), cmd_buf(_cmd_buf)
          {
          }

        public:
          /// \brief Move constructor
          command_buffer(command_buffer&& o)
            : dev(o.dev), pool(o.pool), cmd_buf(o.cmd_buf)
          {
            o.cmd_buf = nullptr;
            o.pool = nullptr;
#if !N_DISABLE_CHECKS
            queue = o.queue;
#endif
          }

          ~command_buffer()
          {
            if (cmd_buf)
            {
              check::debug::n_assert(pool != nullptr, "command_buffer: vk resource is not null but the pool is");
              pool->_free_command_buffer(cmd_buf);
            }
            else
            {
              check::debug::n_assert(pool == nullptr, "command_buffer: vk resource is null but the pool is not");
            }
          }

          /// \brief Start the recording of the command buffer
          /// \note the recording job is done by the command_buffer_recorder instance
          /// \note This is to be used only for primary command buffers
          /// Implemented in command_buffer_recorder.hpp
          command_buffer_recorder begin_recording(VkCommandBufferUsageFlagBits flags = (VkCommandBufferUsageFlagBits)0);

          /// \brief Start the recording of the command buffer
          /// \note the recording job is done by the command_buffer_recorder instance
          /// \note This is to be used only for secondary command buffers
          /// Implemented in command_buffer_recorder.hpp
          command_buffer_recorder begin_recording(bool occlusion_query_enable, VkQueryControlFlags query_flags = 0, VkQueryPipelineStatisticFlags stat_flags = 0, VkCommandBufferUsageFlagBits flags = (VkCommandBufferUsageFlagBits)0);

          /// \brief Start the recording of the command buffer
          /// \note the recording job is done by the command_buffer_recorder instance
          /// \note This is to be used only for secondary command buffers
          /// Implemented in command_buffer_recorder.hpp
          command_buffer_recorder begin_recording(const framebuffer &fb,
                                                  bool occlusion_query_enable = false, VkQueryControlFlags query_flags = 0, VkQueryPipelineStatisticFlags stat_flags = 0,
                                                  VkCommandBufferUsageFlagBits flags = (VkCommandBufferUsageFlagBits)0);

          /// \brief Start the recording of the command buffer
          /// \note the recording job is done by the command_buffer_recorder instance
          /// \note This is to be used only for secondary command buffers
          /// Implemented in command_buffer_recorder.hpp
          command_buffer_recorder begin_recording(const render_pass &rp, uint32_t subpass,
                                                  bool occlusion_query_enable = false, VkQueryControlFlags query_flags = 0, VkQueryPipelineStatisticFlags stat_flags = 0,
                                                  VkCommandBufferUsageFlagBits flags = (VkCommandBufferUsageFlagBits)0);

          /// \brief Start the recording of the command buffer
          /// \note the recording job is done by the command_buffer_recorder instance
          /// \note This is to be used only for secondary command buffers
          /// Implemented in command_buffer_recorder.hpp
          command_buffer_recorder begin_recording(const framebuffer &fb, const render_pass &rp, uint32_t subpass,
                                                  bool occlusion_query_enable = false, VkQueryControlFlags query_flags = 0, VkQueryPipelineStatisticFlags stat_flags = 0,
                                                  VkCommandBufferUsageFlagBits flags = (VkCommandBufferUsageFlagBits)0);

          /// \brief Reset the command buffer
          void reset(VkCommandBufferResetFlags flags = 0) { check::on_vulkan_error::n_assert_success(dev._vkResetCommandBuffer(cmd_buf, flags)); }

          /// \brief End the recording of the command buffer
          void end_recording() { check::on_vulkan_error::n_assert_success(dev._vkEndCommandBuffer(cmd_buf)); }

        public: // advanced
          /// \brief Return the vulkan command buffer
          VkCommandBuffer _get_vk_command_buffer() const { return cmd_buf; }
          device& _get_device() { return dev; }

          void _set_debug_name(const std::string& name)
          {
            dev._set_object_debug_name((uint64_t)cmd_buf, VK_OBJECT_TYPE_COMMAND_BUFFER, name);
          }

        private:
          device& dev;
          command_pool* pool = nullptr;
          VkCommandBuffer cmd_buf;
#if !N_DISABLE_CHECKS
          VkQueue queue = nullptr;
          friend class command_pool;
          friend class submit_info;
#endif
      };



      // ////////////////////////
      // // Implementations // //
      // ////////////////////////

      inline command_buffer command_pool::create_command_buffer(VkCommandBufferLevel level)
      {
        allocated_buffer_counter.fetch_add(1, std::memory_order_release);

        N_MTC_WRITER_SCOPE;

        VkCommandBufferAllocateInfo cmd_cr;
        cmd_cr.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_cr.pNext = nullptr;
        cmd_cr.commandPool = cmd_pool;
        cmd_cr.level = level;
        cmd_cr.commandBufferCount = 1;

        VkCommandBuffer cmd_buf;
        check::on_vulkan_error::n_assert_success(dev._vkAllocateCommandBuffers(&cmd_cr, &cmd_buf));

        command_buffer ret(dev, *this, cmd_buf);
#if !N_DISABLE_CHECKS
        ret.queue = queue;
#endif
        return ret;
      }

      inline std::vector<command_buffer> command_pool::create_command_buffers(uint32_t count, VkCommandBufferLevel level)
      {
        allocated_buffer_counter.fetch_add(count, std::memory_order_release);

        N_MTC_WRITER_SCOPE;
        VkCommandBufferAllocateInfo cmd_cr;
        cmd_cr.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_cr.pNext = nullptr;
        cmd_cr.commandPool = cmd_pool;
        cmd_cr.level = level;
        cmd_cr.commandBufferCount = count;

        std::vector<VkCommandBuffer> vk_cmd_bufs;
        vk_cmd_bufs.resize(count);
        check::on_vulkan_error::n_assert_success(dev._vkAllocateCommandBuffers(&cmd_cr, vk_cmd_bufs.data()));

        std::vector<command_buffer> cmd_bufs;
        cmd_bufs.reserve(count);
        for (VkCommandBuffer it : vk_cmd_bufs)
        {
          cmd_bufs.emplace_back(dev, *this, it);
#if !N_DISABLE_CHECKS
          cmd_bufs.back().queue = queue;
#endif
        }

        return cmd_bufs;
      }
    } // namespace vk
  } // namespace hydra
} // namespace neam



