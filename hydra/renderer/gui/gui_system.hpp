//
// file : gui_system.hpp
// in : file:///home/tim/projects/hydra/hydra/renderer/gui/gui_system.hpp
//
// created by : Timothée Feuillet
// date: Mon Sep 05 2016 15:26:41 GMT+0200 (CEST)
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

#ifndef __N_290289102920029682_364914552_GUI_SYSTEM_HPP__
#define __N_290289102920029682_364914552_GUI_SYSTEM_HPP__

#include <deque>

#include "../../vulkan/buffer.hpp"

namespace neam
{
  namespace hydra
  {
    namespace gui
    {
      /// \brief Handle some things like managing shared buffers and their allocations
      /// As a GUI shouldn't have so many high-poly elements, it has been made for small elements
      /// So sharing buffers between elements can be a good thing
      class system
      {
        public:
          

        private:
          std::deque<vk::buffer> index_buffers;
          std::deque<vk::buffer> uniform_buffers;
          std::deque<vk::buffer> vertex_buffers;
      };
    } // namespace gui
  } // namespace hydra
} // namespace neam

#endif // __N_290289102920029682_364914552_GUI_SYSTEM_HPP__

