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
#include <map>

#include "../vulkan/shader_module.hpp"
#include "../vulkan/shader_loaders/spirv_loader.hpp"

#include <resources/context.hpp>
#include <ntools/async/chain.hpp>

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

      shader_manager(vk::device &_dev, resources::context& _res_context) : dev(_dev), res_context(_res_context), invalid_module(dev, nullptr, "") {}
      shader_manager(shader_manager &&o) : dev(o.dev), res_context(o.res_context), invalid_module(dev, nullptr, ""), module_map(std::move(o.module_map)) {}

      /// \brief asynchronously loads a shader module
      /// \note if multiple queries are done for the same module before it is loaded,
      ///       we currently waste a bit of time doing the same query over and over
      ///       but unless this becomes an issue and more complex system is needed, we will do
      ///       multiple queries. (also the signature and asynchronicity will not change anyway)
      async::chain<vk::shader_module&> load_shader(id_t rid, bool force_reload = false)
      {
        if (!force_reload)
        {
          // retrieve the module from memory:
          auto it = module_map.find(rid);
          if (it != module_map.end())
            return async::chain<vk::shader_module&>::create_and_complete(it->second);
        }

        // query the index for the module:
        return res_context.read_resource<assets::spirv_variation>(rid)
        .then([rid, this](assets::spirv_variation&& mod, resources::status st) -> vk::shader_module&
        {
          if (st == resources::status::failure)
          {
            cr::out().error("failed to load shader module {}", rid);
            return invalid_module;
          }
          auto ret = module_map.insert_or_assign(rid, vk::shader_module(dev, (const uint32_t*)mod.module.data.get(), mod.module.size, mod.entry_point));
          return ret.first->second;
        });
      }

      /// \brief Reload all shaders from the disk
      async::continuation_chain refresh()
      {
        std::vector<async::continuation_chain> chains;

        for (auto &it : module_map)
        {
          chains.push_back(load_shader(it.first, true).to_continuation());
        }

        return async::multi_chain(std::move(chains));
      }

      /// \brief Remove all modules
      void clear()
      {
        module_map.clear();
      }

      size_t get_shader_count() const
      {
        return module_map.size();
      }

    private:
      vk::device &dev;
      resources::context& res_context;
      vk::shader_module invalid_module;
      std::map<id_t, vk::shader_module> module_map;
  };
} // namespace neam::hydra

