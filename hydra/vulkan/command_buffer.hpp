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

#ifndef __N_17835239222945519851_2656411714_COMMAND_BUFFER_HPP__
#define __N_17835239222945519851_2656411714_COMMAND_BUFFER_HPP__

#include <vulkan/vulkan.h>

#include "../hydra_exception.hpp"

#include "device.hpp"
#include "command_pool.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      class command_buffer
      {
        public: // advanced
          /// \brief Create the command buffer from a vulkan structure
          command_buffer(device &_dev, command_pool &_pool, VkCommandBuffer _cmd_buf)
            : dev(_dev), pool(_pool), cmd_buf(_cmd_buf)
          {
          }

        public:
          /// \brief Move constructor
          command_buffer(command_buffer &&o)
            : dev(o.dev), pool(o.pool), cmd_buf(o.cmd_buf)
          {
            o.cmd_buf = nullptr;
          }

          ~command_buffer()
          {
            if (cmd_buf)
              dev._vkFreeCommandBuffers(pool._get_vulkan_command_pool(), 1, &cmd_buf);
          }

          

        public: // advanced
          /// \brief Return the vulkan command buffer
          VkCommandBuffer _get_vulkan_command_buffer() const
          {
            return cmd_buf;
          }

        private:
          device &dev;
          command_pool &pool;
          VkCommandBuffer cmd_buf;
      };



      // Implementations //

      command_buffer command_pool::create_command_buffer(VkCommandBufferLevel level)
      {
        VkCommandBufferAllocateInfo cmd_cr;
        cmd_cr.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_cr.pNext = nullptr;
        cmd_cr.commandPool = cmd_pool;
        cmd_cr.level = level;
        cmd_cr.commandBufferCount = 1;

        VkCommandBuffer cmd_buf;
        check::on_vulkan_error::n_throw_exception(dev._vkAllocateCommandBuffers(&cmd_cr, &cmd_buf));

        return command_buffer(dev, *this, cmd_buf);
      }

      std::vector<command_buffer> command_pool::create_command_buffers(size_t count, VkCommandBufferLevel level)
      {
        VkCommandBufferAllocateInfo cmd_cr;
        cmd_cr.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_cr.pNext = nullptr;
        cmd_cr.commandPool = cmd_pool;
        cmd_cr.level = level;
        cmd_cr.commandBufferCount = count;

        std::vector<VkCommandBuffer> vk_cmd_bufs;
        vk_cmd_bufs.resize(count);
        check::on_vulkan_error::n_throw_exception(dev._vkAllocateCommandBuffers(&cmd_cr, vk_cmd_bufs.data()));

        std::vector<command_buffer> cmd_bufs;
        cmd_bufs.reserve(count);
        for (VkCommandBuffer it : vk_cmd_bufs)
          cmd_bufs.emplace_back(dev, *this, it);

        return cmd_bufs;
      }
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_17835239222945519851_2656411714_COMMAND_BUFFER_HPP__

