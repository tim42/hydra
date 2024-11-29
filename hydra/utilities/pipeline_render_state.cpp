//
// created by : Timothée Feuillet
// date: 2024-3-3
//
//
// Copyright (c) 2024 Timothée Feuillet
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

#include "pipeline_render_state.hpp"

#include <hydra/engine/hydra_context.hpp>
#include <hydra/utilities/shader_gen/descriptor_sets.hpp>

namespace neam::hydra
{
  void pipeline_render_state::invalidate_pipelines()
  {
    std::mtc_map<id_t, vk::pipeline> temp_pipelines;
    vk::pipeline_layout temp_layout { dev, nullptr };
    std::mtc_vector<vk::descriptor_set_layout> temp_ds_layouts;

    {
      std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
      std::swap(pipelines, temp_pipelines);
      std::swap(pipeline_layout, temp_layout);
      std::swap(ds_layouts, temp_ds_layouts);
    }
    hctx.dfe.defer_destruction(std::move(temp_pipelines), std::move(temp_layout), std::move(temp_ds_layouts));
  }

  void pipeline_render_state::build_data_from_reflection_if_needed()
  {
    if (!can_make_valid_pipelines())
      return;
    if (!ds_layouts.empty())
      return;

    std::visit([this](auto& xpcr)
    {
      if constexpr (std::is_same_v<std::monostate&, decltype(xpcr)>) __builtin_unreachable();
      else
      {
        if (xpcr.is_dirty())
        {
          invalidate_pipelines();
          xpcr.set_dirty(false);
        }
        std::lock_guard _l { spinlock_exclusive_adapter::adapt(lock) };
        descriptor_set_map.clear();
        const std::string debug_name = fmt::format("{}", pipeline_id);
        cr::out().debug("building ref data for {}", debug_name);
        vk::pipeline_shader_stage& pss = xpcr.get_pipeline_shader_stage();
        // build ds_layout / ds_pool / pipeline_layout
        std::vector<id_t> set_ids = pss.compute_descriptor_sets();

        std::vector<vk::descriptor_set_layout*> ds_layouts_ptrs;
        ds_layouts_ptrs.reserve(set_ids.size());
        ds_layouts.reserve(set_ids.size());
        for (uint32_t i = 0; i < set_ids.size(); ++i)
        {
          if (set_ids[i] != id_t::none)
          {
            descriptor_set_map.emplace(set_ids[i], i);
            ds_layouts.push_back(shaders::internal::generate_descriptor_set_layout(set_ids[i], dev));
            if (ds_layouts.back()._get_vk_descriptor_set_layout() != nullptr)
              ds_layouts.back()._set_debug_name(fmt::format("{}", string_id::_from_id_t(set_ids[i])));
          }
          else
          {
            ds_layouts.push_back(vk::descriptor_set_layout{dev, nullptr});
          }

          ds_layouts_ptrs.push_back(&ds_layouts.back());
        }
        vk::pipeline_layout temp_layout =
        {
          dev, ds_layouts_ptrs, pss.compute_combined_push_constant_range()
        };
        temp_layout._set_debug_name(debug_name);
        pipeline_layout = std::move(temp_layout);

        // set the pipeline layout in the pcr
        xpcr.set_pipeline_layout(pipeline_layout);
      }
    }, pcr);
  }

  hydra::vk::compute_pipeline_creator& pipeline_render_state::create_simple_compute(hydra_context& context, string_id shader)
  {
    hydra::vk::compute_pipeline_creator& pcr = get_compute_pipeline_creator();
    pcr.get_pipeline_shader_stage().add_shader(context.shmgr.load_shader(shader));
    return pcr;
  }
}

