//
// created by : Timothée Feuillet
// date: 2021-11-30
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

#define N_ENUM_UNARY_OPERATOR(name, op) constexpr name operator op (name a) { return (name)(op std::to_underlying(a)); }
#define N_ENUM_BINARY_OPERATOR(name, op) constexpr name operator op (name a, name b) { return (name)(std::to_underlying(a) op std::to_underlying(b)); }

namespace neam::resources
{
  enum class status
  {
    // Complete failure:
    failure,

    // Not a complete failure.
    // Data has been produced and can be used, but there were some issues
    // (for an index, partial_success could be "we received the file, but some entries were rejected as they were invalid")
    partial_success,

    // Complete success
    success,
  };

  /// \brief returns the worst of the two status
  [[nodiscard]] static constexpr status worst(status a, status b)
  {
    return std::to_underlying(a) < std::to_underlying(b) ? a : b;
  }

  /// \brief returns the best of the two status
  [[nodiscard]] static constexpr status best(status a, status b)
  {
    return std::to_underlying(a) > std::to_underlying(b) ? a : b;
  }
}
