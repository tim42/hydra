//
// created by : Timothée Feuillet
// date: 2022-7-30
//
//
// Copyright (c) 2022 Timothée Feuillet
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

#include <hydra/vulkan/buffer.hpp>
#include <hydra/vulkan/image.hpp>
#include <hydra/vulkan/image_view.hpp>
#include <hydra/utilities/memory_allocator.hpp>

namespace neam::hydra
{
  struct buffer_holder
  {
    vk::buffer buffer;
    memory_allocation allocation;

    buffer_holder(memory_allocator& allocator, vk::buffer&& _buffer, neam::hydra::allocation_type at = neam::hydra::allocation_type::persistent)
      : buffer(std::move(_buffer))
      , allocation(allocator.allocate_memory(buffer.get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, at))
    {
      buffer.bind_memory(*allocation.mem(), allocation.offset());
    }
    buffer_holder(memory_allocation&& _allocation, vk::buffer&& _buffer)
      : buffer(std::move(_buffer))
      , allocation(std::move(_allocation))
    {
    }
    buffer_holder(buffer_holder&&) = default;
    buffer_holder& operator = (buffer_holder&&) = default;
  };

  struct image_holder
  {
    vk::image image;
    memory_allocation allocation;
    vk::image_view view;

    image_holder(memory_allocator& allocator, vk::device& dev, vk::image&& _image, neam::hydra::allocation_type at = neam::hydra::allocation_type::persistent_optimal_image, VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D)
      : image(std::move(_image))
      , allocation(get_allocation(allocator, at))
      , view(dev, image, view_type)
    {
    }
    image_holder(image_holder&&) = default;
    image_holder& operator = (image_holder&&) = default;

    memory_allocation get_allocation(memory_allocator& allocator, neam::hydra::allocation_type at)
    {
      auto alloc = allocator.allocate_memory(image.get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, at);
      image.bind_memory(*alloc.mem(), alloc.offset());
      return alloc;
    }
  };
}

