//
// created by : Timothée Feuillet
// date: 2021-12-1
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

#include <string>
#include <ntools/mt_check/set.hpp>

#include "asset.hpp"
#include <ntools/id/string_id.hpp>

namespace neam::resources
{
  struct file_map_v0
  {
    std::mtc_set<std::string> files;
  };

  /// \brief :file-map resource struct
  struct file_map : public rle_data_asset<"file-map", file_map>
  {
    // handle versioning:
    static constexpr uint32_t min_supported_version = 0;
    static constexpr uint32_t current_version = 1;
    using version_list = ct::type_list< file_map_v0, file_map >;

    // data:
    std::string prefix_path;
    std::mtc_set<std::string> files = {};

    static file_map migrate_from(file_map_v0&& v0)
    {
      return { .prefix_path = {}, .files = std::move(v0.files) };
    }
  };
}

// meta-data:
N_METADATA_STRUCT(neam::resources::file_map_v0)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(files)
  >;
};
N_METADATA_STRUCT(neam::resources::file_map)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(prefix_path),
    N_MEMBER_DEF(files)
  >;
};
