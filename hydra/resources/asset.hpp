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

#include <ntools/id/id.hpp>
#include <ntools/id/string_id.hpp>
#include <ntools/rle/rle.hpp>
#include <ntools/raw_data.hpp>
#include "enums.hpp"
#include "packer.hpp"
#include "concepts.hpp"
#include "../hydra_debug.hpp"

namespace neam::resources
{
  /// \brief Used to indicate asset types and automatic decoding of stuff
  /// The resource spirv:/my/module.frag will have the spirv asset-type
  ///
  /// The system is hands-off on the data-side and as such allows for utilities and wrappers.
  /// Most semi-complex assets will use the rle_data_asset as it provides an easier way to deal with more complex data layouts
  template<ct::string_holder IDName, typename BaseType, typename Helper = BaseType>
  class asset
  {
    public:
      static constexpr id_t asset_type = string_id(IDName);
      static constexpr decltype(IDName) type_name = IDName;
      using helper_type = Helper;

      /// \brief Loads the asset from packed data
      static BaseType from_raw_data(const raw_data& data, status& st) { return Helper::from_raw_data(data, st); }

      /// \brief Saves the asset to packed data
      static raw_data to_raw_data(const BaseType& data, status& st) { return Helper::to_raw_data(data, st); }
  };

  /// \brief Simpler asset whose memory can be dumped as-is (or memcpy'd)
  /// \note If you need more complex, please use rle_data_asset
  template<ct::string_holder IDName, typename BaseType>
  class plain_data_asset : public asset<IDName, BaseType, plain_data_asset<IDName, BaseType>>
  {
    public:
      static BaseType from_raw_data(const raw_data& data, status& st)
      {
        if (check::debug::n_check(data.size == sizeof(BaseType), "Failed to decode plain data asset: type does not match"))
        {
          st = status::failure;
          return {};
        }
        st = status::success;
        return *reinterpret_cast<const BaseType*>(data.data.get());
      }

      static raw_data to_raw_data(const BaseType& data, status& st)
      {
        st = status::success;
        raw_data rd = raw_data::allocate(sizeof(BaseType));
        memcpy(data.data.get(), &data, sizeof(BaseType));
        return rd;
      }
  };

  template<ct::string_holder IDName, typename BaseType>
  class rle_data_asset : public asset<IDName, BaseType, rle_data_asset<IDName, BaseType>>
  {
    public:
      static BaseType from_raw_data(const raw_data& data, status& st)
      {
        st = status::success;
        rle::status rle_st = rle::status::success;
        rle::decoder dc = data;
        BaseType ret = rle::coder<BaseType>::decode(dc, rle_st);
        if (rle_st == rle::status::failure)
          st = status::failure;
        return ret;
      }

      static raw_data to_raw_data(const BaseType& data, status& st)
      {
        st = status::success;
        cr::memory_allocator ma;
        rle::encoder ec(ma);
        rle::status rle_st = rle::status::success;
        rle::coder<BaseType>::encode(ec, data, rle_st);
        if (rle_st == rle::status::failure)
          st = status::failure;
        return ec.to_raw_data();
      }
  };
}
