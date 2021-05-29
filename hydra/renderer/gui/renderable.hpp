//
// file : renderable.hpp
// in : file:///home/tim/projects/hydra/hydra/renderer/gui/renderable.hpp
//
// created by : Timothée Feuillet
// date: Mon Sep 05 2016 12:49:53 GMT+0200 (CEST)
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

#ifndef __N_23799231911767115854_2058722637_RENDERABLE_HPP__
#define __N_23799231911767115854_2058722637_RENDERABLE_HPP__


#include "../../vulkan/pipeline.hpp"
#include "../../vulkan/command_buffer_recorder.hpp"

namespace neam
{
  namespace hydra
  {
    namespace gui
    {
      /// \brief Describe a renderable component of the GUI
      class renderable
      {
        public:
          /// \brief Return the pipeline (~material)
          virtual const vk::pipeline *get_pipeline() const = 0;
        public: // advanced
          /// \brief Setup what is needed to the command buffer to render the element
          /// \note This method will mostly be a single draw call / push constant if you use the utilities
          /// provided by the gui manager for buffer sharing & management
          virtual void _setup_command_buffer(vk::command_buffer_recorder &cbr) = 0;
      };
    } // namespace gui
  } // namespace hydra
} // namespace neam

#endif // __N_23799231911767115854_2058722637_RENDERABLE_HPP__

