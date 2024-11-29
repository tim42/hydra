//
// created by : Timothée Feuillet
// date: 2024-8-25
//
//
// Copyright (c) 2024 Timothée Feuillet
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

#include <ntools/refcount_ptr.hpp>
#include "deferred_fence_execution.hpp"

namespace neam::hydra
{
  namespace internal
  {
    template<typename PoolType>
    struct dfe_object_pool_deleter
    {
      void operator()(typename PoolType::object_ptr ptr) const
      {
        if (!ptr) return;

        check::debug::n_assert(pool != nullptr, "pool-deleter: pool pointer of type <...> is nullptr"/*, ct::type_name<PoolType>.view()*/);

        // NOTE: ANY DFE-autodeleted entry MUST have this function, and should use it to immediately release memory / resources that should be released immediately
        ptr->immediate_resource_release();

        if (dfe == nullptr)
        {
          pool->destruct(ptr);
          pool->deallocate(ptr);
        }
        else
        {
          dfe->defer([pool = this->pool, ptr]()
          {
            pool->destruct(ptr);
            pool->deallocate(ptr);
          });
        }
      }

      deferred_fence_execution* dfe = nullptr;
      PoolType* pool = nullptr;
    };
  }

  // Name is ugly, as the concept itself is ugly. Please don't use this unless there's no alternative.
  template<typename T>
  using dfe_refcount_pooled_ptr = cr::refcount_ptr<T, internal::dfe_object_pool_deleter<cr::memory_pool<T>>>;

  template<typename Pool, typename... Args>
  static dfe_refcount_pooled_ptr<typename Pool::object_type> make_dfe_refcount_pooled_ptr(deferred_fence_execution& dfe, Pool& pool, Args&& ... args)
  {
    using T = typename Pool::object_type;
    T* const ptr = new (pool.allocate()) T(std::forward<Args>(args)...);
    return dfe_refcount_pooled_ptr<T> { ptr, internal::dfe_object_pool_deleter<cr::memory_pool<T>>{&dfe, &pool} };
  }
}
