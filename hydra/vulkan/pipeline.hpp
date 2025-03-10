//
// file : pipeline.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/pipeline.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 13 2016 13:56:21 GMT+0200 (CEST)
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


#include <unordered_map>
#include <vulkan/vulkan.h>
#include <hydra_glm.hpp>
#include <ntools/mt_check/mt_check_base.hpp>


#include "../hydra_debug.hpp"

#include "pipeline_multisample_state.hpp"
#include "pipeline_shader_stage.hpp"
#include "pipeline_viewport_state.hpp"
#include "pipeline_color_blending.hpp"
#include "pipeline_dynamic_state.hpp"
#include "pipeline_layout.hpp"
#include "pipeline_vertex_input_state.hpp"
#include "pipeline_input_assembly_state.hpp"
#include "pipeline_cache.hpp"
#include "pipeline_rendering_create_info.hpp"

#include "rasterizer.hpp"
#include "viewport.hpp"
#include "framebuffer.hpp"
#include "render_pass.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkPipeline object
      class pipeline : public cr::mt_checked<pipeline>
      {
        public: // advanced
          /// \brief Construct the pipeline from a vulkan object
          pipeline(device &_dev, VkPipeline _pipeline, VkPipelineBindPoint _bind_point) : dev(_dev), vk_pipeline(_pipeline), bind_point(_bind_point) {}
        public:
          pipeline(pipeline &&o)
            : dev(o.dev), vk_pipeline(o.vk_pipeline), bind_point(o.bind_point), pipeline_id(o.pipeline_id)
            , cpp_struct_to_set(std::move(o.cpp_struct_to_set))
          {
            o.vk_pipeline = nullptr;
          }
          pipeline& operator = (pipeline&& o)
          {
            N_MTC_WRITER_SCOPE;
            N_MTC_WRITER_SCOPE_OBJ(o);
            check::on_vulkan_error::n_assert(&o.dev == &dev, "can't assign pipelines with different vulkan devices");
            if (vk_pipeline)
              dev._vkDestroyPipeline(vk_pipeline, nullptr);

            vk_pipeline = o.vk_pipeline;
            o.vk_pipeline = nullptr;
            bind_point = o.bind_point;
            pipeline_id = o.pipeline_id;
            cpp_struct_to_set = std::move(o.cpp_struct_to_set);
            return *this;
          }

          ~pipeline()
          {
            N_MTC_WRITER_SCOPE;
            if (vk_pipeline)
              dev._vkDestroyPipeline(vk_pipeline, nullptr);
          }

          bool is_valid() const { N_MTC_READER_SCOPE; return vk_pipeline != nullptr; }

          void _set_debug_name(const std::string& name)
          {
            N_MTC_WRITER_SCOPE;
            dev._set_object_debug_name((uint64_t)vk_pipeline, VK_OBJECT_TYPE_PIPELINE, name);
          }

        public:
          uint32_t get_set_for_struct(id_t struct_id) const
          {
            N_MTC_READER_SCOPE;
            if (auto it = cpp_struct_to_set.find(struct_id); it != cpp_struct_to_set.end())
              return it->second;
            return ~0u;
          }
          string_id get_pipeline_id() const { N_MTC_READER_SCOPE; return pipeline_id; }

        public: // advanced
          /// \brief Return the VkPipeline
          VkPipeline get_vk_pipeline() const
          {
            N_MTC_READER_SCOPE;
            return vk_pipeline;
          }
          VkPipelineBindPoint get_pipeline_bind_point() const
          {
            N_MTC_READER_SCOPE;
            return bind_point;
          }

          void _set_cpp_struct_to_set(std::unordered_map<id_t, uint32_t> map)
          {
            N_MTC_WRITER_SCOPE;
            cpp_struct_to_set = std::move(map);
          }
          void _set_pipeline_id(string_id pid)
          {
            N_MTC_WRITER_SCOPE;
            pipeline_id = pid;
          }
        private:
          device &dev;
          VkPipeline vk_pipeline;
          VkPipelineBindPoint bind_point;
          string_id pipeline_id;
          std::unordered_map<id_t, uint32_t> cpp_struct_to_set;
      };

      /// \brief A pipeline creator object. It creates pipelines.
      /// (wraps VkGraphicsPipelineCreateInfo)
      class graphics_pipeline_creator
      {
        public:
          graphics_pipeline_creator(device& _dev) : dev(_dev), pss(*this) {}

          /// \brief Return the shader stages of the pipeline
          pipeline_shader_stage &get_pipeline_shader_stage() { return pss; }
          /// \brief Return the shader stages of the pipeline
          const pipeline_shader_stage &get_pipeline_shader_stage() const { return pss; }

          /// \brief Return the pipeline vertex input state
          pipeline_vertex_input_state &get_vertex_input_state() { return pvis; }
          /// \brief Return the pipeline vertex input state
          const pipeline_vertex_input_state &get_vertex_input_state() const { return pvis; }

          /// \brief Return the pipeline input assembly state
          pipeline_input_assembly_state &get_input_assembly_state() { return pias; }
          /// \brief Return the pipeline input assembly state
          const pipeline_input_assembly_state &get_input_assembly_state() const { return pias; }

          /// \brief Return the pipeline viewport state
          pipeline_viewport_state &get_viewport_state() { return pvs; }
          /// \brief Return the pipeline viewport state
          const pipeline_viewport_state &get_viewport_state() const { return pvs; }

          /// \brief Return the pipeline rasterization state
          rasterizer &get_pipeline_rasterization_state() { return prs; }
          /// \brief Return the pipeline rasterization state
          const rasterizer &get_pipeline_rasterization_state() const { return prs; }

          /// \brief Return the pipeline multisample state
          pipeline_multisample_state &get_pipeline_multisample_state() { return pms; }
          /// \brief Return the pipeline multisample state
          const pipeline_multisample_state &get_pipeline_multisample_state() const { return pms; }

          /// \brief Set a reference to an external pipeline multisample state (that way you can share the same multisampling settings
          /// in different pipelines and recreate them if needed).
          /// Disable this by setting _ref_pms to nullptr.
          /// \note It will superseed the default pipeline_multisample_state object
          void set_pipeline_multisample_state_ptr(pipeline_multisample_state *_ref_pms) { ref_pms = _ref_pms; }

          /// \brief Return the pipeline color blending state
          pipeline_color_blending &get_pipeline_color_blending_state() { return pcb; }
          /// \brief Return the pipeline color blending state
          const pipeline_color_blending &get_pipeline_color_blending_state() const { return pcb; }
          // TODO: maybe add something like the multisample state pointer (that way you may define color blending
          //       per attachment in the framebuffer

          /// \brief Return the pipeline dynamic states
          pipeline_dynamic_state &get_pipeline_dynamic_state() { return pds; }
          /// \brief Return the pipeline dynamic states
          const pipeline_dynamic_state &get_pipeline_dynamic_state() const { return pds; }

          /// \brief Return the pipeline layout
          pipeline_layout &get_pipeline_layout() const { return *layout; }
          /// \brief Set the pipeline layout
          void set_pipeline_layout(pipeline_layout &_layout) { layout = &_layout; }

          /// \brief Set the render pass
          void set_render_pass(const render_pass &_rp) { rp = &_rp; }


          /// \brief Set the subpass used by the pipeline
          void set_subpass_index(size_t subpass_index) { create_info.subpass = subpass_index; }

          void clear_render_pass() { rp = nullptr; create_info.subpass = 0; }

          void set_pipeline_create_info(const pipeline_rendering_create_info& _prci)
          {
            clear_render_pass();
            prci = _prci;
          }

          /// \brief Return the base pipeline (could be nullptr if none). Creating derivate pipelines
          /// may allow faster transition between derivates of the same pipeline
          pipeline *get_base_pipeline() const { return base_pipeline; }
          /// \brief Set the base pipeline (could be nullptr if none). Creating derivate pipelines
          /// may allow faster transition between derivates of the same pipeline
          void set_base_pipeline(pipeline *_base_pipeline)
          {
            base_pipeline = _base_pipeline;

//             if (base_pipeline)
//               create_info.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
//             else
//               create_info.flags &= ~VK_PIPELINE_CREATE_DERIVATIVE_BIT;
          }

          /// \brief Allow (or disallow) the pipeline to be derived (default: false)
          void allow_derivate_pipelines(bool /*allow*/)
          {
//             if (allow)
//               create_info.flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
//             else
//               create_info.flags &= ~VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
          }

          /// \brief Get the allow derivate state
          bool allow_derivate_pipelines() const
          {
            return create_info.flags & VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
          }

          /// \brief Set custom flags (the fags controlling the inheritence are set automatically)
          void set_flags(VkPipelineCreateFlags flags) { create_info.flags = flags; }
          /// \brief Get the pipeline flags
          VkPipelineCreateFlags get_flags() const { return create_info.flags; }

          /// \brief Create a new pipeline
          pipeline create_pipeline(pipeline_cache *cache = nullptr)
          {
            // refresh structs
            pvs.refresh();
            pcb.refresh();
            pss.refresh();

            if (!pss.is_valid())
            {
              if (!pss.has_async_operations_in_process())
                neam::cr::out().error("hydra::graphics_pipeline_creator: Trying to create a graphic pipeline with invalid shader stages");
              else
                neam::cr::out().debug("hydra::graphics_pipeline_creator: Waiting for async operation to finish (yielding empty pipeline)");
              return pipeline(dev, nullptr, VK_PIPELINE_BIND_POINT_GRAPHICS);
            }

            // update the create_info
            create_info.stageCount = pss.get_shader_stage_count();
            create_info.pStages = pss;

            pds.remove(VK_DYNAMIC_STATE_VIEWPORT);
            pds.remove(VK_DYNAMIC_STATE_SCISSOR);

            if (pvs.uses_dynamic_viewports())
              pds.add(VK_DYNAMIC_STATE_VIEWPORT);
            if (pvs.uses_dynamic_scissors())
              pds.add(VK_DYNAMIC_STATE_SCISSOR);

            create_info.pVertexInputState = &static_cast<const VkPipelineVertexInputStateCreateInfo &>(pvis);
            create_info.pInputAssemblyState = &static_cast<const VkPipelineInputAssemblyStateCreateInfo &>(pias);
//             create_info.pTessellationState = &static_cast<const VkPipelineTessellationStateCreateInfo &>(pts); // TODO
            create_info.pViewportState = &static_cast<const VkPipelineViewportStateCreateInfo &>(pvs);
            create_info.pRasterizationState = &static_cast<const VkPipelineRasterizationStateCreateInfo &>(prs);
            create_info.pMultisampleState = &static_cast<const VkPipelineMultisampleStateCreateInfo &>(pms);
//             create_info.pDepthStencilState = &static_cast<const VkPipelineDepthStencilStateCreateInfo &>(pdss); // TODO
            create_info.pColorBlendState = &static_cast<const VkPipelineColorBlendStateCreateInfo &>(pcb);
            if (pds.has_dynamic_states())
              create_info.pDynamicState = &static_cast<const VkPipelineDynamicStateCreateInfo &>(pds);
            else
              create_info.pDynamicState = nullptr;

            check::on_vulkan_error::n_assert(layout != nullptr, "could not create a pipeline without a valid layout");

            create_info.layout = layout->_get_vk_pipeline_layout();
            check::on_vulkan_error::n_assert(create_info.layout != nullptr, "could not create a pipeline without a valid layout");

            if (rp != nullptr)
            {
              create_info.pNext = nullptr;
              create_info.renderPass = rp->get_vk_render_pass();
            }
            else
            {
              create_info.pNext = &prci._get_vk_info();
              create_info.renderPass = nullptr;
            }
//             if (base_pipeline)
//               create_info.basePipelineHandle = base_pipeline->get_vk_pipeline();
//             else
              create_info.basePipelineHandle = nullptr;
            create_info.basePipelineIndex = (int32_t)-1;

            VkPipeline p;
            VkPipelineCache pcache = nullptr;
            if (cache)
              pcache = cache->get_vk_pipeline_cache();
            check::on_vulkan_error::n_assert_success(dev._vkCreateGraphicsPipelines(pcache, 1, &create_info, nullptr, &p));

            return pipeline(dev, p, VK_PIPELINE_BIND_POINT_GRAPHICS);
          }

          bool is_dirty() const { return dirty; }
          void set_dirty(bool _is_dirty = true) { dirty = _is_dirty; }

          bool is_pss_valid() const { return pss.is_valid(); }
          bool has_async_operations_in_process() const
          {
            return pss.is_valid() && pss.has_async_operations_in_process();
          }

        private:
          device& dev;
          VkGraphicsPipelineCreateInfo create_info = VkGraphicsPipelineCreateInfo
          {
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0,
            0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr,
            0, nullptr, -1
          };

          pipeline_shader_stage pss;
          pipeline_vertex_input_state pvis;
          pipeline_input_assembly_state pias;
          // TODO: tesselation
          pipeline_viewport_state pvs;
          rasterizer prs;
          pipeline_multisample_state pms;
          pipeline_multisample_state* ref_pms = nullptr;
          // TODO: depth & stencil
          pipeline_color_blending pcb;
          pipeline_dynamic_state pds;

          pipeline_rendering_create_info prci;

          pipeline_layout* layout = nullptr;
          const render_pass* rp = nullptr;

          pipeline* base_pipeline = nullptr;
          bool dirty = true;
      };

      /// \brief A pipeline creator object. It creates pipelines.
      /// (wraps VkComputePipelineCreateInfo)
      class compute_pipeline_creator
      {
        public:
          compute_pipeline_creator(device& _dev) : dev(_dev), pss(*this) {}

          /// \brief Return the shader stages of the pipeline
          pipeline_shader_stage &get_pipeline_shader_stage() { return pss; }
          /// \brief Return the shader stages of the pipeline
          const pipeline_shader_stage &get_pipeline_shader_stage() const { return pss; }

          /// \brief Return the pipeline layout
          pipeline_layout &get_pipeline_layout() const { return *layout; }
          /// \brief Set the pipeline layout
          void set_pipeline_layout(pipeline_layout &_layout) { layout = &_layout; }

          /// \brief Return the base pipeline (could be nullptr if none). Creating derivate pipelines
          /// may allow faster transition between derivates of the same pipeline
          pipeline *get_base_pipeline() const { return base_pipeline; }
          /// \brief Set the base pipeline (could be nullptr if none). Creating derivate pipelines
          /// may allow faster transition between derivates of the same pipeline
          void set_base_pipeline(pipeline *_base_pipeline)
          {
            base_pipeline = _base_pipeline;

//             if (base_pipeline)
//               create_info.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
//             else
//               create_info.flags &= ~VK_PIPELINE_CREATE_DERIVATIVE_BIT;
          }

          /// \brief Allow (or disallow) the pipeline to be derived (default: false)
          void allow_derivate_pipelines(bool /*allow*/)
          {
//             if (allow)
//               create_info.flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
//             else
//               create_info.flags &= ~VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
          }

          /// \brief Get the allow derivate state
          bool allow_derivate_pipelines() const
          {
            return create_info.flags & VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
          }

          /// \brief Set custom flags (the fags controlling the inheritence are set automatically)
          void set_flags(VkPipelineCreateFlags flags) { create_info.flags = flags; }
          /// \brief Get the pipeline flags
          VkPipelineCreateFlags get_flags() const { return create_info.flags; }

          /// \brief Create a new pipeline
          pipeline create_pipeline(pipeline_cache *cache = nullptr)
          {
            // refresh structs
            pss.refresh();

            if (!pss.is_valid())
            {
              if (!pss.has_async_operations_in_process())
                neam::cr::out().error("hydra::compute_pipeline_creator: Trying to create a compute pipeline with invalid shader stages");
              else
                neam::cr::out().debug("hydra::compute_pipeline_creator: Waiting for async operation to finish (yielding empty pipeline)");
              return pipeline(dev, nullptr, VK_PIPELINE_BIND_POINT_COMPUTE);
            }

            // update the create_info
            check::on_vulkan_error::n_assert(pss.get_shader_stage_count() == 1, "could not create a compute pipeline with anything other than one stage (stage count: {})", pss.get_shader_stage_count());
            create_info.stage = *pss;

            check::on_vulkan_error::n_assert(layout != nullptr, "could not create a pipeline without a valid layout");

            create_info.layout = layout->_get_vk_pipeline_layout();

//             if (base_pipeline)
//               create_info.basePipelineHandle = base_pipeline->get_vk_pipeline();
//             else
              create_info.basePipelineHandle = nullptr;
            create_info.basePipelineIndex = (int32_t)-1;

            VkPipeline p;
            VkPipelineCache pcache = nullptr;
            if (cache)
              pcache = cache->get_vk_pipeline_cache();
            check::on_vulkan_error::n_assert_success(dev._vkCreateComputePipelines(pcache, 1, &create_info, nullptr, &p));

            return pipeline(dev, p, VK_PIPELINE_BIND_POINT_COMPUTE);
          }

          bool is_dirty() const { return dirty; }
          void set_dirty(bool _is_dirty = true) { dirty = _is_dirty; }

          bool is_pss_valid() const { return pss.is_valid(); }
          bool has_async_operations_in_process() const
          {
            return pss.is_valid() && pss.has_async_operations_in_process();
          }

        private:
          device &dev;
          VkComputePipelineCreateInfo create_info = VkComputePipelineCreateInfo
          {
            VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0, {}, 0, 0, -1
          };

          pipeline_shader_stage pss;

          pipeline_layout* layout = nullptr;

          pipeline* base_pipeline = nullptr;
          bool dirty = true;
      };

      inline void pipeline_shader_stage::ask_pipeline_refresh()
      {
        std::visit([](auto* c)
        {
          c->set_dirty();
        }, creator);
      }
    } // namespace vk
  } // namespace hydra
} // namespace neam



