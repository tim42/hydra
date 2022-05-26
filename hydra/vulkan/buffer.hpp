//
// file : buffer.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/buffer.hpp
//
// created by : Timothée Feuillet
// date: Fri Aug 26 2016 23:02:00 GMT+0200 (CEST)
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

#ifndef __N_24932140943134326865_101749810_BUFFER_HPP__
#define __N_24932140943134326865_101749810_BUFFER_HPP__

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "../hydra_debug.hpp"

#include "device.hpp"
#include "device_memory.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      class buffer
      {
        public: // advanced
          /// \brief Create from the create info vulkan structure
          buffer(device &_dev, const VkBufferCreateInfo &create_info)
            : dev(_dev), buffer_size(create_info.size)
          {
            check::on_vulkan_error::n_assert_success(dev._vkCreateBuffer(&create_info, nullptr, &vk_buffer));
          }

          /// \brief Create from an already existing vulkan buffer object
          buffer(device &_dev, VkBuffer _vk_buffer, size_t _buffer_size) : dev(_dev), vk_buffer(_vk_buffer), buffer_size(_buffer_size) {}

        public:
          /// \brief Create a buffer (with sharing mode = exclusive)
          buffer(device &_dev, size_t size, VkBufferUsageFlags usage, VkBufferCreateFlags flags = 0)
            : buffer(_dev, VkBufferCreateInfo
          {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, flags,
            size, usage,
            VK_SHARING_MODE_EXCLUSIVE, 0, nullptr
          }) {}
          /// \brief Create a buffer (with sharing mode = concurrent)
          buffer(device &_dev, size_t size, VkBufferUsageFlags usage, const std::vector<uint32_t> &queue_family_indices, VkBufferCreateFlags flags = 0)
            : buffer(_dev, VkBufferCreateInfo
          {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, flags,
            size, usage,
            VK_SHARING_MODE_CONCURRENT, (uint32_t)queue_family_indices.size(), queue_family_indices.data()
          }) {}

          buffer(buffer &&o) : dev(o.dev), vk_buffer(o.vk_buffer), buffer_size(o.buffer_size) { o.vk_buffer = nullptr; }
          buffer &operator = (buffer &&o)
          {
            if (&o == this)
              return *this;
            check::on_vulkan_error::n_assert(&dev != &o.dev, "could not move-assign a buffer from a different vulkan device");

            if (vk_buffer)
              dev._vkDestroyBuffer(vk_buffer, nullptr);

            vk_buffer = o.vk_buffer;
            o.vk_buffer = nullptr;
            buffer_size = o.buffer_size;
            return *this;
          }

          ~buffer()
          {
            if (vk_buffer)
              dev._vkDestroyBuffer(vk_buffer, nullptr);
          }

          /// \brief Bind some memory to the buffer
          void bind_memory(const device_memory &mem, size_t offset)
          {
            check::on_vulkan_error::n_assert_success(dev._vkBindBufferMemory(vk_buffer, mem._get_vk_device_memory(), offset));
          }

          /// \brief Return the buffer size (in byte)
          size_t size() const { return buffer_size; }

          /// \brief Return the memory requirements of the buffer
          VkMemoryRequirements get_memory_requirements() const
          {
            VkMemoryRequirements ret;
            dev._vkGetBufferMemoryRequirements(vk_buffer, &ret);
            return ret;
          }

        public: // advanced
          /// \brief Return the underlying VkBuffer
          VkBuffer _get_vk_buffer() const { return vk_buffer; }
        private:
          device &dev;
          VkBuffer vk_buffer;
          size_t buffer_size;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam


#endif // __N_24932140943134326865_101749810_BUFFER_HPP__

