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

#ifndef __N_28485255642531730625_2390811452_COMMAND_POOL_HPP__
#define __N_28485255642531730625_2390811452_COMMAND_POOL_HPP__

#include <vector>

#include <vulkan/vulkan.h>

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
      class command_pool
      {
        public: // advanced
          command_pool(device &_dev, VkCommandPool _cmd_pool)
            : dev(_dev), cmd_pool(_cmd_pool)
          {
          }

        public:
          /// \brief Move constructor
          command_pool(command_pool &&o)
            : dev(o.dev), cmd_pool(o.cmd_pool)
          {
            o.cmd_pool = nullptr;
          }

          ~command_pool()
          {
            if (cmd_pool)
              dev._vkDestroyCommandPool(cmd_pool, nullptr);
          }

          /// \brief Create a single command buffer
          /// Implemented in command_buffer.hpp
          command_buffer create_command_buffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

          /// \brief Create multiple command buffer
          std::vector<command_buffer> create_command_buffers(size_t count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

          /// \brief Reset the command pool
          /// This will also reset command buffers
          void reset() { dev._vkResetCommandPool(cmd_pool, 0); }

          /// \brief Reset the command pool and return the pool's memory to the system
          /// This will also reset command buffers
          void reset_and_free_memory() { dev._vkResetCommandPool(cmd_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT); }

        public: // advanced
          /// \brief Return the vulkan command pool
          VkCommandPool _get_vulkan_command_pool()
          {
            return cmd_pool;
          }

        private:
          device &dev;
          VkCommandPool cmd_pool;
      };



      // from device.hpp (implementations)

      command_pool device::create_command_pool(temp_queue_familly_id_t queue_id, VkCommandPoolCreateFlags flags)
      {
        queue_desc qd = _get_queue_info(queue_id);
        return create_command_pool(qd, flags);
      }

      command_pool device::create_command_pool(const queue_desc &qd, VkCommandPoolCreateFlags flags)
      {
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = NULL;
        cmd_pool_info.queueFamilyIndex = qd.get_queue_familly_index();
        cmd_pool_info.flags = flags;

        VkCommandPool cmd_pool;
        check::on_vulkan_error::n_throw_exception(_vkCreateCommandPool(&cmd_pool_info, nullptr, &cmd_pool));

        return command_pool(*this, cmd_pool);
      }
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_28485255642531730625_2390811452_COMMAND_POOL_HPP__

