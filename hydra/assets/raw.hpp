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
  /// \brief Raw asset, .data contains directly what the resource contains, as-is.
  struct raw_asset : public resources::asset<"raw", raw_asset>
  {
    raw_data data;

    static raw_asset from_raw_data(const raw_data& _data, resources::status& st)
    {
      st = resources::status::success;
      return { .data = _data.duplicate() };
    }

    static raw_data to_raw_data(const raw_asset& _data, resources::status& st)
    {
      st = resources::status::success;
      return _data.data.duplicate();
    }
  };
}

