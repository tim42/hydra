//
// file : device_memory.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/device_memory.hpp
//
// created by : Timothée Feuillet
// date: Mon May 02 2016 12:00:16 GMT+0200 (CEST)
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

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps an area of the device's memory
      /// \note The object can exists in an uninitialized state
      class device_memory
      {
        public: // advanced
          /// \brief Construct the device memory from a vulkan object
          device_memory(device &_dev, VkDeviceMemory _vk_memory, size_t _size)
           : dev(_dev), vk_memory(_vk_memory), size(_size)
          {
          }

        public:
          /// \brief Create an uninitialized object (no memory is allocated)
          device_memory(device &_dev)
           : dev(_dev), vk_memory(VK_NULL_HANDLE), size(0)
          {
          }

          /// \brief Move constructor
          device_memory(device_memory &&o)
           : dev(o.dev), vk_memory(o.vk_memory), size(o.size), mapped_memory(o.mapped_memory)
          {
            o.vk_memory = nullptr;
            o.size = 0;
            o.mapped_memory = nullptr;
          }

          /// \brief Move-assignment operator
          device_memory &operator = (device_memory &&o)
          {
            check::on_vulkan_error::n_assert(&o.dev == &this->dev, "trying to change logical device when move-assigning a device_memory");
            if (&o != this)
            {
              free();

              size = o.size;
              vk_memory = o.vk_memory;
              mapped_memory = o.mapped_memory;
              o.vk_memory = nullptr;
              o.size = 0;
              o.mapped_memory = nullptr;
            }
            return *this;
          }

          ~device_memory()
          {
            free();
          }

          /// \brief Allocate some memory and return a new device_memory instance to wrap that memory
          static device_memory allocate(device &dev, const VkMemoryRequirements &mem_reqs, VkFlags required_memory_flags)
          {
            device_memory dm(dev);

            dm.allocate(mem_reqs, required_memory_flags);
            return dm;
          }

          /// \brief Allocate some memory and return a new device_memory instance to wrap that memory
          static device_memory allocate(device &dev, size_t size, size_t memory_type_index)
          {
            device_memory dm(dev);

            dm.allocate(size, memory_type_index);
            return dm;
          }

          /// \brief Allocate some memory and return a new device_memory instance to wrap that memory
          static device_memory allocate(device &dev, size_t size, VkMemoryPropertyFlags required_memory_flags, uint32_t memory_type_bits)
          {
            device_memory dm(dev);

            dm.allocate(size, required_memory_flags, memory_type_bits);
            return dm;
          }

          /// \brief Return true if the instance wraps an allocated area of the memory device
          bool is_allocated() const
          {
            return vk_memory != VK_NULL_HANDLE;
          }

          /// \brief Return the size of the allocated memory
          size_t get_size() const
          {
            return size;
          }

          /// \brief Allocate some memory on the device
          void allocate(const VkMemoryRequirements &mem_reqs, VkMemoryPropertyFlags required_memory_flags)
          {
            allocate(mem_reqs.size, required_memory_flags, mem_reqs.memoryTypeBits);
          }

          /// \brief Return the memory type index corresponding to the paramerters
          /// \return the memory type index if found, -1 otherwise
          static int get_memory_type_index(device &dev, VkMemoryPropertyFlags required_memory_flags, uint32_t memory_type_bits)
          {
            int memory_type_index = -1;
            // search for the memory type index
            const VkPhysicalDeviceMemoryProperties &dmp = dev.get_physical_device().get_memory_property();
            for (size_t i = 0; i < dmp.memoryTypeCount; i++)
            {
              if ((memory_type_bits & 1) == 1)
              {
                // Type is available, does it match user properties?
                // TODO: perfect match (if possible)
                if ((dmp.memoryTypes[i].propertyFlags & required_memory_flags) == required_memory_flags)
                {
                  memory_type_index = i;
                  break;
                }
              }
              memory_type_bits >>= 1;
            }
            return memory_type_index;
          }

          static uint32_t get_memory_type_count(device &dev)
          {
            const VkPhysicalDeviceMemoryProperties &dmp = dev.get_physical_device().get_memory_property();
            return dmp.memoryTypeCount;
          }

          /// \brief Allocate some memory on the device
          void allocate(size_t _size, VkMemoryPropertyFlags required_memory_flags, uint32_t memory_type_bits)
          {
            size_t memory_type_index = get_memory_type_index(dev, required_memory_flags, memory_type_bits);

            check::on_vulkan_error::n_assert(memory_type_index != (size_t)-1, "could not find a suitable memory type to allocate");

            allocate(_size, memory_type_index);
          }

          /// \brief Allocate some memory on the device
          void allocate(size_t _size, size_t memory_type_index)
          {
            free();

            VkMemoryAllocateInfo mem_alloc;

            mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            mem_alloc.pNext = NULL;
            mem_alloc.allocationSize = _size;
            mem_alloc.memoryTypeIndex = memory_type_index;

            check::on_vulkan_error::n_assert_success(dev._vkAllocateMemory(&mem_alloc, nullptr, &vk_memory));

#ifndef HYDRA_NO_MESSAGES
//             neam::cr::out().debug("allocating a chunk of {} bytes on the gpu", _size);
#endif
            size = _size;
          }

          /// \brief Free the allocated memory
          void free()
          {
            if (vk_memory != VK_NULL_HANDLE)
            {
              unmap_memory();
              dev._vkFreeMemory(vk_memory, nullptr);
              vk_memory = VK_NULL_HANDLE;
              size = 0;
            }
          }

          /// \brief Map the device memory and return a pointer to that area
          /// You can request only a specific area, setting  offset
          void *map_memory(size_t offset = 0) const
          {
            // search in the mapped areas:
            if (mapped_memory != nullptr)
              return (uint8_t*)mapped_memory + offset;

            dev._vkMapMemory(vk_memory, 0, VK_WHOLE_SIZE, 0, &mapped_memory);

            return (uint8_t*)mapped_memory + offset;
          }

          /// \brief Unmap the memory
          void unmap_memory() const
          {
            if (mapped_memory != nullptr)
            {
              mapped_memory = nullptr;
              dev._vkUnmapMemory(vk_memory);
            }
          }

          /// \brief Write the changes of all mapped areas to the device
          void flush() const
          {
            VkMappedMemoryRange mmr;
            mmr.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mmr.pNext = nullptr;
            mmr.memory = vk_memory;
            mmr.offset = 0;
            mmr.size = VK_WHOLE_SIZE;
            check::on_vulkan_error::n_check_success(dev._vkFlushMappedMemoryRanges(1, &mmr));
          }

          /// \brief Write the changes to the device
          void flush(void* memory, size_t mem_size = 0, bool invalidate = false) const
          {
            if (mapped_memory != nullptr)
            {
              const size_t flush_to = dev.get_physical_device().get_limits().nonCoherentAtomSize;
              // compute the offset:
              size_t offset = (uint8_t*)memory - (uint8_t*)mapped_memory;
              if (offset < flush_to)
                offset = 0;
              else if (offset % flush_to)
                offset -= offset % flush_to;
              // compute the size of the range:
              size_t actual_size = ((uint8_t*)memory + mem_size) - ((uint8_t*)mapped_memory + offset);
              if (actual_size % flush_to)
                actual_size += flush_to - actual_size % flush_to;
              if (offset + actual_size > size)
                actual_size = size - offset;

              VkMappedMemoryRange mmr;
              mmr.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
              mmr.pNext = nullptr;
              mmr.memory = vk_memory;
              mmr.offset = offset;
              mmr.size = actual_size;

              check::on_vulkan_error::n_check_success(dev._vkFlushMappedMemoryRanges(1, &mmr));
              if (invalidate)
                check::on_vulkan_error::n_check_success(dev._vkInvalidateMappedMemoryRanges(1, &mmr));
              return;
            }
          }


          /// \brief Invalidate all the mapped areas
          /// After invalidating a mapped memory area, that area should
          /// be considered stale
          void invalidate() const
          {
            VkMappedMemoryRange mmr;
            mmr.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mmr.pNext = nullptr;
            mmr.memory = vk_memory;
            mmr.offset = 0;
            mmr.size = VK_WHOLE_SIZE;
            check::on_vulkan_error::n_check_success(dev._vkInvalidateMappedMemoryRanges(1, &mmr));
          }


        public: // advanced
          /// \brief Return the vulkan device memory object
          VkDeviceMemory _get_vk_device_memory() const
          {
            return vk_memory;
          }

        private:
          device &dev;
          VkDeviceMemory vk_memory;
          size_t size;

          mutable void* mapped_memory = nullptr;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



