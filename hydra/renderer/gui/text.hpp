//
// file : text.hpp
// in : file:///home/tim/projects/hydra/hydra/renderer/gui/text.hpp
//
// created by : Timothée Feuillet
// date: Sun Sep 04 2016 22:05:28 GMT+0200 (CEST)
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

#ifndef __N_9654152302349810866_176996430_TEXT_HPP__
#define __N_9654152302349810866_176996430_TEXT_HPP__

#include <string>

#include "../../vulkan/device.hpp"
#include "../../vulkan/command_buffer.hpp"
#include "../../vulkan/command_buffer_recorder.hpp"
#include "../../vulkan/buffer.hpp"
#include "../../vulkan/pipeline.hpp"

#include "../../utilities/memory_allocator.hpp"

#include "renderable.hpp"
#include "font_face.hpp"

namespace neam
{
  namespace hydra
  {
    namespace gui
    {
      /// \brief A higher level class whom main purpose is to draw text
      /// A text uses a total of three buffer. The vertex buffer (defined by the font face),
      /// The index buffer (the string, but with 16bit values), and an uniform buffer
      /// that only contains vec2 (and the number of vec2 is the length of the string)
      ///
      /// The font faces are put in big vertex buffers (so they share the same vertex buffer)
      /// Index buffer and Uniform buffer are fused into some big buffers managed by the gui manager
      class text : public renderable
      {
        public:
          text(vk::device &dev, std::string &_text, size_t max);
          text(text &&o);
          text &operator = (text &&o);

          /// \brief Set the text
          text &operator = (const std::string &str) { set_text(str); return *this; }

          /// \brief Set the text
          void set_text(const std::string &str)
          {
            text_str = str;
            need_rebuild = true;
          }

          /// \brief Return the text
          std::string get_text() const { return text_str; }

          /// \brief Return the pipeline (~material) used to draw the text
          virtual const vk::pipeline *get_pipeline() const final { return pipeline; }

          /// \brief Set the pipeline (~material) used to draw the text
          void set_pipeline(vk::pipeline &p) { pipeline = &p; }

          /// \brief Return the font face (if any)
          const font_face *get_font_face() const { return ff; }

          /// \brief Set the font face
          void set_font_face(font_face &_ff) { ff = &_ff; }

        public: // advanced
          /// \brief Setup what is needed to the command buffer to render the text
          virtual void _setup_command_buffer(vk::command_buffer_recorder &cbr) final
          {
            if (ff && pipeline)
            {
              if (cbr.get_last_bound_pipeline() != pipeline) // suboptimal :/ (having a gui manager that does this for me ?)
                cbr.bind_pipeline(*pipeline);

              

              cbr.draw_indexed(text_str.size(), 1, 0, ff->vertex_buffer_offset, 0);
            }
            // DEBUG: print a message telling it can't be rendered (missing font face / missing pipeline)
          }

        private:
          vk::device &dev;
          const vk::pipeline *pipeline;
          const font_face *ff = nullptr; // a pointer, 'cause this may be changed

          std::string text_str;
          bool need_rebuild = true;
      };
    } // namespace gui
  } // namespace hydra
} // namespace neam

#endif // __N_9654152302349810866_176996430_TEXT_HPP__

