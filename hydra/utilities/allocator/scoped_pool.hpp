//
// created by : Timothée Feuillet
// date: 2022-8-26
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

#include <cstddef>
#include <cstdint>
#include <ntools/mt_check/vector.hpp>
#include "allocator.hpp"

namespace neam::hydra::allocator
{
  class block_allocator;

  /// \brief Handle scoped allocations (pass-local and stuff like that)
  /// Made to be super fast and aggressive memory re-use.
  /// \todo Thread safety ?
  class scoped_pool final : allocator_interface
  {
    public:
      class scope
      {
        public:
          scope(scope&& o);
          ~scope();

          memory_allocation allocate(uint32_t size, uint32_t alignment);

          /// \brief push a new scope
          /// \note It is incorrect to perform scoped allocations on the parent scope while the child scope is alive
          scope push_scope();

        private:
          scope(scoped_pool& _pool, scope* _parent = nullptr, uint32_t _offset = 0, uint32_t _alloc = 0);

        private:
          scoped_pool& pool;
          scope* parent = nullptr;
          uint32_t current_offset = 0;
          uint32_t current_alloc = 0;
          bool has_child_scope = false;
          bool had_child_scope = false;
          bool is_valid = true;

          friend scoped_pool;
      };

    public:
      scoped_pool(block_allocator& _alloc) : allocator(_alloc) {}

      /// \brief Push a new root scope for the allocations
      /// \warning All root scopes will overlap their allocations (the system is made so that independent render-contexts can run independently on multiple threads)
      ///          It is incorrect to use scoped allocations with anything that has its usage scope outside the scope it's been allocated on
      ///          (like transfer destinations/sources, ...)
      scope push_scope();

    public:
      memory_allocation allocate(uint32_t size, uint32_t alignment);

      void free_allocation(const memory_allocation& mem) override {} // do nothing

    private:
      void do_allocate_block(size_t size);

      static scope*& current_scope()
      {
        thread_local scope* current_scope = nullptr;
        return current_scope;
      }
    private:
      block_allocator& allocator;

      std::mtc_vector<memory_allocation> allocations;
  };
}
