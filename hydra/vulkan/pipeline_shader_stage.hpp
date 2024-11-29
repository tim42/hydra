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
#include <variant>

#include <vulkan/vulkan.h>
#include <ntools/async/chain.hpp>
#include <ntools/spinlock.hpp>
#include <ntools/mt_check/vector.hpp>

#include "device.hpp"
#include "shader_module.hpp"
#include "specialization_info.hpp"
#include "descriptor_set_layout_binding.hpp"

namespace neam::hydra::vk
{
  class graphics_pipeline_creator;
  class compute_pipeline_creator;

  struct push_constant_entry
  {
    uint32_t offset;
    VkFlags stages;
  };

  struct descriptor_set_entries
  {
    uint32_t binding;
    VkFlags stages;
  };

  /// \brief Hold some information about shaders of a given pipeline.
  /// This is meant to be used in/with a pipeline object.
  class pipeline_shader_stage
  {
    public:
      pipeline_shader_stage(graphics_pipeline_creator& _creator) : creator(&_creator)
      {
      }
      pipeline_shader_stage(compute_pipeline_creator& _creator) : creator(&_creator)
      {
      }

      ~pipeline_shader_stage() {clear();}

      pipeline_shader_stage& add_shader(async::chain<shader_module&>&& shader_chain, specialization&& spec = {})
      {
        unsigned current_version = version_count;
        in_process.fetch_add(1, std::memory_order_release);
        shader_chain.then([this, current_version, spec = std::move(spec)](shader_module& shader)
        {
          if (version_count == current_version)
          {
            add_shader(shader, spec);
            if (in_process.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
              ask_pipeline_refresh();
              std::lock_guard _lg { lock };
              in_progress_chains.clear();
            }

          }
        });
        if (in_process.load(std::memory_order_acquire) > 0)
        {
          std::lock_guard _lg { lock };
          in_progress_chains.push_back(std::move(shader_chain));
        }

        return *this;
      }

      /// \brief Add a shader to the pipeline
      /// \note It is your duty to maintain the shader_module & the specialization_info (if specified) alive.
      ///       the entry_point parameter is kept alive by this class.
      pipeline_shader_stage& add_shader(shader_module& shader, const specialization& spec = {})
      {
        std::lock_guard _lg { lock };
        specializations.push_back({spec, shader._get_constant_id_map()});

        check::debug::n_check(!shader.is_valid() || shader.get_stage() != 0, "invalid shader stage in add_shader()");

        shader_modules.push_back(&shader);
        vk_pssci.push_back(VkPipelineShaderStageCreateInfo
        {
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          nullptr,
          0,
          shader.get_stage(),
          shader.get_vk_shader_module(),
          shader.get_entry_point().c_str(),
          specializations.back(),
        });
        return *this;
      }

      /// \brief Apply the same specialization to all the stages. Is not cumulative. (previous stage is lost)
      void specialize(const specialization& spec)
      {
        std::lock_guard _lg { lock };
        for (unsigned i = 0; i < vk_pssci.size(); ++i)
        {
          specializations[i].update(spec, shader_modules[i]->_get_constant_id_map());
        }
      }

      /// \brief Apply a specialization to a set of stages
      void specialize(VkShaderStageFlagBits stages, const specialization& spec)
      {
        std::lock_guard _lg { lock };
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
        std::lock_guard _lg { lock };
        vk_pssci.clear();
        shader_modules.clear();
        ++version_count;
        in_process.store(0, std::memory_order_release);
        for (auto& it : in_progress_chains)
          it.cancel();
        in_progress_chains.clear();
      }

      void refresh()
      {
        std::lock_guard _lg { lock };
        for (unsigned i = 0; i < vk_pssci.size(); ++i)
        {
          vk_pssci[i].pName = shader_modules[i]->get_entry_point().c_str();
          vk_pssci[i].module = shader_modules[i]->get_vk_shader_module();
          vk_pssci[i].stage = shader_modules[i]->get_stage();
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
        return in_process.load(std::memory_order_acquire) != 0;
      }

    public: // shader modules reflection ops:
      std::vector<VkPushConstantRange> compute_combined_push_constant_range() const
      {
        std::vector<id_t> ids;
        std::vector<VkPushConstantRange> ret;
        std::lock_guard _lg { lock };
        for (const auto* mod : shader_modules)
        {
          for (auto&& it : mod->get_push_constant_ranges())
          {
            bool found = false;
            // find if a range matches:
            for (uint32_t i = 0; i < ret.size(); ++i)
            {
              if (ids[i] == it.id)
              {
                found = true;
                ret[i].stageFlags |= mod->get_stage();
                break;
              }
              // FIXME: Check for overlaps (there should not be overlaps)
            }
            if (!found)
            {
              ret.push_back({ .stageFlags = mod->get_stage(), .offset = /*it.offset*/0, .size = it.size, });
              ids.push_back(it.id);
            }
          }
        }
        return ret;
      }

      std::map<id_t, push_constant_entry> compute_push_constant_entries() const
      {
        std::map<id_t, push_constant_entry> ret;
        std::lock_guard _lg { lock };
        for (const auto* mod : shader_modules)
        {
          for (auto&& it : mod->get_push_constant_entries())
          {
            if (auto oit = ret.find(it.first); oit != ret.end())
            {
              if ((oit->second.offset != it.second.offset))
              {
                check::debug::n_check(false, "compute_push_constant_entries: identically named entries are at different offsets in different stages");
                return {};
              }
              else
              {
                oit->second.stages |= mod->get_stage();
              }
            }
            else
            {
              ret.emplace(it.first, push_constant_entry{ .offset = it.second.offset, .stages = mod->get_stage()});
            }
          }
        }
        return ret;
      }

      std::vector<id_t> compute_descriptor_sets()
      {
        std::lock_guard _lg { lock };
        std::map<uint32_t, id_t> dse_set;
        uint32_t max_set = 0;
        for (const auto* mod : shader_modules)
        {
          for (const auto& ds : mod->get_descriptor_sets())
          {
            max_set = std::max(max_set, ds.set);
            if (auto it = dse_set.find(ds.set); it != dse_set.end())
            {
              if (it->second != ds.id)
              {
                check::debug::n_check(false, "compute_descriptor_sets: descriptor set for set {} are different ({} vs {})", ds.set, string_id::_from_id_t(ds.id), string_id::_from_id_t(it->second));
                return {};
              }
            }
            else
            {
              dse_set.emplace(ds.set, ds.id);
            }
          }
        }
        std::vector<id_t> dse;
        dse.resize(max_set + 1, id_t::none);
        for (uint32_t i = 0; i <= max_set; ++i)
        {
          if (auto it = dse_set.find(i); it != dse_set.end())
            dse[i] = it->second;
        }
        return dse;
      }

    private:
      void ask_pipeline_refresh();

    private:
      std::variant<graphics_pipeline_creator*, compute_pipeline_creator*> creator;

      mutable spinlock lock;
      std::mtc_vector<VkPipelineShaderStageCreateInfo> vk_pssci;
      std::mtc_vector<shader_module*> shader_modules;
      std::mtc_vector<specialization_info> specializations;
      std::mtc_vector<async::chain<shader_module&>> in_progress_chains;

      unsigned version_count = 0;
      std::atomic<unsigned> in_process = 0;
  };
}
