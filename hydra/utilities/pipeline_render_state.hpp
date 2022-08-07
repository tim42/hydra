//
// created by : Timothée Feuillet
// date: 2022-7-8
//
//
// Copyright (c) 2022 Timothée Feuillet
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

#include <hydra/vulkan/pipeline.hpp>
#include <hydra/vulkan/render_pass.hpp>
#include <hydra/vulkan/descriptor_set_layout.hpp>
#include <hydra/vulkan/descriptor_pool.hpp>
#include <hydra/geometry/mesh.hpp>

namespace neam::hydra
{
  /// \brief {pipeline, render-pass}* and a ds-pool, combined
  class pipeline_render_state
  {
    public:
      pipeline_render_state(vk::device& _dev) : dev(_dev) {}

      /// \brief Setup the ds-layout, ds-pool and pipeline_layout.
      /// \warning do not use the pool/ds-layout before calling this function
      void create_pipeline_info(vk::descriptor_set_layout&& _ds_layout, vk::descriptor_pool&& _ds_pool, vk::pipeline_layout&& _pipeline_layout)
      {
        ds_layout = std::move(_ds_layout);
        ds_pool = std::move(_ds_pool);
        pipeline_layout = std::move(_pipeline_layout);

        // we recreate the ds-layout, so we must invalidate the pipelines
        invalidate_pipelines();
      }

      template<typename... Args>
      vk::descriptor_set allocate_descriptor_set(Args&&... args)
      {
        return ds_pool.allocate_descriptor_set(ds_layout, std::forward<Args>(args)...);
      }
      vk::pipeline_layout& get_pipeline_layout()
      {
        return pipeline_layout;
      }

      /// \brief Immedialty invalidate the existing pipelines, forcing their re-creation
      void invalidate_pipelines()
      {
        pipelines.clear();
      }

      /// \brief invalidate the existing pipelines using the vrd
      /// \note This is the better way to invalidate stuff
      void invalidate_pipelines(vk_resource_destructor& vrd, vk::queue& queue)
      {
        std::map<id_t, vk::pipeline> temp_pipelines;
        std::swap(pipelines, temp_pipelines);
        vrd.postpone_destruction_to_next_fence(queue, std::move(temp_pipelines));
      }

      /// \brief Return the pipeline creator
      /// If you modify it, please call invalidate_pipelines() to force a reload of the pipelines
      vk::graphics_pipeline_creator& get_graphics_pipeline_creator()
      {
        if (std::holds_alternative<std::monostate>(pcr))
        {
          auto& gpcr = pcr.emplace<vk::graphics_pipeline_creator>(dev);
          gpcr.set_pipeline_layout(pipeline_layout);
          return gpcr;
        }
        if (std::holds_alternative<vk::graphics_pipeline_creator>(pcr))
          return std::get<vk::graphics_pipeline_creator>(pcr);

        check::on_vulkan_error::n_assert(false, "Trying to get a graphics pipeline on a PRS that hold something else");
        __builtin_unreachable();
      }

      /// \brief Return the pipeline creator
      /// If you modify it, please call invalidate_pipelines() to force a reload of the pipelines
      vk::compute_pipeline_creator& get_compute_pipeline_creator()
      {
        if (std::holds_alternative<std::monostate>(pcr))
        {
          auto& cpcr = pcr.emplace<vk::compute_pipeline_creator>(dev);
          cpcr.set_pipeline_layout(pipeline_layout);
          return cpcr;
        }
        if (std::holds_alternative<vk::compute_pipeline_creator>(pcr))
          return std::get<vk::compute_pipeline_creator>(pcr);

        check::on_vulkan_error::n_assert(false, "Trying to get a graphics pipeline on a PRS that hold something else");
        __builtin_unreachable();
      }

