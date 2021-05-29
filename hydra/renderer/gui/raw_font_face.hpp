//
// file : raw_font_face.hpp
// in : file:///home/tim/projects/hydra/hydra/renderer/gui/raw_font_face.hpp
//
// created by : Timothée Feuillet
// date: Sun Sep 04 2016 16:50:35 GMT+0200 (CEST)
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

#ifndef __N_483268558737461_676529784_RAW_FONT_FACE_HPP__
#define __N_483268558737461_676529784_RAW_FONT_FACE_HPP__

#include <fstream>

#include <glm/glm.hpp>

#include "../../tools/chrono.hpp"
#include "../../tools/ct_string.hpp"

#include "../../hydra_reflective.hpp"
#include "../../hydra_exception.hpp"

#include "../../utilities/image_loader.hpp"

#include "../../vulkan/pipeline_vertex_input_state.hpp"
#include "../../vulkan/pipeline_input_assembly_state.hpp"

namespace neam
{
  namespace hydra
  {
    namespace gui
    {
      namespace internal
      {
        /// \brief Vertex data for text
        struct character_nfo
        {
          // homogenous coordinates [0-1] (uv coordinates)
          // {0,0} is 'top-left'
          glm::vec2 lower_pos; // +---
          glm::vec2 upper_pos; // ---+

          // where is the range lower_pos/upper_pos is located the original glyph (for X: |----+---------+----|)
          //                                                                               dt.x           dt.x
          glm::vec2 dt; // in homogenous coordinates.

          glm::vec2 left_top; // almost as DT, but for positioning the upper left corner in homogenous coordinates

          float x_inc;


          // helper function to get the vertex input state
          static vk::pipeline_vertex_input_state get_vertex_input_state()
          {
            vk::pipeline_vertex_input_state pvis;
            pvis.add_binding_description(0, sizeof(character_nfo), VK_VERTEX_INPUT_RATE_VERTEX);
            pvis.add_attribute_description(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(character_nfo, lower_pos));
            pvis.add_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(character_nfo, upper_pos));
            pvis.add_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(character_nfo, dt));
            pvis.add_attribute_description(0, 3, VK_FORMAT_R32G32_SFLOAT, offsetof(character_nfo, left_top));
            pvis.add_attribute_description(0, 4, VK_FORMAT_R32_SFLOAT, offsetof(character_nfo, x_inc));
            return pvis;
          }

          // helper function to get the input assembly state
          static vk::pipeline_input_assembly_state get_input_assembly_state()
          {
            return vk::pipeline_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
          }
        };
      } // namespace internal

      /// \brief A font face.
      /// It describes load and store operations
      /// It uses for now the bfont 0.1 format (used by YägGLer), but this may soon change.
      ///
      /// \todo in bfont 0.2:
      ///         - dynamic entries (remove the 256-entries hardcoded table),
      ///         - encode images using 8-bit RGBA, four entries per emplacement (it may improve speed)
      class raw_font_face
      {
        public:
          // NOTE: the version is part of the magic.
          static constexpr string_t magic_line = "[bfont 0.1]";

        public:
          raw_font_face()
          {
            NHR_MONITOR_THIS_NAME("neam::hydra::gui::raw_font_face::raw_font_face");

            for (uint16_t i = 0; i < 256; ++i)
            {
              table[i].lower_pos = glm::vec2(i % 16, i / 16) / 16.f;
              table[i].upper_pos = glm::vec2(i % 16 + 1, i / 16 + 1) / 16.f;
              table[i].dt = glm::vec2(0, 0);
              table[i].left_top = glm::vec2(0, 0);
              table[i].x_inc = 1.f / 16.f;
            }
            image_data = nullptr;
            image_size = glm::uvec2(0, 0);
          }

          raw_font_face(raw_font_face &&o)
            : image_data(o.image_data)
          {
            o.image_data = nullptr;

            memcpy(table, o.table, sizeof(table));
          }

          raw_font_face &operator = (raw_font_face && o)
          {
            if (&o == this)
              return *this;

            delete image_data;
            image_data = (o.image_data);
            o.image_data = nullptr;

            memcpy(table, o.table, sizeof(table));
            return *this;
          }

          ~raw_font_face()
          {
            delete image_data;
          }

