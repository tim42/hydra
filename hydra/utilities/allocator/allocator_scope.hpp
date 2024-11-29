//
// created by : Timothée Feuillet
// date: 2022-8-27
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

#include <ntools/mt_check/vector.hpp>

#include "scoped_pool.hpp"

namespace neam::hydra
{
  class memory_allocator;
}

namespace neam::hydra::allocator
{
  /// \brief like scoped_pool::scope but handle all the different sets at oce
  class scope
  {
    public:
      scope(scope&& o) = default;
      ~scope() = default;

      scope push_scope()
      {
        return {allocator, this};
      }

    private:
      scope(memory_allocator& _allocator, scope* _parent = nullptr);

    private:
      memory_allocator& allocator;
      scope* parent = nullptr;

      std::mtc_vector<scoped_pool::scope> scopes;

      friend memory_allocator;
  };
}

