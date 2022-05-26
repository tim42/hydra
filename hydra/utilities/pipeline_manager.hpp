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
      private:
        struct pipeline_holder
        {
          vk::pipeline_creator pc;
          vk::pipeline p;
        };
      public:
        pipeline_manager(vk::device &_dev) : dev(_dev) {}
        pipeline_manager(pipeline_manager &&o) : dev(o.dev), pipelines_map(std::move(o.pipelines_map)) {}

        /// \brief Add a new pipeline
        void _add_pipeline(const id_t id, vk::pipeline_creator&& pc)
        {
          pipelines_map.emplace(id, std::move(pc));
        }

        vk::pipeline_creator& add_pipeline(const id_t id)
        {
          pipelines_map.emplace(id, dev);
          return pipelines_map.at(id);
        }

        /// \brief Return the pipeline creator of a given pipeline
        /// To then refresh the pipeline, call refresh(name);
        const vk::pipeline_creator& get_pipeline_creator(const id_t id) const { return find_pipeline(id)->second; }
        vk::pipeline_creator& get_pipeline_creator(const id_t id) { return find_pipeline(id)->second; }
        const vk::pipeline_creator& get_pipeline_creator(const string_id id) const { return find_pipeline(id)->second; }
        vk::pipeline_creator& get_pipeline_creator(const string_id id) { return find_pipeline(id)->second; }

        /// \brief Return the pipeline named name
        const vk::pipeline& get_pipeline(const id_t id) const { return find_pipeline(id)->second.get_pipeline(); }
        vk::pipeline& get_pipeline(const id_t id) { return find_pipeline(id)->second.get_pipeline(); }
        const vk::pipeline& get_pipeline(const string_id id) const { return find_pipeline(id)->second.get_pipeline(); }
        vk::pipeline& get_pipeline(const string_id id) { return find_pipeline(id)->second.get_pipeline(); }

        /// \brief Refresh a single pipeline
        void refresh(const string_id id)
        {
          auto it = find_pipeline(id);
          if (it->second.allow_derivate_pipelines())
          {
            it->second.set_base_pipeline(&it->second.get_pipeline());
            it->second.create_pipeline();
            it->second.set_base_pipeline(nullptr);
          }
          else
          {
            // slow path
            it->second.create_pipeline();
          }
        }

        void refresh(const id_t id)
        {
          auto it = find_pipeline(id);

          if (it->second.allow_derivate_pipelines())
          {
            it->second.set_base_pipeline(&it->second.get_pipeline());
            it->second.set_base_pipeline(nullptr);
          }
          else
          {
            // slow path
            it->second.create_pipeline();
          }
        }

        /// \brief Recreate all the pipelines
        void refresh(vk_resource_destructor& vrd, vk::queue& queue)
        {
          for (auto &it : pipelines_map)
          {
            if (it.second.allow_derivate_pipelines())
            {
              vrd.postpone_destruction_to_next_fence(queue, std::move(it.second.get_pipeline()));
//               it.second.set_base_pipeline(&it.second.get_pipeline());
              it.second.create_pipeline();
//               it.second.set_base_pipeline(nullptr);
            }
            else
            {
              // slow path
              vrd.postpone_destruction_to_next_fence(queue, std::move(it.second.get_pipeline()));
              it.second.create_pipeline();
            }
          }
        }

        size_t get_pipeline_count() const
        {
          return pipelines_map.size();
        }

      private:
        vk::device &dev;
        std::map<id_t, vk::pipeline_creator> pipelines_map;

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
          check::on_vulkan_error::n_check(it != pipelines_map.end(), "could not find pipeline: {}", id.get_string());
          return it;
        }

        auto find_pipeline(string_id id) -> decltype(pipelines_map.find(id))
        {
          auto it = pipelines_map.find(id);
          check::on_vulkan_error::n_check(it != pipelines_map.end(), "could not find pipeline: {}", id.get_string());
          return it;
        }
    };
  } // namespace hydra
} // namespace neam

#endif // __N_164028163982217544_8016717_PIPELINE_MANAGER_HPP__

