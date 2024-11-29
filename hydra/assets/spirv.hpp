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

namespace neam::hydra::assets
{
  struct push_constant_entry
  {
    uint32_t offset;
  };
  struct push_constant_range
  {
    id_t id;
    uint16_t size;
  };
  struct descriptor_set_entry
  {
    id_t id;
    uint32_t set;
  };

  struct spirv_variation : public resources::rle_data_asset<"spirv-variation", spirv_variation>
  {
    // handle versioning:
    static constexpr uint32_t min_supported_version = 0;
    static constexpr uint32_t current_version = 0;
    using version_list = ct::type_list< spirv_variation >;

    // data:
    std::string entry_point = "main";
    raw_data module;
    id_t root;
    uint32_t stage;

    std::vector<push_constant_range> push_constant_ranges;
    std::vector<descriptor_set_entry> descriptor_set;
  };

  struct spirv_shader : public resources::rle_data_asset<"spirv", spirv_shader>
  {
    // handle versioning:
    static constexpr uint32_t min_supported_version = 0;
    static constexpr uint32_t current_version = 0;
    using version_list = ct::type_list< spirv_shader >;

    std::map<id_t, uint32_t> constant_id;
  };
}

N_METADATA_STRUCT(neam::hydra::assets::spirv_variation)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(entry_point),
    N_MEMBER_DEF(module),
    N_MEMBER_DEF(root),
    N_MEMBER_DEF(stage),

    N_MEMBER_DEF(push_constant_ranges),
    N_MEMBER_DEF(descriptor_set)
  >;
};

N_METADATA_STRUCT(neam::hydra::assets::push_constant_entry)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(offset)
  >;
};

N_METADATA_STRUCT(neam::hydra::assets::push_constant_range)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(id),
    N_MEMBER_DEF(size)
  >;
};
N_METADATA_STRUCT(neam::hydra::assets::descriptor_set_entry)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(id),
    N_MEMBER_DEF(set)
  >;
};

N_METADATA_STRUCT(neam::hydra::assets::spirv_shader)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(constant_id)
  >;
};

