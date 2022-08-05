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

#pragma once

#include <ntools/raw_data.hpp>
#include <ntools/struct_metadata/struct_metadata.hpp>
#include <hydra/assets/image.hpp>

#include <glm/glm.hpp>

namespace neam::hydra::packer
{
  struct image_packer_input
  {
    glm::uvec2 size;
    // TODO: add format, currently expecting rgba 8
    raw_data texels;
  };

  struct image_metadata
  {
    VkFormat target_format;
    uint32_t mip_count;
  };
}

N_METADATA_STRUCT(neam::hydra::packer::image_packer_input)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(size),
    N_MEMBER_DEF(texels)
  >;
};

N_METADATA_STRUCT(neam::hydra::packer::image_metadata)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(target_format),
    N_MEMBER_DEF(mip_count)
  >;
};