      /// \brief Create or retrieve a pipeline that does not require a render-pass
      const vk::pipeline& get_pipeline(const vk::specialization& spec = {})
      {
        if (std::holds_alternative<std::monostate>(pcr))
          check::on_vulkan_error::n_assert(false, "Trying to get a pipeline with a non-initialized pipeline creator");

        const id_t hash = spec.hash();
        std::visit([this](auto& xpcr)
        {
          if constexpr (std::is_same_v<std::monostate&, decltype(xpcr)>) __builtin_unreachable();
          else
          {
            if (xpcr.is_dirty())
            {
              // fixme: non-immediate mode
              invalidate_pipelines();
              xpcr.set_dirty(false);
            }
          }
        }, pcr);
        if (auto it = pipelines.find(hash); it != pipelines.end())
        {
          return it->second;
        }

        if (std::holds_alternative<vk::graphics_pipeline_creator>(pcr))
          std::get<vk::graphics_pipeline_creator>(pcr).clear_render_pass();

        return std::visit([this, hash, &spec](auto & xpcr) -> const vk::pipeline&
        {
          if constexpr (std::is_same_v<std::monostate&, decltype(xpcr)>) __builtin_unreachable();
          else
          {
            xpcr.get_pipeline_shader_stage().specialize(spec);
            auto [it, inserted] = pipelines.emplace(hash, xpcr.create_pipeline());
            return it->second;
          }
        }, pcr);
      }

      /// \brief Create or retrieve a pipeline for a given render-pass/mesh
      const vk::pipeline& get_pipeline(vk::render_pass& pass, uint32_t subpass, hydra::mesh& m /*TODO*/,
                                       const vk::specialization& spec = {})
      {
        if (!std::holds_alternative<vk::graphics_pipeline_creator>(pcr))
          check::on_vulkan_error::n_assert(false, "Trying to get a construct graphics pipeline with a PRS that hold something else");

        auto& gpcr = std::get<vk::graphics_pipeline_creator>(pcr);
        const id_t hash = combine(spec.hash(), combine(pass.compute_subpass_hash(subpass), m.compute_vertex_description_hash()));
        if (gpcr.is_dirty())
        {
          // fixme: non-immediate mode
          invalidate_pipelines();
          gpcr.set_dirty(false);
        }
        if (auto it = pipelines.find(hash); it != pipelines.end())
        {
          return it->second;
        }

        gpcr.set_render_pass(pass);
        gpcr.set_subpass_index(subpass);
        gpcr.get_pipeline_shader_stage().specialize(spec);
        m.setup_vertex_description(gpcr);
        auto [it, inserted] = pipelines.emplace(hash, gpcr.create_pipeline());
        gpcr.clear_render_pass();
        return it->second;
      }

      /// \brief Create or retrieve a pipeline for a given render-pass
      const vk::pipeline& get_pipeline(vk::render_pass& pass, uint32_t subpass, const vk::specialization& spec = {})
      {
        if (!std::holds_alternative<vk::graphics_pipeline_creator>(pcr))
          check::on_vulkan_error::n_assert(false, "Trying to get a construct graphics pipeline with a PRS that hold something else");

        auto& gpcr = std::get<vk::graphics_pipeline_creator>(pcr);

        const id_t hash = combine(spec.hash(), pass.compute_subpass_hash(subpass));
        if (gpcr.is_dirty())
        {
          // fixme: non-immediate mode
          invalidate_pipelines();
          gpcr.set_dirty(false);
        }
        if (auto it = pipelines.find(hash); it != pipelines.end())
        {
          return it->second;
        }

        gpcr.set_render_pass(pass);
        gpcr.set_subpass_index(subpass);
        gpcr.get_pipeline_shader_stage().specialize(spec);
        auto [it, inserted] = pipelines.emplace(hash, gpcr.create_pipeline());
        gpcr.clear_render_pass();
        return it->second;
      }

    private:
      vk::device& dev;
      std::variant<std::monostate, vk::graphics_pipeline_creator, vk::compute_pipeline_creator> pcr;

      vk::descriptor_set_layout ds_layout { dev, VkDescriptorSetLayout(nullptr) };
      vk::descriptor_pool ds_pool { dev, VkDescriptorPool(nullptr) };
      vk::pipeline_layout pipeline_layout {dev};

      std::map<id_t, vk::pipeline> pipelines;
  };
}

