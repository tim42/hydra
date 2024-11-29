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
#include <hydra/vulkan/pipeline_rendering_create_info.hpp>
#include <hydra/geometry/mesh.hpp>

#include <ntools/mt_check/vector.hpp>
#include <ntools/mt_check/unordered_map.hpp>
#include <ntools/mt_check/map.hpp>

namespace neam::hydra
{
  /// \brief {pipeline, render-pass}* and a ds-pool, combined
  class pipeline_render_state
  {
    public:
      pipeline_render_state(vk::device& _dev, hydra_context& _hctx) : dev(_dev), hctx(_hctx) {}

      vk::pipeline_layout& get_pipeline_layout()
      {
        return pipeline_layout;
      }
      const vk::pipeline_layout& get_pipeline_layout() const
      {
        return pipeline_layout;
      }

      /// \brief invalidate the existing pipelines using the vrd
      /// \note This is the better way to invalidate stuff
      void invalidate_pipelines();

      /// \brief Return the pipeline creator
      /// If you modify it, please call invalidate_pipelines() to force a reload of the pipelines
      vk::graphics_pipeline_creator& get_graphics_pipeline_creator()
      {
        if (std::holds_alternative<std::monostate>(pcr))
        {
          std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
          auto& gpcr = pcr.emplace<vk::graphics_pipeline_creator>(dev);
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
          std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
          auto& cpcr = pcr.emplace<vk::compute_pipeline_creator>(dev);
          return cpcr;
        }
        if (std::holds_alternative<vk::compute_pipeline_creator>(pcr))
          return std::get<vk::compute_pipeline_creator>(pcr);

        check::on_vulkan_error::n_assert(false, "Trying to get a graphics pipeline on a PRS that hold something else");
        __builtin_unreachable();
      }

      hydra::vk::compute_pipeline_creator& create_simple_compute(hydra_context& context, string_id shader);

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

        build_data_from_reflection_if_needed();

        return std::visit([this, hash, &spec](auto & xpcr) -> const vk::pipeline&
        {
          if constexpr (std::is_same_v<std::monostate&, decltype(xpcr)>) __builtin_unreachable();
          else
          {
            log_pipeline_compilation(hash);

            std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
            xpcr.get_pipeline_shader_stage().specialize(spec);
            auto [it, inserted] = pipelines.emplace(hash, xpcr.create_pipeline());
            it->second._set_debug_name(fmt::format("{} spec. hash: {} ", pipeline_id, hash));
            it->second._set_cpp_struct_to_set(descriptor_set_map);
            it->second._set_pipeline_id(pipeline_id);
            return it->second;
          }
        }, pcr);
      }

      /// \brief Create or retrieve a pipeline for a given pipeline_rendering_create_info
      const vk::pipeline& get_pipeline(const vk::pipeline_rendering_create_info& prci, const vk::specialization& spec = {})
      {
        if (!std::holds_alternative<vk::graphics_pipeline_creator>(pcr))
          check::on_vulkan_error::n_assert(false, "Trying to get a construct graphics pipeline with a PRS that hold something else");

        auto& gpcr = std::get<vk::graphics_pipeline_creator>(pcr);

        const id_t hash = combine(spec.hash(), prci.compute_hash());
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

        build_data_from_reflection_if_needed();

        log_pipeline_compilation(hash);

        std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
        gpcr.set_pipeline_create_info(prci);
        gpcr.get_pipeline_shader_stage().specialize(spec);
        auto [it, inserted] = pipelines.emplace(hash, gpcr.create_pipeline());
        it->second._set_debug_name(fmt::format("{} spec. hash: {} ", pipeline_id, hash));
        it->second._set_cpp_struct_to_set(descriptor_set_map);
        it->second._set_pipeline_id(pipeline_id);
        return it->second;
      }

