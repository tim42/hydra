//
// created by : Timothée Feuillet
// date: 2021-12-17
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

#include "enums.hpp"

namespace neam::resources
{
  template<ct::string_holder IDName, typename BaseType, typename Helper> class asset;

  namespace concepts
  {
    template<typename T>
    concept Asset = requires(T v)
    {
      requires std::is_base_of_v<asset<T::type_name, T, typename T::helper_type>, T>;
    };

    template<typename T>
    concept AssetSimplifiedPacking = requires(T v)
    {
        { v.from_resource_data(raw_data{}, std::declval<status&>()) } -> std::same_as<T>;
    };

  }
}
