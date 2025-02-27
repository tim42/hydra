//
// created by : Timothée Feuillet
// date: 2/9/25
//
//
// Copyright (c) 2025 Timothée Feuillet
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

#include "ecs.hpp"
#include "../../ecs/hierarchy.hpp"
#include "../../engine/hydra_context.hpp"

#include <ntools/ref.hpp>
#include <ntools/function.hpp>

namespace neam::hydra::renderer
{
  struct gpu_task_context
  {
    transfer_context transfers;
  };

  enum class order_mode
  {
    standard,
    forced_prologue,
    forced_epilogue,
  };

  struct viewport_context
  {
    glm::uvec2 size;
    glm::uvec2 offset { 0, 0 };

    vk::rect2D viewport_rect { offset, size };
    vk::viewport viewport { viewport_rect };
  };

  enum class export_mode
  {
    // exporting over an already exported (constant) resource will assert
    // exporting with constant over a non-constant resource will assert
    constant,
    // the resource can be overridden by a later pass but previous versions are still accessible
    // NOTE: this internally uses the specialize("0"), specialize("1"), ... to indicate versions
    versioned,
  };

  struct exported_image
  {
    cr::ref<vk::image> image;
    cr::ref<vk::image_view> view; // main view

    VkImageLayout layout;
    VkAccessFlags access;
    VkPipelineStageFlags stage;
  };

  struct exported_buffer
  {
    vk::buffer& buffer;
  };

  static constexpr string_id k_output_id = "hydra::renderer::default_names::main_output"_rid;
  static constexpr string_id k_context_final_output = "hydra::renderer::default_names::context_final_output"_rid;

  struct exported_resource_data
  {
    export_mode mode;
    std::variant<std::monostate, exported_image, exported_buffer> resource; // erk
    uint32_t version = 0;
  };
}

