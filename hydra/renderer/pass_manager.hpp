//
// created by : Timothée Feuillet
// date: 2022-5-10
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

#include <ntools/id/string_id.hpp>
#include "render_pass.hpp"

namespace neam::hydra
{
  struct hydra_context;
  namespace vk
  {
    class submit_info;
  }

  /// \brief Setup, filter, dispatch and submit render passes
  class pass_manager : public render_pass
  {
    public:
      pass_manager(hydra_context& _context);
      ~pass_manager();

      /// \brief Add a new pass (args are the constructor arguments)
      /// The order passes are executed is the order in which they are provided
      template<typename T, typename... Args>
      T& add_pass(Args&&... args)
      {
        passes.push_back(std::unique_ptr<render_pass>(new T(std::forward<Args>(args)...)));
        return *static_cast<T*>(passes.back().get());
      }

      void setup(render_pass_context& rpctx, bool force);
      void setup(render_pass_context& rpctx) override;
      void prepare(render_pass_context& rpctx) override;
      render_pass_output submit(render_pass_context& rpctx) override;
      void cleanup() override;


      /// \brief render the whole stack of render-passes
      /// \note some operations may be done on multiple threads
      /// It's a bit better to call render than to call prepare/submit/cleanup
      /// \note prepase() is to be called before this function with a proper thread sync
      /// \note cleanup is to be called after the submit info is transmit (just before the vrd perform its job)
      ///       this is done so that the render-pass-manager doesn't have to handle what kind of framebuffer it renders to
      ///       (is it a swapchain? a texture?)
      void render(neam::hydra::vk::submit_info& gsi, neam::hydra::vk::submit_info& csi, render_pass_context& rpctx, bool& has_any_compute);


    private:
      std::vector<std::unique_ptr<render_pass>> passes;
      neam::hydra::vk::semaphore gfx_transfer_finished;
      neam::hydra::vk::semaphore cmp_transfer_finished;
      size_t to_transfer = 0;
  };
}

