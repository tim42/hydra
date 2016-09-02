//
// file : vulkan.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/vulkan.hpp
//
// created by : Timothée Feuillet
// date: Thu Apr 28 2016 14:43:44 GMT+0200 (CEST)
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

// This folder contains some vulkan wrappers (command pool, command buffer, ...)


#ifndef __N_11194131652893914663_22232226_VULKAN_HPP__
#define __N_11194131652893914663_22232226_VULKAN_HPP__

#include "instance.hpp"
#include "device.hpp"
#include "device_memory.hpp"
#include "queue.hpp"
#include "command_pool.hpp"
#include "descriptor_pool.hpp"
#include "command_buffer.hpp"
#include "command_buffer_recorder.hpp"
#include "fence.hpp"
#include "semaphore.hpp"
#include "event.hpp"
#include "memory_barrier.hpp"
#include "image.hpp"
#include "image_view.hpp"
#include "submit_info.hpp"
#include "shader_module.hpp"
#include "pipeline.hpp"
#include "descriptor_set_layout.hpp"
#include "descriptor_set.hpp"
#include "sampler.hpp"

// extensions:
#include "surface.hpp"
#include "swapchain.hpp"

#include "shader_loaders/spirv_loader.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_11194131652893914663_22232226_VULKAN_HPP__

