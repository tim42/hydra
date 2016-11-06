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

#ifndef __N_21334102461211220665_1439814387_PIPELINE_HPP__
#define __N_21334102461211220665_1439814387_PIPELINE_HPP__

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "../hydra_exception.hpp"

#include "pipeline_multisample_state.hpp"
#include "pipeline_shader_stage.hpp"
#include "pipeline_viewport_state.hpp"
#include "pipeline_color_blending.hpp"
#include "pipeline_dynamic_state.hpp"
#include "pipeline_layout.hpp"
#include "pipeline_vertex_input_state.hpp"
#include "pipeline_input_assembly_state.hpp"
#include "pipeline_cache.hpp"

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
      class pipeline
      {
        public: // advanced
          /// \brief Construct the pipeline from a vulkan object
          pipeline(device &_dev, VkPipeline _pipeline) : dev(_dev), vk_pipeline(_pipeline) {}
        public:
          pipeline(pipeline &&o) : dev(o.dev), vk_pipeline(o.vk_pipeline) { o.vk_pipeline = nullptr; }
          pipeline &operator = (pipeline &&o)
          {
            check::on_vulkan_error::n_assert(&o.dev == &dev, "can't assign pipelines with different vulkan devices");
            if (vk_pipeline)
              dev._vkDestroyPipeline(vk_pipeline, nullptr);

            vk_pipeline = o.vk_pipeline;
            o.vk_pipeline = nullptr;
            return *this;
          }

          ~pipeline()
          {
            if (vk_pipeline)
              dev._vkDestroyPipeline(vk_pipeline, nullptr);
          }

        public: // advanced
          /// \brief Return the VkPipeline
          VkPipeline get_vk_pipeline() const { return vk_pipeline; }
        private:
          device &dev;
          VkPipeline vk_pipeline;
      };

      /// \brief A pipeline creator object. Used to create pipelines
      /// (wraps a VkGraphicsPipelineCreateInfo)
      class pipeline_creator
      {
        public:
          // no default constructors / copy things / ... 'cause the compiler is able to do it for me

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

          /// \brief Return the render pass
          render_pass &get_render_pass() const { return *rp; }
          /// \brief Set the render pass
          void set_render_pass(render_pass &_rp) { rp = &_rp; }

          /// \brief Set the subpass used by the pipeline
          size_t get_subpass_index() const { return create_info.subpass; }
          /// \brief Return te subpass used by the pipeline
          void set_subpass_index(size_t subpass_index) { create_info.subpass = subpass_index; }

          /// \brief Return the base pipeline (could be nullptr if none). Creating derivate pipelines
          /// may allow faster transition between derivates of the same pipeline
          pipeline *get_base_pipeline() const { return base_pipeline; }
          /// \brief Set the base pipeline (could be nullptr if none). Creating derivate pipelines
          /// may allow faster transition between derivates of the same pipeline
          void set_base_pipeline(pipeline *_base_pipeline)
          {
            base_pipeline = _base_pipeline;

            if (base_pipeline)
              create_info.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
            else
              create_info.flags &= ~VK_PIPELINE_CREATE_DERIVATIVE_BIT;
          }

          /// \brief Allow (or disallow) the pipeline to be derived (default: false)
          void allow_derivate_pipelines(bool allow)
          {
            if (allow)
              create_info.flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
            else
              create_info.flags &= ~VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
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
          pipeline create_pipeline(device &dev, pipeline_cache *cache = nullptr)
          {
            // refresh structs
            pvs.refresh();
            pcb.refresh();

            // update the create_info
            create_info.stageCount = pss.get_shader_stage_count();
            create_info.pStages = pss;

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
            check::on_vulkan_error::n_assert(rp != nullptr, "could not create a pipeline without a valid render pass");

            create_info.layout = layout->_get_vk_pipeline_layout();
            create_info.renderPass = rp->get_vk_render_pass();
            if (base_pipeline)
              create_info.basePipelineHandle = base_pipeline->get_vk_pipeline();
            else
              create_info.basePipelineHandle = nullptr;
            create_info.basePipelineIndex = (int32_t)-1;

            VkPipeline p;
            VkPipelineCache pcache = nullptr;
            if (cache)
              pcache = cache->get_vk_pipeline_cache();
            check::on_vulkan_error::n_throw_exception(dev._vkCreateGraphicsPipelines(pcache, 1, &create_info, nullptr, &p));

            return pipeline(dev, p);
          }

        private:
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
          pipeline_multisample_state *ref_pms = nullptr;
          // TODO: depth & stencil
          pipeline_color_blending pcb;
          pipeline_dynamic_state pds;

          pipeline_layout *layout = nullptr;
          render_pass *rp = nullptr;

          pipeline *base_pipeline = nullptr;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_21334102461211220665_1439814387_PIPELINE_HPP__

