//
// file : font_face.hpp
// in : file:///home/tim/projects/hydra/hydra/renderer/gui/font_face.hpp
//
// created by : Timothée Feuillet
// date: Sun Sep 04 2016 20:45:20 GMT+0200 (CEST)
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

#ifndef __N_1690817569182923612_519829399_FONT_FACE_HPP__
#define __N_1690817569182923612_519829399_FONT_FACE_HPP__

#include "../../vulkan/buffer.hpp"
#include "../../vulkan/image.hpp"
#include "../../vulkan/image_view.hpp"

#include "../../utilities/memory_allocation.hpp"

#include "raw_font_face.hpp"

namespace neam
{
  namespace hydra
  {
    namespace gui
    {
      /// \brief Hold all the information that is needed to render a text with
      /// a given font
      class font_face
      {
        private:
          font_face(raw_font_face &&_rff, vk::image &&_font_sdf_img, vk::image_view &&_font_sdf_view, memory_allocation _image_allocation,
                    vk::buffer *_vertex_buffer, size_t _vertex_buffer_offset, size_t _vertex_buffer_entry_count)
            : rff(std::move(_rff)), font_sdf_image(std::move(_font_sdf_img)), font_sdf_image_view(std::move(_font_sdf_view)),
              image_allocation(_image_allocation), vertex_buffer(_vertex_buffer),
              vertex_buffer_offset(_vertex_buffer_offset), vertex_buffer_entry_count(_vertex_buffer_entry_count)
          {
          }

        public:
          font_face(font_face &&o)
            : rff(std::move(o.rff)), font_sdf_image(std::move(o.font_sdf_image)), font_sdf_image_view(std::move(o.font_sdf_image_view)),
              image_allocation(o.image_allocation), vertex_buffer(o.vertex_buffer), vertex_buffer_offset(o.vertex_buffer_offset),
              vertex_buffer_entry_count(o.vertex_buffer_entry_count)
          {
            o.image_allocation = memory_allocation(); // clear its memory_allocation, so that it does not free the image
          }

          ~font_face()
          {
            // free the image
            image_allocation.free();
          }

        public: // static
          /// \brief Return the pipeline_vertex_input_state of the font face
          static vk::pipeline_vertex_input_state get_vertex_input_state()
          {
            return internal::character_nfo::get_vertex_input_state();
          }

          /// \brief Return the pipeline_input_assembly_state of the font face
          static vk::pipeline_input_assembly_state get_input_assembly_state()
          {
            return internal::character_nfo::get_input_assembly_state();
          }

        private:
          raw_font_face rff;

        public:
          vk::image font_sdf_image;
          vk::image_view font_sdf_image_view;
          memory_allocation image_allocation;

          vk::buffer *vertex_buffer;
          size_t vertex_buffer_offset;
          size_t vertex_buffer_entry_count;

          friend class font_manager;
      };
    } // namespace gui
  } // namespace hydra
} // namespace neam

#endif // __N_1690817569182923612_519829399_FONT_FACE_HPP__

