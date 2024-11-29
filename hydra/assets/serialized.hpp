//
// created by : Timothée Feuillet
// date: 2023-1-20
//
//
// Copyright (c) 2023 Timothée Feuillet
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

#include "raw.hpp"

namespace neam::hydra::assets
{
  /// \brief Handle any kind of serialized C++ struct/class.
  /// \note The default packer for raw resources should create a :serialized simlink to the raw resource
  /// \note SubType must be de/serializable
  template<typename SubType>
  struct serialized_asset : public resources::asset<"serialized", serialized_asset<SubType>>
  {
    SubType data;

    static serialized_asset from_raw_data(const raw_data& data, resources::status& st)
    {
      st = resources::status::success;
      rle::status rle_st = rle::status::success;
      rle::decoder dc = data;
      serialized_asset ret = { rle::coder<SubType>::decode(dc, rle_st) };
      if (rle_st == rle::status::failure)
        st = resources::status::failure;
      return ret;
    }

    static raw_data to_raw_data(const serialized_asset& data, resources::status& st)
    {
      st = resources::status::success;
      cr::memory_allocator ma;
      rle::encoder ec(ma);
      rle::status rle_st = rle::status::success;
      rle::coder<SubType>::encode(ec, data.data, rle_st);
      if (rle_st == rle::status::failure)
        st = resources::status::failure;
      return ec.to_raw_data();
    }
  };
}

