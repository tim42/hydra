//
// created by : Timothée Feuillet
// date: 2021-12-11
//
//
// Copyright (c) 2021 Timothée Feuillet
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

#include <ntools/id/string_id.hpp>
#include <ntools/raw_data.hpp>
#include <resources/asset.hpp>
#include <resources/packer.hpp>

#include <glm/glm.hpp>

namespace neam::hydra::assets
{
  struct image : public resources::rle_data_asset<"image", image>
  {
    // handle versioning:
    static constexpr uint32_t min_supported_version = 0;
    static constexpr uint32_t current_version = 0;
    using version_list = ct::type_list< image >;

    glm::uvec3 size;
    VkFormat format;

    std::vector<id_t> mips; // FIXME: is that necessary ?
  };

  struct image_mip : public resources::rle_data_asset<"mipmap", image_mip>
  {
    // handle versioning:
    static constexpr uint32_t min_supported_version = 0;
    static constexpr uint32_t current_version = 0;
    using version_list = ct::type_list< image_mip >;

    // data:
    glm::uvec3 size;
    raw_data texels;
  };
}

N_METADATA_STRUCT(neam::hydra::assets::image)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(size),
    N_MEMBER_DEF(format),
    N_MEMBER_DEF(mips)
  >;
};
N_METADATA_STRUCT(neam::hydra::assets::image_mip)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(size),
    N_MEMBER_DEF(texels)
  >;
};


