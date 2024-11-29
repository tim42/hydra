//
// file : shader_module.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/shader_module.hpp
//
// created by : Timothée Feuillet
// date: Wed Aug 10 2016 18:15:10 GMT+0200 (CEST)
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
#include <ntools/mt_check/vector.hpp>
#include <vulkan/vulkan.h>
#include <assets/spirv.hpp>

#include "device.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a vulkan shader module
      class shader_module
      {
        public: // advanced
          /// \brief Construct from a VkShaderModuleCreateInfo
          shader_module(device &_dev, const VkShaderModuleCreateInfo &create_info, uint32_t _stage, std::string _entry_point)
           : dev(_dev), entry_point(std::move(_entry_point)), stage(_stage)
          {
            check::on_vulkan_error::n_assert_success(dev._vkCreateShaderModule(&create_info, nullptr, &vk_shader_module));
          }

          /// \brief Construct from a VkShaderModule
          shader_module(device &_dev, VkShaderModule _vk_shader_module, uint32_t _stage, std::string _entry_point)
           : dev(_dev), vk_shader_module(_vk_shader_module), entry_point(std::move(_entry_point)), stage(_stage)
          {
          }

        public:
          /// \brief Construct the shader module from a spirv buffer
          /// There's some "rules" about the spirv_data parameter to obey, see
          ///   https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkShaderModuleCreateInfo.html
          shader_module(device &_dev, const uint32_t *spirv_data, size_t data_length, uint32_t _stage, std::string _entry_point)
           : shader_module(_dev, VkShaderModuleCreateInfo
             {
               VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
               nullptr,
               0,
               data_length,
               spirv_data
             }, _stage, std::move(_entry_point))
          {
          }

          /// \brief Move constructor
          shader_module(shader_module &&o)
           : dev(o.dev), vk_shader_module(o.vk_shader_module), entry_point(std::move(o.entry_point)),
              stage(o.stage), constant_id(std::move(o.constant_id)),
              push_constant_entries(std::move(o.push_constant_entries)),
              push_constant_ranges(std::move(o.push_constant_ranges)),
              descriptor_set(std::move(o.descriptor_set))
          {
            o.vk_shader_module = nullptr;
          }

          /// \brief move/assign
          shader_module &operator=(shader_module &&o)
          {
            check::on_vulkan_error::n_assert(&o.dev == &dev, "can't assign shader modules with different vulkan devices");
            if (&o == this)
              return *this;
            if (vk_shader_module)
              dev._vkDestroyShaderModule(vk_shader_module, nullptr);
            vk_shader_module = o.vk_shader_module;
            o.vk_shader_module = nullptr;
            stage = o.stage;
            entry_point = std::move(o.entry_point);
            constant_id = std::move(o.constant_id);
            push_constant_entries = std::move(o.push_constant_entries);
            push_constant_ranges = std::move(o.push_constant_ranges);
            descriptor_set = std::move(o.descriptor_set);
            return *this;
          }

          /// \brief Destructor
          ~shader_module()
          {
            if (vk_shader_module)
              dev._vkDestroyShaderModule(vk_shader_module, nullptr);
          }

          bool is_valid() const
          {
            return vk_shader_module != nullptr;
          }

          const std::string& get_entry_point() const { return entry_point; }
        public: // advanced
          VkShaderModule get_vk_shader_module()
          {
            return vk_shader_module;
          }

          void _set_debug_name(const std::string& name)
          {
            dev._set_object_debug_name((uint64_t)vk_shader_module, VK_OBJECT_TYPE_SHADER_MODULE, name);
          }

          auto& _get_constant_id_map() { return constant_id; }
          const auto& _get_constant_id_map() const { return constant_id; }

          auto& get_push_constant_entries() { return push_constant_entries; }
          const auto& get_push_constant_entries() const { return push_constant_entries; }

          auto& get_push_constant_ranges() { return push_constant_ranges; }
          const auto& get_push_constant_ranges() const { return push_constant_ranges; }

          auto& get_descriptor_sets() { return descriptor_set; }
          const auto& get_descriptor_sets() const { return descriptor_set; }

          VkShaderStageFlagBits get_stage() const { return (VkShaderStageFlagBits)stage; }
        private:
          device &dev;
          VkShaderModule vk_shader_module;
          std::string entry_point = "main";

          uint32_t stage;

          std::mtc_map<id_t, uint32_t> constant_id;
          std::mtc_map<id_t, assets::push_constant_entry> push_constant_entries;
          std::mtc_vector<assets::push_constant_range> push_constant_ranges;
          std::mtc_vector<assets::descriptor_set_entry> descriptor_set;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



