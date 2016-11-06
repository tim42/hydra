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

#include "../hydra_exception.hpp"

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
        void add_pipeline(const std::string &name, vk::pipeline_creator pc)
        {
          pipelines_map.emplace(name, pipeline_holder { pc, pc.create_pipeline(dev) });
        }

        /// \brief Return the pipeline creator of a given pipeline
        /// To then refresh the pipeline, call refresh(name);
        vk::pipeline_creator &get_pipeline_creator(const std::string &name)
        {
          auto it = pipelines_map.find(name);
          check::on_vulkan_error::n_assert(it != pipelines_map.end(), "could not find pipeline: " + name);
          return it->second.pc;
        }

        /// \brief Return the pipeline creator of a given pipeline
        /// To then refresh the pipeline, call refresh(name);
        const vk::pipeline_creator &get_pipeline_creator(const std::string &name) const
        {
          auto it = pipelines_map.find(name);
          check::on_vulkan_error::n_assert(it != pipelines_map.end(), "could not find pipeline: " + name);
          return it->second.pc;
        }

        /// \brief Return the pipeline named name
        vk::pipeline &get_pipeline(const std::string &name)
        {
          auto it = pipelines_map.find(name);
          check::on_vulkan_error::n_assert(it != pipelines_map.end(), "could not find pipeline: " + name);
          return it->second.p;
        }

        /// \brief Return the pipeline named name
        const vk::pipeline &get_pipeline(const std::string &name) const
        {
          auto it = pipelines_map.find(name);
          check::on_vulkan_error::n_assert(it != pipelines_map.end(), "could not find pipeline: " + name);
          return it->second.p;
        }

        /// \brief Refresh a single pipeline
        void refresh(const std::string &name)
        {
          auto it = pipelines_map.find(name);
          check::on_vulkan_error::n_assert(it != pipelines_map.end(), "could not find pipeline: " + name);

          if (it->second.pc.allow_derivate_pipelines())
          {
            it->second.pc.set_base_pipeline(&it->second.p);
            it->second.p = it->second.pc.create_pipeline(dev);
            it->second.pc.set_base_pipeline(nullptr);
          }
          else
          {
            // slow path
            it->second.p = it->second.pc.create_pipeline(dev);
          }
        }

        /// \brief Recreate all the pipelines
        void refresh()
        {
          for (auto &it : pipelines_map)
          {
            if (it.second.pc.allow_derivate_pipelines())
            {
              it.second.pc.set_base_pipeline(&it.second.p);
              it.second.p = it.second.pc.create_pipeline(dev);
              it.second.pc.set_base_pipeline(nullptr);
            }
            else
            {
              // slow path
              it.second.p = it.second.pc.create_pipeline(dev);
            }
          }
        }

      private:
        vk::device &dev;
        std::map<std::string, pipeline_holder> pipelines_map;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_164028163982217544_8016717_PIPELINE_MANAGER_HPP__

