//
// file : utilities.hpp (2)
// in : file:///home/tim/projects/hydra/hydra/utilities/utilities.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 27 2016 15:52:45 GMT+0200 (CEST)
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

#ifndef __N_70305883316559647_442029613_UTILITIES_HPP__2___
#define __N_70305883316559647_442029613_UTILITIES_HPP__2___


// Batch transfers asynchronously
#include "transfer.hpp"

// Manage the GPU memory
#include "memory_allocator.hpp"

// An easy way to manage uniform/... buffers
#include "auto_buffer.hpp"

// Avoid loading multiple time the same shader
#include "shader_manager.hpp"

// Avoid creating multiple time the same pipeline and improve pipeline re-use
#include "pipeline_manager.hpp"

#include "vk_resource_destructor.hpp"

#include "image_loader.hpp"

namespace neam
{
  namespace hydra
  {
    
  } // namespace hydra
} // namespace neam

#endif // __N_70305883316559647_442029613_UTILITIES_HPP__2___

