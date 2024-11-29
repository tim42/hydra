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


#include <hydra/hydra_debug.hpp>

#include "scoped_pool.hpp"
#include "block_allocator.hpp"

namespace neam::hydra::allocator
{
  static constexpr uint32_t align(uint32_t value, uint32_t alignment)
  {
    return (value - 1u + alignment) & -alignment;
  }

  memory_allocation scoped_pool::scope::allocate(uint32_t size, uint32_t alignment)
  {
    check::debug::n_assert(size > 0, "allocate: cannot perform an allocation of size 0");
    check::debug::n_assert(has_child_scope == false, "allocate: cannot perform scoped allocation when a child scope is still alive");
    check::debug::n_assert(had_child_scope == false, "allocate: cannot perform scoped allocation when a child scope has already been pushed");
    check::debug::n_assert(parent != nullptr, "allocate: cannot perform scoped allocation in the root scope");

    const uint32_t aligned_offset = align(current_offset, alignment);
    if (current_alloc < pool.allocations.size() && (aligned_offset + size) <= pool.allocations[current_alloc].size())
    {
      current_offset = aligned_offset + size;
      auto& allocation = pool.allocations[current_alloc];
      return {pool.allocator._get_memory_type_index(), allocation_type::scoped, allocation.offset() + aligned_offset, size, allocation.mem(), &pool, nullptr};
    }
    else if (current_alloc < pool.allocations.size())
    {
      // first, try to grow the current allocation
      const uint32_t block_count = (size + block_allocator::k_block_size - 1u) / block_allocator::k_block_size;
      if (pool.allocator.try_grow_allocation(pool.allocations[current_alloc], block_count))
      {
        current_offset = aligned_offset + size;
        auto& allocation = pool.allocations[current_alloc];
        return {pool.allocator._get_memory_type_index(), allocation_type::scoped, allocation.offset() + aligned_offset, size, allocation.mem(), &pool, nullptr};
      }

      // if that fails, recurse to the next allocation (or allocate a new block)
      ++current_alloc;
      current_offset = 0;
      // not recursion (tail recursion + no non-trivially destructible object on stack)
      return allocate(size, alignment);
    }
    else
    {
      pool.do_allocate_block(size);
      current_offset = size;
      auto& allocation = pool.allocations.back();
      return {pool.allocator._get_memory_type_index(), allocation_type::scoped, allocation.offset(), size, allocation.mem(), &pool, nullptr};
    }

    return {}; // failed
  }

  scoped_pool::scope scoped_pool::scope::push_scope()
  {
    check::debug::n_assert(has_child_scope == false, "push_scope: cannot push a new scope when a child scope is still alive");
    has_child_scope = true;
    had_child_scope = true;
    return { pool, this, current_offset, current_alloc };
  }

  scoped_pool::scope::scope(scoped_pool& _pool, scope* _parent, uint32_t _offset, uint32_t _alloc)
    : pool(_pool)
    , parent(_parent)
    , current_offset(_offset)
    , current_alloc(_alloc)
  {
    pool.current_scope() = this;
  }

  scoped_pool::scope::scope(scope&& o)
   : pool(o.pool)
   , parent(o.parent)
   , current_offset(o.current_offset)
   , current_alloc(o.current_alloc)
   , has_child_scope(o.has_child_scope)
   , had_child_scope(o.had_child_scope)

  {
    check::debug::n_assert(is_valid, "scope(scope&&): trying to create an invalid allocation scope (from an already moved object), aborting");
    check::debug::n_assert(has_child_scope == false, "scope(scope&&): scopes cannot be moved when they have a child scope");

    pool.current_scope() = this;
    o.is_valid = false;
  }

  scoped_pool::scope::~scope()
  {
    // nothing to do if invalid
    if (!is_valid)
      return;

    check::debug::n_assert(has_child_scope == false, "~scope: scopes should not be destroyed when child scopes are still alive");

    if (parent != nullptr)
    {
      check::debug::n_assert(parent->has_child_scope == true, "~scope: parent scope does not have childs, but the current instance has it as a parent");
      parent->has_child_scope = false;
    }

    pool.current_scope() = parent;
  }

  scoped_pool::scope scoped_pool::push_scope()
  {
    return {*this};
  }

  memory_allocation scoped_pool::allocate(uint32_t size, uint32_t alignment)
  {
    check::debug::n_assert(current_scope() != nullptr, "scoped_pool::allocate: trying to perform scoped allocation when no scope are present");
    return current_scope()->allocate(size, alignment);
  }

  void scoped_pool::do_allocate_block(size_t size)
  {
    const uint32_t block_count = std::max<uint32_t>(2u, (size + block_allocator::k_block_size - 1u) / block_allocator::k_block_size);

    allocations.push_back(allocator.block_level_allocation(block_count));
  }
}

