//
// file : memory_allocator.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/memory_allocator.hpp
//
// created by : Timothée Feuillet
// date: Tue Aug 30 2016 18:37:47 GMT+0200 (CEST)
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

#ifndef __N_167156602804017135_753626619_MEMORY_ALLOCATOR_HPP__
#define __N_167156602804017135_753626619_MEMORY_ALLOCATOR_HPP__

#include "../vulkan/device.hpp"
#include "../vulkan/device_memory.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief Represent a memory allocation
    struct memory_allocation
    {
      vk::device_memory *mem;
      size_t offset;
      size_t size;
      uint32_t type_index;
    };

    /// \brief An utility to manage GPU memory and perform less allocation
    /// \note It may not be ultra-efficient but should give correct results
    class memory_allocator
    {
      public:
        /// \brief 
        memory_allocator(vk::device &_dev, size_t _default_chunk_size = 1024 * 1024) : dev(_dev), default_chunk_size(_default_chunk_size) {}

        /// \brief Free the memory
        ~memory_allocator() {}

        /// \brief Allocate some memory
        memory_allocation allocate_memory(const VkMemoryRequirements &reqs, VkMemoryPropertyFlags flags);

        /// \brief Allocate some memory
        memory_allocation allocate_memory(size_t size, uint32_t alignment, uint32_t memory_type_index);

        /// \brief Return a chunk of memory previously allocated
        void free_memory(const memory_allocation &mem);

      private:
        vk::device &dev;
        size_t default_chunk_size;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_167156602804017135_753626619_MEMORY_ALLOCATOR_HPP__

