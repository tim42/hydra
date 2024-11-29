//
// created by : Timothée Feuillet
// date: 2023-10-7
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

#include <ntools/ct_list.hpp>
#include <ntools/io/context.hpp>
#include "../context.hpp"

namespace neam::resources::io_proxy
{
  enum class outcome
  {
    handled, // the proxy has handled the request. Forward the answer.

    // the operation will either be handled by the local context of by the next proxy (in the case of a proxy stack)
    // the answer is completly ignored
    unhandled,

    // unhandled + force local execution
    // in the case of a proxy stack, immediatly defer to the local context, skipping the rest of the stack. Otherwise, behaves like unhandled.
    force_local,

    // unhandled + prevent local execution.
    // in the case of a proxy stack, try other proxies, but, unless a proxy returns force_local,
    // will yield not_supported to the local context, erroring out.
    not_supported,
  };

  /// \brief Proxy for most resources::context operations
  /// It can bypass the index and (local) io but add a layer of indirection
  /// Handle most operations that resources::context can do
  class base_proxy
  {
    public: // types
      using read_chain = ct::list::append<io::context::read_chain, outcome>;
      using reldb_chain = async::chain<rel_db&&, outcome>;
      using has_res_chain = async::chain<bool, outcome>;

    public:
      // resource context internal operations:
      virtual const reldb_chain get_rel_db() const { return reldb_chain::create_and_complete({}, outcome::unhandled); }

      // resources operations:
      virtual read_chain read_raw_resource(id_t ) const { return read_chain::create_and_complete({}, false, 0, outcome::unhandled); }
      virtual has_res_chain has_resource(id_t ) const { return has_res_chain::create_and_complete(false, outcome::unhandled); }


  };
}

