//
// created by : Timothée Feuillet
// date: 2021-11-21
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

#include "render_pass_context.hpp"

namespace neam::hydra
{
  struct hydra_context;

  /// \brief Provide the base process of how to render stuff using hydra
  /// Hydra uses a prepare/submit process:
  ///  - prepare: create all the temporary buffers and setup the cpu -> gpu transfers
  ///  - submit: fill the command buffers. It may run on multiple threads.
  ///
  /// Right after the prepare phase, cpu -> gpu transfers are started, all the gpu memory is unmapped and the memory allocator is frozen.
  /// It is not possible to allocate gpu-memory during the submit phase.
  ///
  /// Allocations marked as pass-local are expected to be heavilly re-used, so they must not leave the pass.
  /// Allocations marked as frame-local are not reused during the frame and so can leave the pass, but not the frame.
  class base_render_pass
  {
    public:
      base_render_pass(hydra_context& _context);
      virtual ~base_render_pass() = default;

      virtual bool enabled() const
      {
        return true;
      }


      // Called at init
      // Single threaded, in order of insertion in the pass manager
      virtual void setup(render_pass_context& /*rpctx*/)
      {
      }

      // Single threaded, in order of insertion in the pass manager
      virtual void setup_dependencies(/* FIXME !*/)
      {
      }

      // Single threaded*, in order of insertion in the pass manager
      virtual void prepare(render_pass_context& /*rpctx*/)
      {
        // create buffers, allocate memory, setup cpu -> gpu transfers, ...
      }

      // multi-threaded
      virtual render_pass_output submit(render_pass_context& /*rpctx*/)
      {
        // work with the command buffer
        return {};
      }

      virtual void cleanup(render_pass_context& /*rpctx*/)
      {
        // end !
      }

  protected:
      hydra_context& context;

      bool need_setup = true;
      friend class pass_manager;
  };

  template<typename Child>
  class render_pass : public base_render_pass
  {
    public:
      /// \brief Setup global \e static dependencies
      /// Those dependencies are gathered at the start of the engine and compiled in a set of instruction
      /// Adding a pass to a render-context will automatically add requisites before and "hook" passes after
      /// \note Circular dependencies are checked for and will generate an error (and disable all the passes in the cycle and those that depend on them)
      static void setup_static_dependencies() {}
  };
}
