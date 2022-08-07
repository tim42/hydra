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

#ifndef __N_164028163982217544_8016717_PIPELINE_MANAGER_HPP__
#define __N_164028163982217544_8016717_PIPELINE_MANAGER_HPP__

#include <map>
#include <string>

#include <ntools/id/string_id.hpp>
#include <ntools/id/id.hpp>
#include "../hydra_debug.hpp"

#include "../vulkan/device.hpp"
#include "../vulkan/pipeline.hpp"

#include "pipeline_render_state.hpp"

namespace neam
{
  namespace hydra
  {
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
        pipeline_manager(vk::device &_dev) : dev(_dev) {}
        pipeline_manager(pipeline_manager &&o) : dev(o.dev), pipelines_map(std::move(o.pipelines_map)) {}

        template<typename Fnc>
        void add_pipeline(const id_t id, Fnc&& fnc)
        {
          if (auto sit = pipelines_map.find(id); sit == pipelines_map.end())
          {
            auto [it, inserted] = pipelines_map.emplace(id, dev);
            fnc(it->second);
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

        bool has_pipeline(id_t id) const
        {
          return pipelines_map.contains(id);
        }

        /// \brief Return the pipeline named name
        template<typename... Args>
        const vk::pipeline& get_pipeline(const id_t id, Args&&... args)
        {
          return find_pipeline(id)->second.get_pipeline(std::forward<Args>(args)...);
        }
        template<typename... Args>
        const vk::pipeline& get_pipeline(const string_id id, Args&&... args)
        {
          return find_pipeline(id)->second.get_pipeline(std::forward<Args>(args)...);
        }
        template<typename Type, typename... Args>
        const vk::pipeline& get_pipeline(Args&&... args)
        {
          return get_pipeline(Type::pipeline_id, std::forward<Args>(args)...);
        }

        const vk::pipeline_layout& get_pipeline_layout(const id_t id)
        {
          return find_pipeline(id)->second.get_pipeline_layout();
        }
        const vk::pipeline_layout& get_pipeline_layout(const string_id id)
        {
          return find_pipeline(id)->second.get_pipeline_layout();
        }
        template<typename Type>
        const vk::pipeline_layout& get_pipeline_layout()
        {
          return get_pipeline_layout(Type::pipeline_id);
        }

        template<typename... Args>
        vk::descriptor_set allocate_descriptor_set(const id_t id, Args&& ... args)
        {
          return find_pipeline(id)->second.allocate_descriptor_set(std::forward<Args>(args)...);
        }
        template<typename... Args>
        vk::descriptor_set allocate_descriptor_set(const string_id id, Args&& ... args)
        {
          return find_pipeline(id)->second.allocate_descriptor_set(std::forward<Args>(args)...);
        }
        template<typename Type, typename... Args>
        vk::descriptor_set allocate_descriptor_set(Args&& ... args)
        {
          return allocate_descriptor_set(Type::pipeline_id, std::forward<Args>(args)...);
        }

        /// \brief Refresh a single pipeline
        void refresh(const string_id id, vk_resource_destructor& vrd, vk::queue& queue) { find_pipeline(id)->second.invalidate_pipelines(vrd, queue); }
        /// \brief Refresh a single pipeline
        void refresh(const id_t id, vk_resource_destructor& vrd, vk::queue& queue) { find_pipeline(id)->second.invalidate_pipelines(vrd, queue); }

        void immediate_refresh(const string_id id) { find_pipeline(id)->second.invalidate_pipelines(); }
        void immediate_refresh(const id_t id) { find_pipeline(id)->second.invalidate_pipelines(); }

        /// \brief Recreate all the pipelines
        void refresh(vk_resource_destructor& vrd, vk::queue& queue)
        {
          for (auto &it : pipelines_map)
          {
            it.second.invalidate_pipelines(vrd, queue);
          }
        }

        size_t get_pipeline_count() const
        {
          return pipelines_map.size();
        }

      private:
        vk::device &dev;
        std::map<id_t, pipeline_render_state> pipelines_map;

      private:
        auto find_pipeline(id_t id) const -> decltype(pipelines_map.find(id))
        {
          auto it = pipelines_map.find(id);
          check::on_vulkan_error::n_check(it != pipelines_map.end(), "could not find pipeline: {}", id);
          return it;
        }
        auto find_pipeline(id_t id) -> decltype(pipelines_map.find(id))
        {
          auto it = pipelines_map.find(id);
          check::on_vulkan_error::n_check(it != pipelines_map.end(), "could not find pipeline: {}", id);
          return it;
        }

        auto find_pipeline(string_id id) const -> decltype(pipelines_map.find(id))
        {
          auto it = pipelines_map.find(id);
          check::on_vulkan_error::n_check(it != pipelines_map.end(), "could not find pipeline: {}", id);
          return it;
        }

        auto find_pipeline(string_id id) -> decltype(pipelines_map.find(id))
        {
          auto it = pipelines_map.find(id);
          check::on_vulkan_error::n_check(it != pipelines_map.end(), "could not find pipeline: {}", id);
          return it;
        }
    };
  } // namespace hydra
} // namespace neam

#endif // __N_164028163982217544_8016717_PIPELINE_MANAGER_HPP__

