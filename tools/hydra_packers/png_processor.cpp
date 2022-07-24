//
// created by : Timothée Feuillet
// date: 2022-5-3
//
//
// Copyright (c) 2022 Timothée Feuillet
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

#include "image_packer.hpp"

#include <hydra/resources/processor.hpp>

#include <lodePNG/lodepng.h>

namespace neam::hydra::processor
{
  struct png_processor : resources::processor::processor<png_processor, "image/png">
  {
    static constexpr id_t processor_hash = "neam/png-processor:0.1.0"_rid;

    static resources::processor::chain process_resource(hydra::core_context& /*ctx*/, resources::processor::input_data&& input)
    {
      glm::uvec2 size;
      uint8_t* temp_ret = nullptr;
      unsigned lode_ret_code = lodepng_decode32(&temp_ret, &size.x, &size.y, (const uint8_t*)input.file_data.data.get(), input.file_data.size);
      if (lode_ret_code)
      {
        neam::cr::out().error("process_resource(image/png): file: {}, lodePNG error: {}", input.file.c_str(), lodepng_error_text(lode_ret_code));
        if (temp_ret)
          free(temp_ret);
        return resources::processor::chain::create_and_complete({}, resources::status::failure);
      }

      raw_data rd = raw_data::allocate(size.x * size.y * 4 /*RGBA*/);
      memcpy(rd.data.get(), temp_ret, rd.size);
      free(temp_ret); // erk

      std::vector<resources::processor::data> ret;
      ret.emplace_back(resources::processor::data
        {
          .resource_id = get_resource_id(input.file),
          .resource_type = assets::image::type_name,
          .data = rle::serialize(packer::image_packer_input
          {
            .size = size,
            .texels = std::move(rd),
          }),
          .metadata = std::move(input.metadata),
          .db = input.db,
        });
      return resources::processor::chain::create_and_complete({.to_pack = std::move(ret)}, resources::status::success);
    }
  };
}
