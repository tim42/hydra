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
#include <ntools/chrono.hpp>

namespace neam::hydra
{
  /// \brief A render context. Works for both on/offscreen
  /// \note Will only perform rendering operations (and \e some initialization).
  ///       Framebuffer creation and framebuffer selection is left to the caller
  /// \note framebuffer format / resolution change is to be handled by the caller
  class render_context_t
  {
    public:
      render_context_t(hydra_context& _hctx) : hctx(_hctx) {}
      virtual ~render_context_t() {}

      hydra_context& hctx;
      pass_manager pm { hctx };

      vk::command_pool graphic_transient_cmd_pool {hctx.gqueue.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)};
      vk::command_pool compute_transient_cmd_pool {hctx.gqueue.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)};

      struct render_context_ref_t* ref;
      bool need_setup = true;

      // below this point: managed by the caller

      std::vector<vk::framebuffer> framebuffers {};

      glm::uvec2 size {0, 0};
      VkFormat framebuffer_format = VK_FORMAT_B8G8R8A8_UNORM;
      VkImageLayout output_layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;

      virtual void begin() {}
      virtual uint32_t get_framebuffer_index() { return 0; }
      virtual void pre_render(vk::submit_info& gsi) {}
      virtual void post_render(vk::submit_info& gsi) {}
      virtual void post_submit() {}
      virtual void end() {}
  };

  /// \brief Simple implementation of a offscreen render-context
  class offscreen_render_context_t : public render_context_t
  {
    public:
      void begin() override;
  };

  /// \brief Reference type, as contexts cannot be destructed like a C++ object (they must use a VRD)
  struct render_context_ref_t
  {
    render_context_t& ref;
    class renderer_module& mod;

    render_context_t* operator ->() { return &ref; }
    const render_context_t* operator ->() const { return &ref; }
    render_context_t& operator *() { return ref; }
    const render_context_t& operator *() const { return ref; }

    ~render_context_ref_t();
  };
  /// \brief Manages the render task group + VRD
  class renderer_module final : public engine_module<renderer_module>
  {
    public: // conf:
      float min_frame_time = 0.005f; // in s. Set to 0 to remove.

    public: // render contexts:
      template<typename T, typename... Args>
      std::unique_ptr<render_context_ref_t> create_render_context(Args&&... args)
      {
        static_assert(std::is_base_of_v<render_context_t, T>, "render context created this way must inherit from renderer_module::context_t");

        std::unique_ptr<render_context_t> rct { new T(*hctx, std::forward<Args>(args)...) };
        std::unique_ptr<render_context_ref_t> r {new render_context_ref_t{*rct, *this}};
        rct->ref = r.get();
        return r;
      }

      // automatically called on destruction of context_ref_t
      void _request_removal(render_context_ref_t& ref)
      {
        std::lock_guard _l(lock);
        contexts_to_remove.push_back(&ref);
      }

      /// \brief Render a render context.
      void render_context(render_context_ref_t& ref);

    public: // render task group API:
      void register_on_render_start(id_t fid, std::function<void()> func)
      {
        functions_start.emplace_back(fid, std::move(func));
      }
      void register_on_render_end(id_t fid, std::function<void()> func)
      {
        functions_end.emplace_back(fid, std::move(func));
      }
      void unregister_on_render_start(id_t fid)
      {
        functions_start.erase
        (
          std::remove_if(functions_start.begin(), functions_start.end(), [fid](auto & x) { return x.first == fid; }),
          functions_start.end()
        );
      }
      void unregister_on_render_end(id_t fid)
      {
        functions_end.erase
        (
          std::remove_if(functions_end.begin(), functions_end.end(), [fid](auto & x) { return x.first == fid; }),
          functions_end.end()
        );
      }

    private: // module interface:
      static constexpr const char* module_name = "renderer";

      static bool is_compatible_with(runtime_mode m)
      {
        // we only need a full hydra context:
        // (the renderer works for passive/offscreen, as long as there's a full hydra context)
        if ((m & runtime_mode::hydra_context) != runtime_mode::hydra_context)
          return false;
        return true;
      }

      void add_task_groups(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_task_group("render"_rid, "render");
      }
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override
      {
        tgd.add_dependency("render"_rid, "io"_rid);
        tgd.add_dependency("render"_rid, "init"_rid);
      }

      void on_context_initialized() override
      {
        hctx->tm.set_start_task_group_callback("render"_rid, [this]
        {
          skip_frame = true;
          if (chrono.get_accumulated_time() >= min_frame_time)
          {
            chrono.reset();
            skip_frame = false;
          }

          if (skip_frame)
            return;

          // start a task to avoid stalling the task manager:
          // (and so that tasks that are spawned by the functions are immediatly dispatched)
          cctx->tm.get_task([this]
          {
            // Housekeeping: cleanup resources that need cleanup
            hctx->vrd.update();

            // Call the different functions:
            for (auto& it : functions_start)
              it.second();

          });
        });
        hctx->tm.set_end_task_group_callback("render"_rid, [this]
        {
          if (skip_frame)
            return;

          // may stall the task manager :/
          for (auto& it : functions_end)
            it.second();
        });
      }

    private:
      struct render_pass_pair
      {
        vk::render_pass init_render_pass;
        vk::render_pass output_render_pass;
      };

    private: // utilities:
      render_pass_pair& get_pass_pair_for(VkFormat framebuffer_format, VkImageLayout output_layout);

    private:
      // render task-group:
      std::vector<std::pair<id_t, std::function<void()>>> functions_start;
      std::vector<std::pair<id_t, std::function<void()>>> functions_end;


      // render contexts:
      std::map<uint64_t, render_pass_pair> render_pass_cache;
      std::vector<std::unique_ptr<render_context_t>> contexts;

      spinlock lock;
      std::vector<std::unique_ptr<render_context_t>> contexts_to_add;
      std::vector<render_context_ref_t*> contexts_to_remove;

      cr::chrono chrono;
      bool skip_frame = false;

      friend class engine_t;
      friend engine_module<renderer_module>;
  };
}