namespace neam::hydra::renderer::concepts
{
  /// \brief Concept implemented by anything that can produce (directly or indirectly) command buffers (like render-passes)
  /// \note The render operations are done when the universe hierarchical_update* function is called
  class gpu_task_producer : public ecs::ecs_concept<gpu_task_producer>
                                , public ecs::concepts::hierarchical::concept_provider<gpu_task_producer>
  {
    private:
      using ecs_concept = ecs::ecs_concept<gpu_task_producer>;
      using hierarchical = ecs::concepts::hierarchical::concept_provider<gpu_task_producer>;

      /// \brief The base logic class
      class concept_logic : public ecs_concept::base_concept_logic
      {
        protected:
          concept_logic(typename ecs_concept::base_t& _base, hydra_context& _hctx)
            : ecs_concept::base_concept_logic(_base, _hctx)
            , hctx(_hctx)
          {}

        protected: // here be all the export/import API (NOTE: those functions can only be accessed during `prepare`)
          /// \brief exports a texture
          void export_resource(id_t id, exported_image image, export_mode mode = export_mode::versioned);
          /// \brief exports a buffer
          void export_resource(id_t id, vk::buffer& buffer, export_mode mode = export_mode::versioned);

          // TODO: other resources?
          // TODO: maybe a more generic way to export/import resources?

          /// \brief returns whether the resource exists and is importable
          [[nodiscard]] bool can_import(id_t id) const;
          /// \brief returns the highest importable version of the resource.
          // \note returns ~0u if the resource does not exist
          [[nodiscard]] uint32_t get_importable_version(id_t id) const;

          /// \brief returns the image at specified id and version (optional)
          /// \note final_layout indicate the layout the image will be left at the end of the pass
          [[nodiscard]] exported_image import_image(id_t id, VkImageLayout final_layout, VkAccessFlags final_access, VkPipelineStageFlags final_stage) const { return import_image(id, ~0u, final_layout, final_access, final_stage); }
          [[nodiscard]] exported_image import_image(id_t id, uint32_t version, VkImageLayout final_layout, VkAccessFlags final_access, VkPipelineStageFlags final_stage) const;

          /// \brief returns the buffer at specified id and version (optional)
          [[nodiscard]] vk::buffer& import_buffer(id_t id, uint32_t version = ~0u) const;

        protected: // here be the viewport-related API
          /// \brief Return whether a viewport context has been set
          [[nodiscard]] bool has_viewport_context() const;

          /// \brief returns the viewport context
          /// \note asserts if non where set
          [[nodiscard]] viewport_context& get_viewport_context() const;

          /// \brief set the viewport context for the stack
          /// \note can only be called once in the stack
          void set_viewport_context(const viewport_context& vpc);

        protected: // vulkan helpers
          /// \brief Helper for a generic begin rendering
          void begin_rendering(vk::command_buffer_recorder& cbr, const exported_image& img, VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op);

          /// \brief Helper for a generic begin rendering
          void begin_rendering(vk::command_buffer_recorder& cbr, const std::vector<exported_image>& imgs, VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op);

          void pipeline_barrier(vk::command_buffer_recorder& cbr, exported_image& img,
                          VkImageLayout new_layout,
                          VkAccessFlags dst_access,
                          VkPipelineStageFlags dst_stage);

          void pipeline_barrier(vk::command_buffer_recorder& cbr, exported_image& img,
                          VkAccessFlags dst_access,
                          VkPipelineStageFlags dst_stage);

          // void pipeline_barrier(vk::command_buffer_recorder& cbr, const std::vector<exported_image>& imgs,
          //                 VkAccessFlags dst_access,
          //                 VkPipelineStageFlags dst_stage);

        private: // here be all the API to be (optionally) implemented in children
          /// \brief Called once, used to set up resources that are created once are reused without change (PSO, DS, ...)
          virtual void do_setup(gpu_task_context& gtctx) = 0;

          virtual void update_enabled_flags() = 0;
          virtual bool concept_provider_requires_skip() const = 0;
          virtual bool is_concept_provider_enabled() const = 0;

          /// \brief Called every frame, before submit. Where memory allocations, transfer setups and transient object creation are performed
          virtual void do_prepare(gpu_task_context& gtctx) = 0;
          /// \brief Fill the submit info
          virtual void do_submit(gpu_task_context& gtctx, vk::submit_info& si) = 0;
          /// \brief Optional step, called once the submission is done
          virtual void do_cleanup() = 0;

        protected: // here be all the API accessible to children
          /// \brief Return the first gpu-task-producer of that type in the hierarchy
          [[nodiscard]] concept_logic* get_gpu_task_producer(enfield::type_t type) const { return get_concept().get_gpu_task_producer(type); }

          /// \brief Return the first gpu-task-producer of that type in the hierarchy
          template<typename T>
          [[nodiscard]] T* get_gpu_task_producer() const { return static_cast<T*>(get_gpu_task_producer(T::object_type_id)); }

        protected: // a more internal part of the API:
          virtual order_mode get_order_mode() const = 0;

        protected:
          hydra_context& hctx;

        private:
          bool need_setup = true;

        private:
          friend class gpu_task_producer;
      };

    public:
      template<typename ConceptProvider>
      class concept_provider : public concept_logic
      {
        protected:
          using gpu_task_producer_provider_t = concept_provider<ConceptProvider>;

        protected:
          concept_provider(ConceptProvider& _p, hydra_context& _hctx) : concept_logic(static_cast<typename ecs_concept::base_t&>(_p), _hctx) {}

        public:
          // to be overridden if necessary in children
          static constexpr order_mode order = order_mode::standard;

          ~concept_provider()
          {
            using setup_state_t = typename ct::function_traits<decltype(&ConceptProvider::setup)>::return_type;
            using prepare_state_t = typename ct::function_traits<decltype(&ConceptProvider::prepare)>::return_type;

            static constexpr bool has_setup_state = !std::is_same_v<setup_state_t, void>;
            static constexpr bool has_prepare_state = !std::is_same_v<prepare_state_t, void>;

            check::debug::n_assert(!prepare_state_constructed, "destructor for concept_provider<{}>: prepare state is still constructed at deletion", ct::type_name<ConceptProvider>.str);

            if constexpr (has_setup_state)
            {
              hctx.dfe.defer_destruction(std::move((setup_state_t*)setup_state_ptr.get()));
              std::destroy_at((setup_state_t*)setup_state_ptr.get());
              operator delete(setup_state_ptr.release(), sizeof(setup_state_t), std::align_val_t{alignof(setup_state_t)});
            }
            if constexpr (has_prepare_state)
            {
              operator delete(prepare_state_ptr.release(), sizeof(prepare_state_t), std::align_val_t{alignof(prepare_state_t)});
            }
          }

          bool has_setup_state()
          {
            using setup_state_t = typename ct::function_traits<decltype(&ConceptProvider::setup)>::return_type;
            static constexpr bool has_setup_state = !std::is_same_v<setup_state_t, void>;
            if constexpr (has_setup_state)
            {
              return setup_state_ptr;
            }
            return false;
          }
          auto get_setup_state()
          {
            using setup_state_t = typename ct::function_traits<decltype(&ConceptProvider::setup)>::return_type;
            static constexpr bool has_setup_state = !std::is_same_v<setup_state_t, void>;

            if constexpr (has_setup_state)
            {
              check::debug::n_assert(setup_state_ptr, "{}::get_setup_state: setup has not yet been called", ct::type_name<ConceptProvider>.str);
              return (setup_state_t*)(setup_state_ptr.get());
            }
          }

          bool has_prepare_state()
          {
            using prepare_state_t = typename ct::function_traits<decltype(&ConceptProvider::prepare)>::return_type;
            static constexpr bool has_prepare_state = !std::is_same_v<prepare_state_t, void>;
            if constexpr (has_prepare_state)
            {
              return prepare_state_constructed;
            }
            return false;
          }
          auto get_prepare_state()
          {
            using prepare_state_t = typename ct::function_traits<decltype(&ConceptProvider::prepare)>::return_type;
            static constexpr bool has_prepare_state = !std::is_same_v<prepare_state_t, void>;

            if constexpr (has_prepare_state)
            {
              check::debug::n_assert(prepare_state_ptr, "{}::get_prepare_state: setup has not yet been called", ct::type_name<ConceptProvider>.str);
              check::debug::n_assert(prepare_state_constructed, "{}::get_prepare_state: prepare has not yet been called this frame", ct::type_name<ConceptProvider>.str);
              return (prepare_state_t*)(prepare_state_ptr.get());
            }
          }

        protected: // override in children as needed
          void setup(gpu_task_context& gtctx) {}

          template<typename... T>
          void prepare(gpu_task_context& gtctx, T&&...) {}

          template<typename... T>
          void submit(gpu_task_context& gtctx, vk::submit_info& si, T&&...) {}

          template<typename... T>
          void cleanup(T&&... p)
          {
            hctx.dfe.defer_destruction<T...>(std::move(p)...);
          }

          template<typename... T>
          bool is_enabled(T&&...) const { return true; }
          template<typename... T>
          bool should_skip(T&&...) const { return false; }

        private:
          void update_enabled_flags() final
          {
            enabled_flag = static_cast<const ConceptProvider*>(this)->is_enabled(/*FIXME!*/);
            should_skip_flag = static_cast<const ConceptProvider*>(this)->should_skip(/*FIXME!*/);
          }
          bool concept_provider_requires_skip() const final
          {
            return should_skip_flag;
          }
          bool is_concept_provider_enabled() const final
          {
            return enabled_flag;
          }

          void do_setup(gpu_task_context& gtctx) final
          {
            TRACY_SCOPED_ZONE;
            using setup_state_t = typename ct::function_traits<decltype(&ConceptProvider::setup)>::return_type;
            using prepare_state_t = typename ct::function_traits<decltype(&ConceptProvider::prepare)>::return_type;

            static constexpr bool has_setup_state = !std::is_same_v<setup_state_t, void>;
            static constexpr bool has_prepare_state = !std::is_same_v<prepare_state_t, void>;

            if constexpr (has_prepare_state)
            {
              if (!prepare_state_ptr)
                prepare_state_ptr = operator new (sizeof(prepare_state_t), std::align_val_t{alignof(prepare_state_t)});
            }

            if constexpr (has_setup_state)
            {
              if (!setup_state_ptr)
              {
                setup_state_ptr = operator new (sizeof(setup_state_t), std::align_val_t{alignof(setup_state_t)});

                // placement new when calling setup
                new(setup_state_ptr.get()) setup_state_t(static_cast<ConceptProvider*>(this)->setup(gtctx));
              }
              else
              {
                hctx.dfe.defer_destruction(std::move((setup_state_t*)setup_state_ptr.get()));
                std::destroy_at((setup_state_t*)setup_state_ptr.get());
                new(setup_state_ptr.get()) setup_state_t(static_cast<ConceptProvider*>(this)->setup(gtctx));
              }
            }
            else
            {
              // simple call to setup
              static_cast<ConceptProvider*>(this)->setup(gtctx);
            }
          }

          void do_prepare(gpu_task_context& gtctx) final
          {
            TRACY_SCOPED_ZONE;
            using setup_state_t = typename ct::function_traits<decltype(&ConceptProvider::setup)>::return_type;
            using prepare_state_t = typename ct::function_traits<decltype(&ConceptProvider::prepare)>::return_type;

            static constexpr bool has_setup_state = !std::is_same_v<setup_state_t, void>;
            static constexpr bool has_prepare_state = !std::is_same_v<prepare_state_t, void>;

            if constexpr (has_prepare_state)
            {
              check::debug::n_assert(!prepare_state_constructed, "{}::prepare: prepare state will be overwritten but has not been destructed", ct::type_name<ConceptProvider>.str);
              if constexpr (has_setup_state)
              {
                new (prepare_state_ptr.get()) prepare_state_t(static_cast<ConceptProvider*>(this)->prepare(gtctx, *(setup_state_t*)setup_state_ptr.get()));
              }
              else
              {
                new (prepare_state_ptr.get()) prepare_state_t(static_cast<ConceptProvider*>(this)->prepare(gtctx));
              }
              prepare_state_constructed = true;
            }
            else
            {
              if constexpr (has_setup_state)
              {
                static_cast<ConceptProvider*>(this)->prepare(gtctx, *(setup_state_t*)setup_state_ptr.get());
              }
              else
              {
                static_cast<ConceptProvider*>(this)->prepare(gtctx);
              }
            }
          }

          void do_submit(gpu_task_context& gtctx, vk::submit_info& si) final
          {
            TRACY_SCOPED_ZONE;
            using setup_state_t = typename ct::function_traits<decltype(&ConceptProvider::setup)>::return_type;
            using prepare_state_t = typename ct::function_traits<decltype(&ConceptProvider::prepare)>::return_type;

            static constexpr bool has_setup_state = !std::is_same_v<setup_state_t, void>;
            static constexpr bool has_prepare_state = !std::is_same_v<prepare_state_t, void>;

            if constexpr (has_setup_state && has_prepare_state)
            {
              static_cast<ConceptProvider*>(this)->submit(gtctx, si, *(setup_state_t*)setup_state_ptr.get(), *(prepare_state_t*)prepare_state_ptr.get());
            }
            else if constexpr (has_setup_state && !has_prepare_state)
            {
              static_cast<ConceptProvider*>(this)->submit(gtctx, si, *(setup_state_t*)setup_state_ptr.get());
            }
            else if constexpr (!has_setup_state && has_prepare_state)
            {
              static_cast<ConceptProvider*>(this)->submit(gtctx, si, *(prepare_state_t*)prepare_state_ptr.get());
            }
            else
            {
              static_cast<ConceptProvider*>(this)->submit(gtctx, si);
            }
          }

          void do_cleanup() final
          {
            TRACY_SCOPED_ZONE;
            using prepare_state_t = typename ct::function_traits<decltype(&ConceptProvider::prepare)>::return_type;

            static constexpr bool has_prepare_state = !std::is_same_v<prepare_state_t, void>;

            if constexpr (has_prepare_state)
            {
              check::debug::n_assert(prepare_state_constructed, "{}::cleanup: prepare state is expected to be constructed, but isn't", ct::type_name<ConceptProvider>.str);

              static_cast<ConceptProvider*>(this)->cleanup(std::move(*(prepare_state_t*)prepare_state_ptr.get()));
              // destroy the object after the call to cleanup
              std::destroy_at((prepare_state_t*)prepare_state_ptr.get());

              prepare_state_constructed = false;
            }
            else
            {
              static_cast<ConceptProvider*>(this)->cleanup();
            }
          }

        protected: // here be boilerplate part of the API
          order_mode get_order_mode() const final
          {
            return ConceptProvider::order;
          }

        private:
          cr::raw_ptr<void*> setup_state_ptr;
          cr::raw_ptr<void*> prepare_state_ptr;
          bool prepare_state_constructed = false;
          bool should_skip_flag = false;
          bool enabled_flag = true;
      };

    public:
      gpu_task_producer(typename ecs_concept::param_t p, hydra_context& _hctx)
        : ecs_concept(p)
        , hierarchical(*this)
        , hctx(_hctx)
      {}

    public: // public API
      // nothing!

    private: // 1-to-n dispatch:
      void setup(gpu_task_context& gtctx);
      void prepare(gpu_task_context& gtctx);
      void cleanup();

    private: // hierarchical API
      void update_from_hierarchy();

    private: // internal API
      concept_logic* get_gpu_task_producer(enfield::type_t type) const;

      template<typename T>
      void export_resource_tpl(id_t id, T&& res, export_mode mode = export_mode::versioned);

      void export_resource(id_t id, exported_image res, export_mode mode = export_mode::versioned);
      void export_resource(id_t id, vk::buffer& res, export_mode mode = export_mode::versioned);

      bool can_import(id_t id) const;
      uint32_t get_importable_version(id_t id) const;

      const exported_resource_data& import_resource(id_t id, uint32_t version) const;
      exported_image import_image(id_t id, uint32_t version, VkImageLayout final_layout, VkAccessFlags final_access, VkPipelineStageFlags final_stage) const;
      vk::buffer& import_buffer(id_t id, uint32_t version) const;

    private:
      bool has_viewport_context() const;
      viewport_context& get_viewport_context() const;
      void set_viewport_context(const viewport_context& vpc);

    private:
      template<typename UnaryFunction>
      void sorted_for_each_entries(UnaryFunction&& func);

    private:
      hydra_context& hctx;
      gpu_task_context gtc { transfer_context{hctx}};

      std::mtc_unordered_map<id_t, exported_resource_data> exported_resources;

      std::optional<viewport_context> vpc;

      friend ecs_concept;
      friend hierarchical;
  };
}