      /// \brief Create or retrieve a pipeline for a given create info/mesh
      const vk::pipeline& get_pipeline(const vk::pipeline_rendering_create_info& prci, hydra::mesh& m /*TODO*/,
                                       const vk::specialization& spec = {})
      {
        if (!std::holds_alternative<vk::graphics_pipeline_creator>(pcr))
          check::on_vulkan_error::n_assert(false, "Trying to get a construct graphics pipeline with a PRS that hold something else");

        auto& gpcr = std::get<vk::graphics_pipeline_creator>(pcr);
        const id_t hash = combine(spec.hash(), combine(prci.compute_hash(), m.compute_vertex_description_hash()));
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

        build_data_from_reflection_if_needed();

        log_pipeline_compilation(hash);

        std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
        gpcr.set_pipeline_create_info(prci);
        gpcr.get_pipeline_shader_stage().specialize(spec);
        m.setup_vertex_description(gpcr);
        auto [it, inserted] = pipelines.emplace(hash, gpcr.create_pipeline());
        it->second._set_debug_name(fmt::format("{} spec. hash: {} ", pipeline_id, hash));
        it->second._set_cpp_struct_to_set(descriptor_set_map);
        it->second._set_pipeline_id(pipeline_id);
        return it->second;
      }

      /// \brief Create or retrieve a pipeline for a given render-pass/mesh
      const vk::pipeline& get_pipeline(const vk::render_pass& pass, uint32_t subpass, hydra::mesh& m /*TODO*/,
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

        build_data_from_reflection_if_needed();

        log_pipeline_compilation(hash);

        std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
        gpcr.set_render_pass(pass);
        gpcr.set_subpass_index(subpass);
        gpcr.get_pipeline_shader_stage().specialize(spec);
        m.setup_vertex_description(gpcr);
        auto [it, inserted] = pipelines.emplace(hash, gpcr.create_pipeline());
        gpcr.clear_render_pass();
        it->second._set_debug_name(fmt::format("{} spec. hash: {} ", pipeline_id, hash));
        it->second._set_cpp_struct_to_set(descriptor_set_map);
        it->second._set_pipeline_id(pipeline_id);
        return it->second;
      }

      /// \brief Create or retrieve a pipeline for a given render-pass
      const vk::pipeline& get_pipeline(const vk::render_pass& pass, uint32_t subpass, const vk::specialization& spec = {})
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

        build_data_from_reflection_if_needed();

        log_pipeline_compilation(hash);

        std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
        gpcr.set_render_pass(pass);
        gpcr.set_subpass_index(subpass);
        gpcr.get_pipeline_shader_stage().specialize(spec);
        auto [it, inserted] = pipelines.emplace(hash, gpcr.create_pipeline());
        gpcr.clear_render_pass();
        it->second._set_debug_name(fmt::format("{} spec. hash: {} ", pipeline_id, hash));
        it->second._set_cpp_struct_to_set(descriptor_set_map);
        it->second._set_pipeline_id(pipeline_id);
        return it->second;
      }

      /// \brief Returns false is no valid pipeline can be made at this time
      /// \note Will return false during shader loading, and if valid, will return true
      bool can_make_valid_pipelines() const
      {
        return std::visit([](const auto& xpcr) -> bool
        {
          if constexpr (std::is_same_v<const std::monostate&, decltype(xpcr)>) __builtin_unreachable();
          else
            return xpcr.is_pss_valid() && !xpcr.has_async_operations_in_process();
        }, pcr);
      }

      VkPipelineBindPoint get_pipeline_bind_point() const
      {
        if (std::holds_alternative<vk::graphics_pipeline_creator>(pcr))
          return VK_PIPELINE_BIND_POINT_GRAPHICS;
        else if (std::holds_alternative<vk::compute_pipeline_creator>(pcr))
          return VK_PIPELINE_BIND_POINT_COMPUTE;
        return VK_PIPELINE_BIND_POINT_MAX_ENUM;
      }

    private:
      void build_data_from_reflection_if_needed();

    private:
      void log_pipeline_compilation(id_t hash) const
      {
        cr::out().debug("pipeline {}: compiling variation with hash: {}", pipeline_id, hash);
      }

    private:
      vk::device& dev;
      hydra_context& hctx;
      mutable shared_spinlock lock;
      std::variant<std::monostate, vk::graphics_pipeline_creator, vk::compute_pipeline_creator> pcr;

      std::mtc_vector<vk::descriptor_set_layout> ds_layouts;
      vk::pipeline_layout pipeline_layout {dev, nullptr};
      std::mtc_unordered_map<id_t, uint32_t> descriptor_set_map;

      // specializations:
      std::mtc_map<id_t, vk::pipeline> pipelines;

      std::mtc_map<id_t, vk::descriptor_set_entries> id_to_descriptor_set_entry;
      std::mtc_map<id_t, vk::push_constant_entry> id_to_push_constant_entry;

      string_id pipeline_id = {};

      friend class pipeline_manager;
  };
}

