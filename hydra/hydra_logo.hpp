//
// file : hydra_logo.hpp
// in : file:///home/tim/projects/hydra/hydra/hydra_logo.hpp
//
// created by : Timothée Feuillet
// date: Fri Sep 02 2016 19:41:28 GMT+0200 (CEST)
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

#ifndef __N_200324051639512063_182864571_HYDRA_LOGO_HPP__
#define __N_200324051639512063_182864571_HYDRA_LOGO_HPP__

#include <cstdint>  // uint*
#include <cstddef>  // size_t
#include <cstring>  // memset

namespace neam
{
  namespace hydra
  {
    /// \brief Generate the RGBA logo of hydra
    /// \param[out] pixels Where the image will be written. Must have a size of at least icon_sz * icon_sz * 4
    /// \param icon_sz The size of the image. Must be a power of 2 and greater than 16.
    /// \param glyph_count The number of glyph the image will have (must be lower than 5 and lower than 4 if icon_sz is 16)
    static uint8_t *generate_rgba_logo(uint8_t *pixels, size_t icon_sz = 256, size_t glyph_count = 4)
    {
      const uint8_t hydra_logo[] = {0x7D, 0x3D, 0xEB, 0x5F, 0x7B};

      if (icon_sz < 16) return pixels; // not possible
      if (glyph_count < 1 || glyph_count > sizeof(hydra_logo)) glyph_count = 4;
      if (icon_sz == 16 && glyph_count > 4) glyph_count = 4;

      const size_t sq_sz = icon_sz / (glyph_count * 4) ;
      const size_t y_pos = (glyph_count == 1 ? (sq_sz / 2) : (icon_sz / 2 - (sq_sz / 2 + 1)));
      const size_t x_pos = sq_sz / 2;
      memset(pixels, 0, icon_sz * icon_sz * 4);

      for (size_t i = 0; i < glyph_count; ++i)
      {
        for (size_t y = 0; y < sq_sz; ++y)
        {
          for (size_t x = 0; x < sq_sz; ++x)
          {
            size_t bidx = (y_pos + y) * icon_sz * 4 + (x_pos + x + 4 * sq_sz * i) * 4;
            pixels[bidx + 3 + 0 * sq_sz * 4 + 0 * icon_sz * sq_sz * 4] = hydra_logo[i] & 0x01 ? 255 : 0;
            pixels[bidx + 3 + 1 * sq_sz * 4 + 0 * icon_sz * sq_sz * 4] = hydra_logo[i] & 0x02 ? 255 : 0;
            pixels[bidx + 3 + 2 * sq_sz * 4 + 0 * icon_sz * sq_sz * 4] = hydra_logo[i] & 0x04 ? 255 : 0;
            pixels[bidx + 3 + 0 * sq_sz * 4 + 1 * icon_sz * sq_sz * 4] = hydra_logo[i] & 0x08 ? 255 : 0;
            pixels[bidx + 3 + 1 * sq_sz * 4 + 1 * icon_sz * sq_sz * 4] = hydra_logo[i] & 0x10 ? 255 : 0;
            pixels[bidx + 3 + 2 * sq_sz * 4 + 1 * icon_sz * sq_sz * 4] = hydra_logo[i] & 0x20 ? 255 : 0;
            pixels[bidx + 3 + 0 * sq_sz * 4 + 2 * icon_sz * sq_sz * 4] = hydra_logo[i] & 0x40 ? 255 : 0;
            pixels[bidx + 3 + 1 * sq_sz * 4 + 2 * icon_sz * sq_sz * 4] = hydra_logo[i] & 0x80 ? 255 : 0;
            pixels[bidx + 3 + 2 * sq_sz * 4 + 2 * icon_sz * sq_sz * 4] = 255;
          }
        }
      }
      return pixels;
    }
  } // namespace hydra
} // namespace neam

#endif // __N_200324051639512063_182864571_HYDRA_LOGO_HPP__

