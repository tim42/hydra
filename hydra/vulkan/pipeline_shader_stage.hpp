//
// file : pipeline_shader_stage.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/pipeline_shader_stage.hpp
//
// created by : Timothée Feuillet
// date: Thu Aug 11 2016 13:51:36 GMT+0200 (CEST)
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

#include <vector>
#include <deque>

#include <vulkan/vulkan.h>
#include <ntools/async/chain.hpp>

#include "device.hpp"
#include "shader_module.hpp"
#include "specialization_info.hpp"

namespace neam::hydra::vk
{
  class graphics_pipeline_creator;
  class compute_pipeline_creator;

  /// \brief Hold some information about shaders of a given pipeline.
  /// This is meant to be used in/with a pipeline object.
  class pipeline_shader_stage
  {
    public:
      pipeline_shader_stage(graphics_pipeline_creator& _creator) : creator(&_creator)
      {
        vk_pssci.reserve(4);
        shader_modules.reserve(4);
        specializations.reserve(4);
      }
      pipeline_shader_stage(compute_pipeline_creator& _creator) : creator(&_creator)
      {
        vk_pssci.reserve(4);
        shader_modules.reserve(4);
        specializations.reserve(4);
      }

      ~pipeline_shader_stage() {}

      pipeline_shader_stage &add_shader(async::chain<shader_module &>&& shader_chain, VkShaderStageFlagBits stage, specialization&& spec = {})
      {
        unsigned current_version = version_count;
        ++in_process;
        shader_chain.then([this, current_version, stage, spec = std::move(spec)](shader_module& shader)
        {
          if (version_count == current_version)
          {
            add_shader(shader, stage, spec);
            --in_process;
            if (!in_process)
              ask_pipeline_refresh();
          }
        });
        return *this;
      }

      /// \brief Add a shader to the pipeline
      /// \note It is your duty to maintain the shader_module & the specialization_info (if specified) alive.
      ///       the entry_point parameter is kept alive by this class.
      pipeline_shader_stage &add_shader(shader_module &shader, VkShaderStageFlagBits stage, const specialization& spec = {})
      {
        specializations.push_back({spec, shader._get_constant_id_map()});

        shader_modules.push_back(&shader);
        vk_pssci.push_back(VkPipelineShaderStageCreateInfo
        {
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          nullptr,
          0,
          stage,
          shader.get_vk_shader_module(),
          shader.get_entry_point().c_str(),
          specializations.back(),
        });
        return *this;
      }

      /// \brief Apply the same specialization to all the stages. Is not cumulative. (previous stage is lost)
      void specialize(const specialization& spec)
      {
        for (unsigned i = 0; i < vk_pssci.size(); ++i)
        {
          specializations[i].update(spec, shader_modules[i]->_get_constant_id_map());
        }
      }

      /// \brief Apply a specialization to a set of stages
      void specialize(VkShaderStageFlagBits stages, const specialization& spec)
      {
        for (unsigned i = 0; i < vk_pssci.size(); ++i)
        {
          if ((vk_pssci[i].stage & stages) == vk_pssci[i].stage)
          {
            specializations[i].update(spec, shader_modules[i]->_get_constant_id_map());
          }
        }
      }

      /// \brief For a better integration with the vulkan API
      operator VkPipelineShaderStageCreateInfo *() { return vk_pssci.size() ? vk_pssci.data() : nullptr; }
      /// \brief For a better integration with the vulkan API
      operator const VkPipelineShaderStageCreateInfo *() const { return vk_pssci.size() ? vk_pssci.data() : nullptr; }

      /// \brief Return the number of shader stage specified in this pipeline_shader_stage object
      size_t get_shader_stage_count() const { return vk_pssci.size(); }

      /// \brief Clear the shader pipeline
      void clear()
      {
        vk_pssci.clear();
        shader_modules.clear();
        ++version_count;
        in_process = 0;
      }

      void refresh()
      {
        for (unsigned i = 0; i < vk_pssci.size(); ++i)
        {
          vk_pssci[i].pName = shader_modules[i]->get_entry_point().c_str();
          vk_pssci[i].module = shader_modules[i]->get_vk_shader_module();
          vk_pssci[i].pSpecializationInfo = specializations[i];
        }
      }

      bool is_valid() const
      {
        if (has_async_operations_in_process())
          return false;
        for (const auto& it : shader_modules)
        {
          if (!it->is_valid())
            return false;
        }
        return true;
      }

      bool has_async_operations_in_process() const
      {
        return in_process != 0;
      }

    private:
      void ask_pipeline_refresh();

    private:
      std::variant<graphics_pipeline_creator*, compute_pipeline_creator*> creator;

      std::vector<VkPipelineShaderStageCreateInfo> vk_pssci;
      std::vector<shader_module*> shader_modules;
      std::vector<specialization_info> specializations;
      unsigned version_count = 0;
      unsigned in_process = 0;
  };
}
