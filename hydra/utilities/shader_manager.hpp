//
// file : shader_manager.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/shader_manager.hpp
//
// created by : Timothée Feuillet
// date: Sun Sep 04 2016 22:07:38 GMT+0200 (CEST)
//
//
// Copyright (c) 2016 Timothée Feuillet
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

#include <string>
#include <ntools/mt_check/map.hpp>

#include "../vulkan/shader_module.hpp"

#include <resources/context.hpp>
#include <ntools/async/chain.hpp>
#include <ntools/event.hpp>

#include <assets/spirv.hpp>

namespace neam::hydra
{
  /// \brief A lovely class that handle shaders to avoid the same source being loaded
  /// more than one time
  ///
  /// As shaders are only used at pipeline creation, having a slow [O(log(n))] way
  /// to retrieve shader modules does not really matter
  ///
  /// \note Only the spirv loaded is currently handled
  class shader_manager
  {
    public:

      shader_manager(vk::device &_dev, resources::context& _res_context) : dev(_dev), res_context(_res_context)
      {
        register_index_reload_event();
      }
      shader_manager(shader_manager &&o) : dev(o.dev), res_context(o.res_context), module_map(std::move(o.module_map))
      {
        register_index_reload_event();
        o.on_index_loaded.release();
      }

#if 0 // FIXME
      /// \brief asynchronously loads shader info
      /// \note if multiple queries are done for the same module before it is loaded,
      ///       we currently waste a bit of time doing the same query over and over
      ///       but unless this becomes an issue and more complex system is needed, we will do
      ///       multiple queries. (also the signature and asynchronicity will not change anyway)
      async::chain<assets::spirv_shader&> load_shader_info(id_t rid, bool force_reload = false)
      {
        if (!force_reload)
        {
          // retrieve the module from memory:
          auto it = shader_info_map.find(rid);
          if (it != shader_info_map.end())
            return async::chain<assets::spirv_shader&>::create_and_complete(it->second);
        }
        // FIXME?
      }
#endif

      /// \brief asynchronously loads a shader module
      /// \note if multiple queries are done for the same module before it is loaded,
      ///       we currently waste a bit of time doing the same query over and over
      ///       but unless this becomes an issue and more complex system is needed, we will do
      ///       multiple queries. (also the signature and asynchronicity will not change anyway)
      async::chain<vk::shader_module&> load_shader(id_t rid, bool force_reload = false)
      {
        if (!force_reload)
        {
          std::lock_guard _l { spinlock_shared_adapter::adapt(lock) };
          // retrieve the module from memory:
          auto it = module_map.find(rid);
          if (it != module_map.end())
            return async::chain<vk::shader_module&>::create_and_complete(it->second);
        }

        // query the index for the module:
        return res_context.read_resource<assets::spirv_variation>(rid)
        .then([rid, this, force_reload](assets::spirv_variation&& mod, resources::status st)
        {
          if (st == resources::status::failure)
          {
            cr::out().error("failed to load shader module {}", res_context.resource_name(rid));

            // we got an invalid module, but we should still create the data and insert it in our mod list, so we can reload it
            // NOTE: this is only valid behavior for cases where the assets can changes
            std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
            if (auto it = module_map.find(rid); it != module_map.end() && !force_reload)
              return async::chain<vk::shader_module&>::create_and_complete(it->second);

            vk::shader_module vkmod { dev, nullptr, 0, "" };
            auto ret = module_map.insert_or_assign(rid, std::move(vkmod));

            return async::chain<vk::shader_module&>::create_and_complete(ret.first->second);
          }

          vk::shader_module vkmod { dev, (const uint32_t*)mod.module.data.get(), mod.module.size, mod.stage, mod.entry_point };
          vkmod._set_debug_name(res_context.resource_name(rid));

          vkmod.get_push_constant_ranges() = std::move(mod.push_constant_ranges);
          vkmod.get_descriptor_sets() = std::move(mod.descriptor_set);

          return res_context.read_resource<assets::spirv_shader>(mod.root)
          .then([this, rid, root_id = mod.root, vkmod = std::move(vkmod), force_reload]
                       (assets::spirv_shader&& shader_info, resources::status st) mutable -> vk::shader_module &
          {
            if (st == resources::status::failure)
            {
              cr::out().warn("failed to load shader info {} (for {})", res_context.resource_name(root_id), res_context.resource_name(rid));
            }
            else // not a failure:
            {
              vkmod._get_constant_id_map() = std::move(shader_info.constant_id);
            }
            std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
            if (auto it = module_map.find(rid); it != module_map.end() && !force_reload)
              return it->second;
            auto ret = module_map.insert_or_assign(rid, std::move(vkmod));
            return ret.first->second;
          });
        });
      }

      /// \brief Reload all shaders from the disk
      async::continuation_chain refresh()
      {
        std::vector<async::continuation_chain> chains;

        cr::out().warn("shader manager: reloading all loaded shaders");
        std::vector<id_t> ids;
        {
          std::lock_guard _l { spinlock_shared_adapter::adapt(lock) };
          for (auto& it : module_map)
            ids.push_back(it.first);
        }

        for (id_t it : ids)
          chains.push_back(load_shader(it, true).to_continuation());

        return async::multi_chain(std::move(chains)).then([this]()
        {
          on_shaders_reloaded();
        });
      }

      /// \brief Remove all modules
      void clear()
      {
        std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
        module_map.clear();
      }

      size_t get_shader_count() const
      {
        std::lock_guard _l { spinlock_shared_adapter::adapt(lock) };
        return module_map.size();
      }

    public:
      cr::event<> on_shaders_reloaded;

    private:
      void register_index_reload_event()
      {
        on_index_loaded = res_context.on_index_loaded.add([this]() { refresh(); });
      }

    private:
      vk::device &dev;
      resources::context& res_context;
      mutable shared_spinlock lock;
      std::mtc_map<id_t, vk::shader_module> module_map;
      // std::map<id_t, assets::spirv_shader> shader_info_map;

      cr::event_token_t on_index_loaded;
  };
} // namespace neam::hydra

