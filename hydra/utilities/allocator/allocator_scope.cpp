//
// created by : Timothée Feuillet
// date: 2023-1-14
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

#include "allocator_scope.hpp"

#include "../memory_allocator.hpp"

neam::hydra::allocator::scope::scope(neam::hydra::memory_allocator& _allocator, neam::hydra::allocator::scope* _parent)
  : allocator(_allocator)
  , parent(_parent)
{
  const uint32_t scope_count = allocator.heaps.size();
  scopes.reserve(scope_count);

  if (parent == nullptr)
  {
    // allocate scopes from the allocator (those will be root scopes)
    for (auto& it : allocator.heaps)
    {
      scopes.emplace_back(it.second.push_scope());
    }
  }
  else
  {
    // allocate scopes from the parent
    for (uint32_t i = 0; i < scope_count; ++i)
    {
      scopes.emplace_back(parent->scopes[i].push_scope());
    }
  }
}

