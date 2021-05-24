//
// file : lodePNG.hpp
// in : file:///home/tim/projects/hydra/hydra/ext/lodePNG/lodePNG.hpp
//
// created by : Timothée Feuillet
// date: Sun Sep 04 2016 17:38:35 GMT+0200 (CEST)
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

#ifndef __N_23694226432166622597_379525003_LODEPNG_HPP__
#define __N_23694226432166622597_379525003_LODEPNG_HPP__

#define LODEPNG_NO_COMPILE_CPP
#include <lodepng.h>
#undef LODEPNG_NO_COMPILE_CPP

#include "../../utilities/image_loader.hpp"
#include "../../hydra_exception.hpp"

namespace neam
{
  namespace hydra
  {
    class lodePNG_loader : public image_loader
    {
      private:
        // TODO: 16bit formats
        static constexpr size_t format_count = 4;
        static constexpr VkFormat handled_formats[] =      {VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};
        static constexpr LodePNGColorType lode_formats[] = {LCT_GREY,           LCT_GREY_ALPHA,       LCT_RGB,                LCT_RGBA};
        static constexpr int bpp[] =                       {8,                  8,                    8,                      8};
        static constexpr int comp[] =                      {1,                  2,                    3,                      4};

      public:
        uint8_t *load_image_from_file(const std::string &file, VkFormat format, glm::uvec2 &image_size) final
        {
          uint8_t *ret;
          size_t format_index;
          for (format_index = 0; format_index < format_count; ++format_index)
          {
            if (handled_formats[format_index] == format)
              break;
          }
          check::on_vulkan_error::n_assert(format_index < format_count, "image format not supported");

          uint8_t *temp_ret = nullptr;
          unsigned lode_ret_code = lodepng_decode_file(&temp_ret, &image_size.x, &image_size.y, file.c_str(), lode_formats[format_index], bpp[format_index]);
          if (lode_ret_code)
            neam::cr::out.error() << file << ": lodePNG error: " << lodepng_error_text(lode_ret_code) << std::endl;
          check::on_vulkan_error::n_assert(lode_ret_code == 0, file + ": lodePNG returned an error code (see the logs)");

          // copy, 'cause we need to use delete to free the memory (not 'free()')
          size_t ret_size = image_size.x * image_size.y * comp[format_index];
          ret = (uint8_t *)operator new(ret_size);
          memcpy(ret, temp_ret, ret_size);

          free(temp_ret);
          return ret;
        }
    };
  } // namespace hydra
} // namespace neam

#endif // __N_23694226432166622597_379525003_LODEPNG_HPP__

