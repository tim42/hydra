//
// file : pipeline_manager.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/pipeline_manager.hpp
//
// created by : Timothée Feuillet
// date: Sun Sep 04 2016 22:29:54 GMT+0200 (CEST)
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


#include <ntools/mt_check/map.hpp>
#include <string>

#include <ntools/id/string_id.hpp>
#include <ntools/id/id.hpp>
#include <ntools/event.hpp>
#include "../hydra_debug.hpp"

#include "../vulkan/device.hpp"
#include "../vulkan/pipeline.hpp"

#include "pipeline_render_state.hpp"

namespace neam
{
  namespace hydra
  {
    struct hydra_context;

    /// \brief Almost like shader_manager, it handles pipelines
    /// It is particularly used in the gui system where texts mostly share one
    /// (or two) different pipeline
    ///
    /// \note Unlike shader_manager, pipelines need to be created before being used
    ///       but one created they can be quickly modified / refreshed
    ///
    /// \todo handle transparently pipeline caches
    class pipeline_manager
    {
      public:
        pipeline_manager(hydra_context& _hctx, vk::device &_dev) : hctx(_hctx), dev(_dev) {}
        pipeline_manager(pipeline_manager &&o) : hctx(o.hctx), dev(o.dev), pipelines_map(std::move(o.pipelines_map)) {}

        template<typename Fnc>
        void add_pipeline(const string_id id, Fnc&& fnc)
        {
          // WARNING: no lock guard here, we unlock before the call to fnc
          lock.lock_exclusive();
          if (auto sit = pipelines_map.find(id); sit == pipelines_map.end())
          {
            auto [it, inserted] = pipelines_map.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(dev, hctx));
            it->second.pipeline_id = id;
            fnc(it->second);
            lock.unlock_exclusive();
          }
          else
          {
            lock.unlock_exclusive();
          }
        }
        template<typename Type, typename... Args>
        void add_pipeline(Args&&... args)
        {
          add_pipeline(Type::pipeline_id, [&](pipeline_render_state &prs)
          {
            Type::make_pipeline(prs, std::forward<Args>(args)...);
          });
        }

        bool has_pipeline(string_id id) const
        {
          std::lock_guard _l { spinlock_shared_adapter::adapt(lock) };
          return pipelines_map.contains(id);
        }

        bool is_pipeline_ready(string_id id) const
        {
          auto& prs = find_pipeline(id)->second;
          if (!prs.can_make_valid_pipelines())
            return false;
          return true;
        }

        /// \brief Return the pipeline named name
        template<typename... Args>
        const vk::pipeline& get_pipeline(const string_id id, Args&&... args)
        {
          auto& prs = find_pipeline(id)->second;
          if (!prs.can_make_valid_pipelines())
            return invalid_pipeline;
          return prs.get_pipeline(std::forward<Args>(args)...);
        }
        template<typename Type, typename... Args>
        const vk::pipeline& get_pipeline(Args&&... args)
        {
          return get_pipeline(Type::pipeline_id, std::forward<Args>(args)...);
        }

        VkPipelineBindPoint get_pipeline_bind_point(const string_id id) const
        {
          auto& prs = find_pipeline(id)->second;
          if (!prs.can_make_valid_pipelines())
            return VK_PIPELINE_BIND_POINT_MAX_ENUM;
          return prs.get_pipeline_bind_point();
        }
        template<typename Type>
        VkPipelineBindPoint get_pipeline_bind_point() const
        {
          return get_pipeline_bind_point(Type::pipeline_id);
        }

        const vk::pipeline_layout& get_pipeline_layout(const string_id id) const
        {
          auto& prs = find_pipeline(id)->second;
          if (!prs.can_make_valid_pipelines())
            return invalid_pipeline_layout;
          return prs.get_pipeline_layout();
        }
        template<typename Type>
        const vk::pipeline_layout& get_pipeline_layout() const
        {
          return get_pipeline_layout(Type::pipeline_id);
        }

        /// \brief Refresh a single pipeline
        void refresh(const string_id id) { find_pipeline(id)->second.invalidate_pipelines(); }

        /// \brief Recreate all the pipelines
        void refresh()
        {
          std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
          need_refresh = false;
          cr::out().debug("pipeline manager: invalidating all pipelines");
          for (auto &it : pipelines_map)
          {
            it.second.invalidate_pipelines();
          }
        }

        size_t get_pipeline_count() const
        {
          return pipelines_map.size();
        }

      public:
        void register_shader_reload_event(hydra_context& hctx, bool use_graphic_queue = true);

        bool should_refresh() const { return need_refresh; }

      private:
        hydra_context& hctx;
        vk::device &dev;
        mutable shared_spinlock lock;
        vk::pipeline invalid_pipeline { dev, nullptr, VK_PIPELINE_BIND_POINT_GRAPHICS };
        vk::pipeline_layout invalid_pipeline_layout { dev, nullptr };
        std::mtc_map<string_id, pipeline_render_state> pipelines_map;

        cr::event_token_t on_shaders_reloaded;
        bool need_refresh = false;

      private:
        auto find_pipeline(string_id id) const -> decltype(pipelines_map.find(id))
        {
          std::lock_guard _l { spinlock_shared_adapter::adapt(lock) };
          auto it = pipelines_map.find(id);
          check::on_vulkan_error::n_check(it != pipelines_map.end(), "could not find pipeline: {}", id);
          return it;
        }

        auto find_pipeline(string_id id) -> decltype(pipelines_map.find(id))
        {
          std::lock_guard _l { spinlock_shared_adapter::adapt(lock) };
          auto it = pipelines_map.find(id);
          check::on_vulkan_error::n_check(it != pipelines_map.end(), "could not find pipeline: {}", id);
          return it;
        }
    };
  } // namespace hydra
} // namespace neam



