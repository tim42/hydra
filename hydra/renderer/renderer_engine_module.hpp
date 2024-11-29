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

      void* ref = nullptr;
      bool need_setup = true;
      std::string debug_context;

      vk::semaphore last_transfer_operation { hctx.device, nullptr };

      // below this point: managed by the caller

      glm::uvec2 size {0, 0};
      VkImageLayout input_layout = VK_IMAGE_LAYOUT_UNDEFINED;
      VkImageLayout output_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      bool clear_framebuffer = false;

      virtual std::vector<VkFormat> get_framebuffer_format() const = 0;
      virtual void begin() {}
      virtual void pre_render(vk::submit_info& si) {}
      virtual void post_render(vk::submit_info& si) {}
      virtual void post_submit() {}
      virtual void end() {}

      virtual std::vector<vk::image*> get_images() = 0;
      virtual std::vector<vk::image_view*> get_images_views() = 0;
  };

  /// \brief Reference type, as contexts cannot be destructed like a C++ object (they must use a VRD)
  template<typename T>
  struct render_context_ref_t
  {
    T& ref;
    class renderer_module& mod;

    T* operator ->() { return &ref; }
    const T* operator ->() const { return &ref; }
    render_context_t& operator *() { return ref; }
    const T& operator *() const { return ref; }

    ~render_context_ref_t();
  };
  /// \brief Manages the render task group + VRD
  class renderer_module final : public engine_module<renderer_module>
  {
    public: // conf:
      float min_frame_time = 0.005f; // in s. Set to 0 to remove.

    public: // render contexts:
      template<typename T, typename... Args>
      std::unique_ptr<render_context_ref_t<T>> create_render_context(Args&&... args)
      {
        static_assert(std::is_base_of_v<render_context_t, T>, "render context created this way must inherit from renderer_module::context_t");

        std::unique_ptr<render_context_t> rct { new T(*hctx, std::forward<Args>(args)...) };
        std::unique_ptr<render_context_ref_t<T>> r {new render_context_ref_t<T>{static_cast<T&>(*rct), *this}};
        rct->ref = r.get();
        std::lock_guard _l(lock);
        contexts_to_add.push_back(std::move(rct));
        return r;
      }

      // automatically called on destruction of context_ref_t
      template<typename T>
      void _request_removal(render_context_ref_t<T>& ref)
      {
        std::lock_guard _l(lock);
        contexts_to_remove.push_back(&ref);
      }

      /// \brief Render a render context.
      template<typename T>
      void render_context(render_context_ref_t<T>& ref)
      {
        render_context(ref.ref);
      }
      void render_context(render_context_t& context);

    public: // render task group API:
      cr::event<> on_render_start;
      cr::event<> on_render_end;

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

      void init_vulkan_interface(gen_feature_requester& gfr, bootstrap& /*hydra_init*/) override;

      void add_named_threads(threading::threads_configuration& tc) override;
      void add_task_groups(threading::task_group_dependency_tree& tgd) override;
      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override;

      void on_context_initialized() override;

      void on_shutdown_post_idle_gpu() override;
      void on_start_shutdown() override;

    private: // internal api:
      void prepare_renderer();

    private:
      // render contexts:
      std::vector<std::unique_ptr<render_context_t>> contexts;

      spinlock lock;
      std::vector<std::unique_ptr<render_context_t>> contexts_to_add;
      std::vector<void*> contexts_to_remove;

      cr::chrono chrono;
      bool skip_frame = false;

      friend class engine_t;
      friend engine_module<renderer_module>;

      cr::event_token_t on_frame_start_tk;
  };

  template<typename T>
  render_context_ref_t<T>::~render_context_ref_t() { mod._request_removal(*this); }
}

