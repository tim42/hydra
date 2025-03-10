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

#include "engine_module.hpp"

#include "core_context.hpp"
#include "vk_context.hpp"
#include "hydra_context.hpp"

#include <ntools/mt_check/map.hpp>
#include <hydra/hydra_debug.hpp>

namespace neam::hydra
{
  /// \brief Constant, boot settings for the engine:
  /// \note Here is the stuff that should not go in the runtime_mode
  struct engine_settings_t
  {
    hydra_device_creator::filter_device_preferences vulkan_device_preferences = hydra_device_creator::prefer_discrete_gpu;

    uint32_t thread_count = std::thread::hardware_concurrency() - 2;
  };

  /// \brief Root class of hydra. Can startup all the different runtime modes.
  /// \note Multiple engines might exist at the same time.
  class engine_t
  {
    public:
      engine_t() = default;
      ~engine_t() { sync_teardown(); cleanup(); }

    public: // lifecycle

      /// \brief Set the engine settings. Must be done pre-boot.
      void set_engine_settings(const engine_settings_t& settings)
      {
        check::debug::n_assert(mode == runtime_mode::none, "Trying to set the engine settings post-boot");
        engine_settings = settings;
      }

      /// \brief Init the engine. Fully synchronous.
      /// \note will initialize the vulkan instance/device if requested in the mode
      resources::status init(runtime_mode mode);

      /// \brief Boot the engine
      /// In case of failure, the engine is reverted to its initial state (so boot() can be called again)
      /// The chain is resolved after either the engine is reverted to its initial state or the process is completed
      ///
      /// It is invalid to call boot if a boot process is still in progress or a previous call has succeded
      resources::context::status_chain boot(index_boot_parameters_t&& ibp = { .index_file = "root.index" });

      /// \brief init() + boot() combined
      resources::context::status_chain boot(runtime_mode _mode, index_boot_parameters_t&& ibp = { .index_file = "root.index" })
      {
        if (init(_mode) != resources::status::failure)
          return boot(std::move(ibp));
        return resources::context::status_chain::create_and_complete(resources::status::failure);
      }

      /// \brief Return a sucessfuly initialized engine to its pre-boot state
      /// Sync version, only return when everything is uninit
      void sync_teardown();

      /// \brief perform the destruction of the engine modules. Should be called outside the task manager.
      void uninit();

      /// \brief Fully cleanup after a teardown.
      /// Must be called outside the task manager / outside of the context
      void cleanup();

      /// \brief Called when recurring tasks should not be pushed.
      /// This indicates that the engine is stopping
      bool should_stop_pushing_tasks() const { return shutdown_stop_task_manager; }

    public:
      /// \brief Return the mode with which the engine has been setup
      runtime_mode get_runtime_mode() const { return mode; }

    public: // modules
      bool has_module(id_t name) const { return modules.contains(name); }
      template<typename Type>
      bool has_module() const { return has_module(string_id{Type::module_name}); }

      template<typename FinalType = engine_module_base>
      FinalType* get_module(id_t name)
      {
        if (auto it = modules.find(name); it != modules.end())
          return static_cast<FinalType*>(it->second.get());
        return nullptr;
      }

      template<typename FinalType = engine_module_base>
      const FinalType* get_module(id_t name) const
      {
        if (auto it = modules.find(name); it != modules.end())
          return static_cast<const FinalType*>(it->second.get());
        return nullptr;
      }

      template<typename FinalType>
      FinalType* get_module()
      {
        return get_module<FinalType>(string_id{FinalType::module_name});
      }

      template<typename FinalType>
      const FinalType* get_module() const
      {
        return get_module<FinalType>(string_id{FinalType::module_name});
      }

    public: // contextes
      /// \brief Return the hydra context. Kill the app if the context does not exist.
      hydra_context& get_hydra_context()
      {
        check::debug::n_assert((mode & runtime_mode::hydra_context) == runtime_mode::hydra_context, "Trying to get the hydra-context in a mode that doesn't provide it");
        return get_context<hydra_context>("hydra-context");
      }

      /// \brief Return the vulkan context. Kill the app if the context does not exist.
      vk_context& get_vulkan_context()
      {
        check::debug::n_assert((mode & runtime_mode::vulkan_context) == runtime_mode::vulkan_context, "Trying to get the vulkan-context in a mode that doesn't provide it");
        if ((mode & runtime_mode::hydra_context) == runtime_mode::hydra_context)
          return get_context<hydra_context>("hydra-context");
        return get_context<internal_vk_context>("vk-context");
      }

      /// \brief Return the core context. Kill the app if the context does not exist.
      core_context& get_core_context()
      {
        check::debug::n_assert((mode & runtime_mode::core) == runtime_mode::core, "Trying to get the core-context in a mode that doesn't provide it");
        if ((mode & runtime_mode::hydra_context) == runtime_mode::hydra_context)
          return get_context<hydra_context>("hydra-context");
        if ((mode & runtime_mode::vulkan_context) == runtime_mode::vulkan_context)
          return get_context<internal_vk_context>("vk-context");
        return get_context<core_context>("core-context");
      }

    public:
      const engine_settings_t& get_engine_settings() const { return engine_settings; }

    private:
      template<typename Context>
      Context& get_context(const char* name)
      {
        Context* ctx = std::get_if<Context>(&context);
        check::debug::n_assert(ctx != nullptr, "Trying to get the {} before its creation", name);
        return *ctx;
      }

    private:
      struct internal_vk_context : public core_context, public vk_context
      {
        using vk_context::vk_context;
      };

      std::mtc_map<id_t, std::unique_ptr<engine_module_base>> modules;
      std::variant<std::monostate, core_context, internal_vk_context, hydra_context> context;

      engine_settings_t engine_settings;

      runtime_mode mode = runtime_mode::none;
      bool shutdown_stop_task_manager = false;
      bool shutdown_idle_io = false;
      bool shutdown_no_more_vulkan = false;

      // prevent teardown running before we init the modules
      spinlock init_lock;
  };
}

