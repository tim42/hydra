//
// created by : Timothée Feuillet
// date: 2022-5-22
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

#include <functional>

#include <ntools/enum.hpp>
#include <ntools/type_id.hpp>

namespace neam::threading
{
  class task_group_dependency_tree;
  class task_manager;
}

namespace neam::hydra
{
  class bootstrap;
  class gen_feature_requester;

  class core_context;
  class vk_context;
  struct hydra_context;
  class engine_t;

  namespace vk
  {
    class physical_device;
    class instance;
  }

  enum class runtime_mode
  {
    none = 0,

    // context modes:
    core = 1 << 0,
    vulkan_context = (1 << 1) | core,
    hydra_context = (1 << 2) | vulkan_context,

    context_flags = core | vulkan_context | hydra_context,

    // engine mode: (each flag represent the absence of a feature)

    // (only valid for vk/hydra contextes)
    // There won't be windows/swapchain
    // Events/inputs can still be present
    // If this flag is absent, the engine will render to the screen
    offscreen = 1 << 3,

    // (only valid for vk/hydra contextes)
    // The engine is a replicata from another one. No inputs.
    // This means that no data/state changes are possible
    // If this flag is absent, the engine is active and can change data
    passive = 1 << 4,

    // No network connection
    // If this flag is absent, networking is possible
    offline = 1 << 5,

    // No debug stuff (outside compiled-in debug stuff)
    // If this flag is absent, debug stuff can be present
    // This flag prevent automatic index reload/index watch
    release = 1 << 6,

    // There won't be any resource packing
    // If this flag is absent, resource packing might take place
    packer_less = 1 << 7,
  };
  N_ENUM_FLAG(runtime_mode)

  class engine_module_base;

  /// \brief Free function module manager
  namespace module_manager
  {
    using filter_func_t = bool (*)(runtime_mode);
    using create_func_t = std::unique_ptr<engine_module_base> (*)();

    struct filtered_module_t
    {
      create_func_t create;
      std::string name;
    };

    /// \brief Register a module
    /// Might have adverse consequences if done during/after the initialization
    void register_module(create_func_t create, filter_func_t filter, const std::string& name);

    /// \brief Unregister a module
    /// Might have adverse consequences if done during/after the initialization
    void unregister_module(const std::string& name);

    /// \brief Filter modules for a given mode.
    std::vector<filtered_module_t> filter_modules(runtime_mode mode);
  }

  /// \brief An engine module is an object that affects core components of the engine
  ///        and provide functionnalities (like imgui, glfw, ...)
  /// \note The constructor might be called before main is. Do not put anything specific in there.
  class engine_module_base
  {
    public:
      virtual ~engine_module_base() {}

    public: // module
      /// \brief Used to filter-out modules
      /// \note Can be overriden in the child class
      /// \warning The sole purpose of this function is to filter the module.
      ///          It is an error to have any kind of code unrelated to this task in the function.
      static bool is_compatible_with(runtime_mode /*m*/) { return true; }

      /// \brief Set the engine
      void set_engine(engine_t* _engine) { engine = _engine; }

      /// \brief Set the core context
      void set_core_context(core_context* _cctx) { cctx = _cctx; }


    public: // init (core)
      /// \brief Called right before the boot step of the engine.
      /// \note Use on_context_initialized for heavier tasks.
      ///       This call-back should only be there to setup configuration that \e must be done before the call to boot() on the context
      /// \note this means that in the case of a vk_context the vulkan instance and device do exist
      virtual void on_pre_boot_step() {}

      /// \brief If there's specific task groups to create
      virtual void add_task_groups(threading::task_group_dependency_tree& /*tgd*/) {}
      /// \brief Add dependencies between the task-groups:
      virtual void add_task_groups_dependencies(threading::task_group_dependency_tree& /*tgd*/) {}


      /// \brief Called after the core context has been set (and is fully initialized).
      /// \note This function can do any specific initialization as the module has been selected
      ///       and will be used.
      /// \warning resource access is not possible at this time and the task manager might be locked
      /// \note this means that in the case of a vk_context the vulkan instance and device do exist
      virtual void on_context_initialized() {}

      /// \brief Called when the final index (resources) is loaded, but right before the task manager is unblocked
      /// \note the context might not be fully initialized (some modules might be pending init)
      virtual void on_resource_index_loaded() {}

      /// \brief Called right after the task manager is unlocked, the index is fully loaded and the context fully initialized
      virtual void on_engine_boot_complete() {}

    public: // init (vk_context)
      /// \brief Request specific features / stuff to do for the vulkan interface creation
      /// \note It will not be called if no vulkan interface is created
      virtual void init_vulkan_interface(gen_feature_requester& /*gfr*/, bootstrap& /*hydra_init*/) {}

      void set_vk_context(vk_context* _vctx) { vctx = _vctx; }

      virtual bool filter_queue(vk::instance& /*instance*/, int/*VkQueueFlagBits*/ /*queue_type*/, size_t /*qindex*/, const neam::hydra::vk::physical_device &/*gpu*/)
      {
        return true;
      }

    public: // init (hydra_context)
      void set_hydra_context(hydra_context* _hctx) { hctx = _hctx; }

    public: // shutdown:
      /// \brief Called before the task manager is stopping
      virtual void on_start_shutdown() {}
      /// \brief Called after the task manager has stopped (no task allowed), and after the vulkan device is idle.
      /// The place to destroy any vulkan resources
      virtual void on_shutdown() {}

    protected:
      core_context* cctx = nullptr; // guaranteed to always exist
      vk_context* vctx = nullptr;
      hydra_context* hctx = nullptr;
      engine_t* engine = nullptr;
  };


  template<typename Mod>
  class engine_module : public engine_module_base
  {
    public:
      /// \brief Module name (can be changed (type and constness, but must remain static and be any type of string) in the child class
      ///        It defaults to the type name
      static constexpr const char* module_name = ct::type_name<Mod>.str;

    private:
      static void register_module()
      {
        module_manager::register_module([]{ return std::unique_ptr<engine_module_base>(new Mod); }, &Mod::is_compatible_with, Mod::module_name);
      }

      static inline int _reg_placeholder_ = []
      {
        register_module();
        return 0; // so it's placed in .bss
      }();
      // force _reg_placeholder_ to be kept and be initialized
      static_assert(&_reg_placeholder_ == &_reg_placeholder_);
  };
}

