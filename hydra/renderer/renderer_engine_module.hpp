//
// created by : Timothée Feuillet
// date: 2022-5-23
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

#include <hydra/engine/engine_module.hpp>
#include <hydra/engine/hydra_context.hpp>
#include <ntools/threading/threading.hpp>
#include <ntools/id/string_id.hpp>

namespace neam::hydra
{
//   class renderer_module final : private engine_module<renderer_module>
//   {
//     public: // public interface:
//       render_pass_context rpctx;
//       std::function<void()> do_submit;
// 
//     private: // module interface:
//       static constexpr const char* module_name = "renderer";
// 
//       static bool is_compatible_with(runtime_mode m)
//       {
//         // we need vulkan for imgui to be active
//         if ((m & runtime_mode::hydra_context) == runtime_mode::none)
//           return false;
//         return true;
//       }
// 
//       void add_task_groups(threading::task_group_dependency_tree& tgd) override
//       {
//         tgd.add_task_group("render"_rid, "render");
//       }
//       void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override
//       {
//         tgd.add_dependency("render"_rid, "io"_rid);
//       }
// 
//       void on_context_initialized() override
//       {
//         hctx->tm.set_start_task_group_callback("render"_rid, [this]
//         {
//           hctx->tm.get_task([this]
//           {
//             if (need_setup)
//             {
//               hctx->pm.setup(rpctx);
//               need_setup = false;
//             }
//             
//             hctx->pm.render();
// 
//             do_submit();
//             hctx->pm.cleanup();
//           });
//         });
//       }
// 
//     private:
//       bool need_setup = true;
//       friend class engine_t;
//       friend engine_module<renderer_module>;
// 
//   };
}

