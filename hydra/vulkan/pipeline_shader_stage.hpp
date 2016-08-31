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

#ifndef __N_316313208783933411_2145825365_PIPELINE_SHADER_STAGE_HPP__
#define __N_316313208783933411_2145825365_PIPELINE_SHADER_STAGE_HPP__

#include <vector>
#include <deque>

#include <vulkan/vulkan.h>

#include "device.hpp"
#include "shader_module.hpp"
#include "specialization_info.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Hold some information about shaders of a given pipeline.
      /// This is meant to be used in/with a pipeline object.
      class pipeline_shader_stage
      {
        public:
          pipeline_shader_stage() {}
          pipeline_shader_stage(pipeline_shader_stage &&o) : vk_pssci(std::move(o.vk_pssci)), entry_points(std::move(o.entry_points)) {}
          pipeline_shader_stage(const pipeline_shader_stage &o) : vk_pssci(o.vk_pssci), entry_points(o.entry_points)
          {
            for (size_t i = 0; i < entry_points.size(); ++i)
              vk_pssci[i].pName = entry_points[i].data();
          }
          pipeline_shader_stage &operator = (pipeline_shader_stage &&o)
          {
            if (&o == this)
              return *this;
            vk_pssci = std::move(o.vk_pssci);
            entry_points = std::move(o.entry_points);
            return *this;
          }
          pipeline_shader_stage &operator = (const pipeline_shader_stage &o)
          {
            if (&o == this)
              return *this;
            vk_pssci = o.vk_pssci;
            entry_points = o.entry_points;
            for (size_t i = 0; i < entry_points.size(); ++i)
              vk_pssci[i].pName = entry_points[i].data();
            return *this;
          }

          ~pipeline_shader_stage() {}

          /// \brief Add a shader to the pipeline
          /// \note It is your duty to maintain the shader_module & the specialization_info (if specified) alive.
          ///       the entry_point parameter is kept alive by this class.
          pipeline_shader_stage &add_shader(shader_module &shader, VkShaderStageFlagBits stage, const std::string &entry_point = "main", specialization_info *nfo = nullptr)
          {
            entry_points.push_back(entry_point);
            vk_pssci.push_back(VkPipelineShaderStageCreateInfo
            {
              VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
              nullptr,
              0,
              stage,
              shader.get_vk_shader_module(),
              entry_points.back().data(),
              (nfo ? & ((VkSpecializationInfo &)(*nfo)) : nullptr)
            });
            return *this;
          }

          /// \brief Directly add a VkPipelineShaderStageCreateInfo structure into the array
          /// \note advanced
          pipeline_shader_stage &_add_shader(const VkPipelineShaderStageCreateInfo &pssci)
          {
            vk_pssci.push_back(pssci);
            return *this;
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
            entry_points.clear();
          }

        private:
          std::vector<VkPipelineShaderStageCreateInfo> vk_pssci;
          std::deque<std::string> entry_points;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_316313208783933411_2145825365_PIPELINE_SHADER_STAGE_HPP__