          // init the font from a file containing the infos of the font
          // (bleunw fonts are a couple of a .bfont and a .png (or .whatever) files)
          // I now, this is a crapy loader. But it works. And is simple.
          raw_font_face(const std::string &init_file, image_loader *loader)
          {
            // bfont format:

            // magic line
            // font texture file, _relative to the binary_
            // char-value [ x y ]   [ x y ]  [ x y ] [ x y ] x_inc
            //           lower_pos upper_pos   dt      l-t

            NHR_MONITOR_THIS_NAME("neam::hydra::gui::raw_font_face::raw_font_face");

#ifndef HYDRA_NO_MESSAGES
            neam::cr::chrono timer;
#endif

            std::string line;
            std::ifstream file(init_file);
            check::on_vulkan_error::n_assert(file.is_open(), init_file + ": file not found");

            // get the magic line
            std::getline(file, line);
            check::on_vulkan_error::n_assert(line == magic_line, "bad magic in bfont file " + init_file);

            // get the font texture
            std::string font_texture;
            std::getline(file, font_texture);

            image_data = loader->load_image_from_file(font_texture, VK_FORMAT_R8_UNORM, image_size);

            // load the table
            size_t line_num = 0;
            while (std::getline(file, line))
            {
              char discard;
              size_t index;
              float x_inc;
              glm::vec2 upper;
              glm::vec2 lower;
              glm::vec2 dt;
              glm::vec2 left_top;
              ++line_num;

              if (line.size())
              {
                std::istringstream is(line);

                // char-value [ x y ]   [ x y ]  [ x y ] [ x y ] x_inc
                //           lower_pos upper_pos   dt      l-t
                is >> index
                   >> discard >> lower.x >> lower.y >> discard
                   >> discard >> upper.x >> upper.y >> discard
                   >> discard >> dt.x >> dt.y >> discard
                   >> discard >> left_top.x >> left_top.y >> discard
                   >> x_inc;

                check::on_vulkan_error::n_assert(index < 256, "index greater than the **hardcoded** array size of 256");

                table[index].lower_pos = lower;
                table[index].upper_pos = upper;
                table[index].dt = dt;
                table[index].left_top = left_top;
                table[index].x_inc = x_inc;
              }
            }

#ifndef YAGGLER_NO_MESSAGES
              neam::cr::out.debug() << LOGGER_INFO_TPL(init_file, 0) << "loaded font face in " << timer.delta() << " seconds" << std::endl;
#endif

          }

          // write the conf into the out_file and the texture into the font_texture
          // The output file could then be loaded with raw_font_face(file).
          bool write_out_raw_font_face(const std::string &out_file, const std::string &font_texture/*, image_writer *writer*/) const
          {
            // bfont format:

            // magic line
            // font texture file, _relative to the binary_
            // char-value [ x y ]   [ x y ]  [ x y ] [ x y ] x_inc
            //           lower_pos upper_pos   dt      l-t

            NHR_MONITOR_THIS_FUNCTION(neam::hydra::gui::raw_font_face::write_out_raw_font_face);

            std::ofstream file(out_file);
            if (!file)
              return false;

            // write the magic line and the texture file line
            file << magic_line << "\n"
                 << font_texture << "\n";

            // char-value [ x y ]   [ x y ]  [ x y ] [ x y ] x_inc
            //           lower_pos upper_pos   dt      l-t

            for (size_t i = 0; i < 256; ++i)
            {
              file << i << " [" << table[i].lower_pos.x << " " << table[i].lower_pos.y << " ] [ "
                   << table[i].upper_pos.x << " " << table[i].upper_pos.y << " ] [ "
                   << table[i].dt.x << " " << table[i].dt.y << " ] [ "
                   << table[i].left_top.x << " " << table[i].left_top.y << " ] "
                   << table[i].x_inc << "\n";
            }
            file.close();

            // use the image writer here.

            return true;
          }

        public:
          internal::character_nfo table[256]; // TODO: change this for a dynamic map
          glm::uvec2 image_size;
          uint8_t *image_data = nullptr; // 8b-GREY (R-channel only) raw data
      };
    } // namespace gui
  } // namespace hydra
} // namespace neam

#endif // __N_483268558737461_676529784_RAW_FONT_FACE_HPP__

