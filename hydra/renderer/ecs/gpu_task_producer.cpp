//
// created by : Timothée Feuillet
// date: 2/9/25
//
//
// Copyright (c) 2025 Timothée Feuillet
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

#include "gpu_task_producer.hpp"
#include "gpu_tasks_order.hpp"

namespace neam::hydra::renderer::concepts
{
  void gpu_task_producer::concept_logic::export_resource(id_t id, exported_image image, export_mode mode)
  {
    get_concept().export_resource(id, image, mode);
  }

  void gpu_task_producer::concept_logic::export_resource(id_t id, vk::buffer& buffer, export_mode mode)
  {
    get_concept().export_resource(id, buffer, mode);
  }

  bool gpu_task_producer::concept_logic::can_import(id_t id) const
  {
    return get_concept().can_import(id);
  }

  uint32_t gpu_task_producer::concept_logic::get_importable_version(id_t id) const
  {
    return get_concept().get_importable_version(id);
  }

  exported_image gpu_task_producer::concept_logic::import_image(id_t id, uint32_t version, VkImageLayout final_layout, VkAccessFlags final_access, VkPipelineStageFlags final_stage) const
  {
    return get_concept().import_image(id, version, final_layout, final_access, final_stage);
  }

  vk::buffer& gpu_task_producer::concept_logic::import_buffer(id_t id, uint32_t version) const
  {
    return get_concept().import_buffer(id, version);
  }

  bool gpu_task_producer::concept_logic::has_viewport_context() const
  {
    return get_concept().has_viewport_context();
  }

  viewport_context& gpu_task_producer::concept_logic::get_viewport_context() const
  {
    return get_concept().get_viewport_context();
  }

  void gpu_task_producer::concept_logic::set_viewport_context(const viewport_context& vpc)
  {
    return get_concept().set_viewport_context(vpc);
  }

  void gpu_task_producer::concept_logic::begin_rendering(vk::command_buffer_recorder& cbr, const exported_image& img, VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op)
  {
    begin_rendering(cbr, std::vector{img}, load_op, store_op);
  }

  void gpu_task_producer::concept_logic::begin_rendering(vk::command_buffer_recorder& cbr, const std::vector<exported_image>& imgs, VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op)
  {
    std::vector<vk::rendering_attachment_info> rai;
    rai.reserve(imgs.size());
    for (auto& it : imgs)
    {
      rai.push_back
      ({
          it.view, it.layout,
          load_op, store_op
      });
    }
    cbr.begin_rendering
    ({
      get_viewport_context().viewport_rect, rai,
    });
  }

  void gpu_task_producer::concept_logic::pipeline_barrier(vk::command_buffer_recorder& cbr, exported_image& img, VkImageLayout new_layout, VkAccessFlags dst_access, VkPipelineStageFlags dst_stage)
  {
    vk::image_memory_barrier imb
    {
      img.image, img.layout, new_layout,
      img.access, dst_access
    };
    cbr.pipeline_barrier(img.stage, dst_stage, 0, imb);
    img.layout = new_layout;
    img.stage = dst_stage;
    img.access = dst_access;
  }

  void gpu_task_producer::concept_logic::pipeline_barrier(vk::command_buffer_recorder& cbr, exported_image& img, VkAccessFlags dst_access, VkPipelineStageFlags dst_stage)
  {
    vk::image_memory_barrier imb
    {
      img.image, img.layout, img.layout,
      img.access, dst_access
    };
    cbr.pipeline_barrier(img.stage, dst_stage, 0, imb);
    img.stage = dst_stage;
    img.access = dst_access;
  }

  // void gpu_task_producer::concept_logic::pipeline_barrier(vk::command_buffer_recorder& cbr, const std::vector<exported_image>& imgs, VkAccessFlags dst_access, VkPipelineStageFlags dst_stage)
  // {
  //   std::vector<vk::image_memory_barrier> imb;
  //   for (auto& it : imgs)
  //   {
  //     imb.push_back(vk::image_memory_barrier
  //     {
  //       it.image, it.layout, it.layout,
  //       it.access, dst_access
  //     });
  //   }
  //   cbr.pipeline_barrier(src_stage, dst_stage, 0, imb);
  // }

  template<typename T>
  void gpu_task_producer::export_resource_tpl(id_t id, T&& res, export_mode mode)
  {
    if (mode == export_mode::constant)
    {
      check::debug::n_check(!can_import(id), "gpu_task_producer::export_resource({}): resource already exist with the same identifier while trying to export a constant", id);

      exported_resources.emplace(std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(mode, res, 0));
    }
    else if (mode == export_mode::versioned)
    {
      uint32_t version = 0;
      [[maybe_unused]] uint32_t res_index = ~0u;
      if (auto it = exported_resources.find(id); it != exported_resources.end())
      {
        exported_resource_data erd = it->second;
        check::debug::n_check(erd.mode != export_mode::constant, "gpu_task_producer::export_resource({}): trying to export over a constant with the same identifier", id);

        version = erd.version + 1;
        res_index = erd.resource.index();
        const id_t spec_id = specialize(id, fmt::format("{}", erd.version));

        // if we export over a previous version, we have to move the latest entry to the specialized slot
        exported_resources.erase(it);
        exported_resources.emplace(std::piecewise_construct,
          std::forward_as_tuple(spec_id),
          std::forward_as_tuple(erd));
      }

      [[maybe_unused]] auto [it, _cr] = exported_resources.emplace(std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(mode, res, version));
      check::debug::n_check(it->second.resource.index() == res_index || version == 0, "gpu_task_producer::export_resource({}): trying to export over a resource with a different type", id);
    }
  }

  void gpu_task_producer::export_resource(id_t id, exported_image res, export_mode mode)
  {
    export_resource_tpl(id, res, mode);
  }

  void gpu_task_producer::export_resource(id_t id, vk::buffer& res, export_mode mode)
  {
    export_resource_tpl(id, exported_buffer{ res }, mode);
  }

  bool gpu_task_producer::can_import(id_t id) const
  {
    return exported_resources.contains(id);
  }

  uint32_t gpu_task_producer::get_importable_version(id_t id) const
  {
    if (auto it = exported_resources.find(id); it != exported_resources.end())
    {
      return it->second.version;
    }
    return ~0u;
  }

  const exported_resource_data& gpu_task_producer::import_resource(id_t id, uint32_t version) const
  {
    // check if we have a non-versioned resource:
    if (auto it = exported_resources.find(id); it != exported_resources.end())
    {
      const exported_resource_data& ret = it->second;
      check::debug::n_check(ret.mode == export_mode::versioned || version == 0 || version == ~0u, "gpu_task_producer::import_resource({}): requested version {} of an unversioned entry", id, version);
      check::debug::n_check(ret.mode != export_mode::versioned || version <= ret.version || version == ~0u, "gpu_task_producer::import_resource({}): requested version {} of an versioned entry with a current version of {}", id, version, ret.version);

      // constant resources only need one lookup
      if (ret.mode == export_mode::constant)
        return ret;

      // latest version only need one lookup
      if (ret.version <= version)
        return ret;
    }
    else
    {
      check::debug::n_assert(false, "gpu_task_producer::import_resource({}): resource does not exist", id);
      __builtin_unreachable();
    }

    // we need two lookups for specific versions
    const id_t spec_id = specialize(id, fmt::format("{}", version));
    if (auto it = exported_resources.find(id); it != exported_resources.end())
    {
      return it->second;
    }

    check::debug::n_assert(false, "gpu_task_producer::import_resource({}): specialized resource (original resource: {}, requested version: {}) does not exist", spec_id, id, version);
    __builtin_unreachable();
  }

  exported_image gpu_task_producer::import_image(id_t id, uint32_t version, VkImageLayout final_layout, VkAccessFlags final_access, VkPipelineStageFlags final_stage) const
  {
    const exported_resource_data& erd = import_resource(id, version);
    const exported_image* cptr = std::get_if<exported_image>(&erd.resource);
    check::debug::n_assert(cptr != nullptr, "gpu_task_producer::import_image({}): entry does not hold an image", id, version);
    exported_image& ref = *const_cast<exported_image*>(cptr);
    exported_image ret = ref;
    ref.layout = final_layout;
    ref.access = final_access;
    ref.stage = final_stage;
    return ret;
  }

  vk::buffer& gpu_task_producer::import_buffer(id_t id, uint32_t version) const
  {
    const exported_resource_data& erd = import_resource(id, version);
    const exported_buffer* ret = std::get_if<exported_buffer>(&erd.resource);
    check::debug::n_assert(ret != nullptr, "gpu_task_producer::import_buffer({}): entry does not hold a buffer", id, version);
    return const_cast<vk::buffer&>(ret->buffer);
  }

  bool gpu_task_producer::has_viewport_context() const
  {
    return vpc.has_value();
  }

  viewport_context& gpu_task_producer::get_viewport_context() const
  {
    check::debug::n_assert(has_viewport_context(), "gpu_task_producer::get_viewport_context(): trying to get the viewport context when none were set");
    return const_cast<viewport_context&>(*vpc);
  }

  void gpu_task_producer::set_viewport_context(const viewport_context& _vpc)
  {
    check::debug::n_assert(!has_viewport_context(), "gpu_task_producer::set_viewport_context(): trying to set the viewport context when one was already set");

    vpc.emplace(_vpc);
  }

  template<typename UnaryFunction>
  void gpu_task_producer::sorted_for_each_entries(UnaryFunction&& func)
  {
    std::vector<concept_logic*> prologues;
    std::vector<concept_logic*> entries;
    std::vector<concept_logic*> epilogues;
    for_each_concept_provider([&, this](concept_logic& it)
    {
      switch (it.get_order_mode())
      {
        case order_mode::forced_prologue: prologues.push_back(&it);
          break;
        case order_mode::standard: entries.push_back(&it);
          break;
        case order_mode::forced_epilogue: epilogues.push_back(&it);
          break;
      }
    });
    for (auto* it : prologues)
      func(*it);
    for (auto* it : entries)
      func(*it);
    for (auto* it : epilogues)
      func(*it);
  }

  void gpu_task_producer::setup(gpu_task_context& gtctx)
  {
    sorted_for_each_entries([this, &gtctx](concept_logic& it)
    {
      if (it.need_setup)
      {
        it.need_setup = false;
        it.do_setup(gtctx);
      }
    });
  }

  void gpu_task_producer::prepare(gpu_task_context& gtctx)
  {
    sorted_for_each_entries([this, &gtctx](concept_logic& it)
    {
      if (!it.is_concept_provider_enabled())
        return;
      it.do_prepare(gtctx);
    });
  }

  void gpu_task_producer::cleanup()
  {
    // FIXME: could run in tasks! (but should it?)
    sorted_for_each_entries([this](concept_logic& it)
    {
      if (!it.is_concept_provider_enabled())
        return;
      it.do_cleanup();
    });
  }

  void gpu_task_producer::update_from_hierarchy()
  {
    ecs::universe* uni = get_universe();
    auto* task_order = uni->get_universe_root().get<internals::gpu_task_order>();
    check::debug::n_assert(task_order != nullptr, "invalid universe setup for renderer");

    // set back the dirty flag
    set_dirty();

    // setup and prepare must be done during the update, synchronously as any following/child operation might depend on what was
    setup(gtc);

    bool skip_render = false;
    bool has_any_enabled = false;
    sorted_for_each_entries([this, &skip_render, &has_any_enabled](concept_logic& it)
    {
      it.update_enabled_flags();
      skip_render = skip_render || it.concept_provider_requires_skip();
      has_any_enabled = has_any_enabled || it.is_concept_provider_enabled();
    });
    if (skip_render || !has_any_enabled)
      return;

    auto* opt_name = get_unsafe<ecs::name_component>();

    // set/update the name of the debug context
    gtc.transfers.debug_context = opt_name ? opt_name->data : "<gpu_task_producer>";

    // prepare our place in the final submission array
    async::chain<std::vector<vk::submit_info>>::state si_state;
    task_order->push_pass_data(si_state.create_chain());

    {
      // FIXME: Do we need a scoped hierarchy instead?
      allocator::scope gpu_alloc_scope = hctx.allocator.push_scope();
      prepare(gtc);
    }

    // the hierarchical update use the single threaded version
    // (we could probably do with the multi-threaded version but that would complexify a bit the things)
    // so to avoid taking too much time on a single thread, we dispatch a task (that will then dispatch more tasks)
    hctx.tm.get_task([this, si_state = std::move(si_state)] mutable
    {
      TRACY_SCOPED_ZONE;

      bool is_single_threaded = false;
      if (is_single_threaded)
      {
        // single-threaded version
        std::vector<vk::submit_info> vsi;
        vsi.emplace_back(hctx);
        vk::submit_info& si = vsi.back();

        gtc.transfers.build(si);
        sorted_for_each_entries([this, &si](concept_logic& it)
        {
          if (!it.is_concept_provider_enabled())
            return;

          TRACY_SCOPED_ZONE;
          it.do_submit(gtc, si);
        });

        si_state.complete(std::move(vsi));

        cleanup();
        exported_resources.clear();
        vpc.reset();
      }
      else
      {
        std::vector<async::chain<vk::submit_info>> chains;
        // build the cpu->gpu transfers
        chains.reserve(1 + get_concept_providers_count());
        chains.emplace_back();
        hctx.tm.get_task([this, tx_si_state = chains.back().create_state()] mutable
        {
          TRACY_SCOPED_ZONE;
          vk::submit_info si(hctx);
          gtc.transfers.build(si);
          tx_si_state.complete(std::move(si));
        });

        // parallel dispatch the submit calls
        sorted_for_each_entries([this, &chains](concept_logic& it)
        {
          if (!it.is_concept_provider_enabled())
            return;
          chains.emplace_back();
          hctx.tm.get_task([this, state = chains.back().create_state(), &it] mutable
          {
            TRACY_SCOPED_ZONE;
            vk::submit_info si(hctx);
            it.do_submit(gtc, si);
            state.complete(std::move(si));
          });
        });

        // once everything is done, complete the state
        async::multi_chain<std::vector>(std::move(chains))
        .then([this, si_state = std::move(si_state)](std::vector<vk::submit_info>&& vsi) mutable
        {
          // complete the state with the submit info list
          si_state.complete(std::move(vsi));

          // std::vector<vk::submit_info> merged_vsi;
          // merged_vsi.emplace_back(hctx);
          // for (auto& it : vsi)
          //   merged_vsi.back().append(std::move(it));
          //
          // // complete the state with the submit info list
          // si_state.complete(std::move(merged_vsi));

          // all the entries are completed, call cleanup
          // (this allows to move stuff to the DFE)
          cleanup();
          exported_resources.clear();
          vpc.reset();
        });
      }
    });
  }

  gpu_task_producer::concept_logic* gpu_task_producer::get_gpu_task_producer(enfield::type_t type) const
  {
    {
      // TODO: Faster search (keep a bitmask of providers?)
      for (uint32_t i = 0; i < get_concept_providers_count(); ++i)
      {
        if (get_concept_provider(i).get_base().object_type_id == type)
          return const_cast<concept_logic*>(&get_concept_provider(i));
      }
    }

    // bubble-up the search (NOTE: the get_parent call skips any entries with no gpu_task_producer)
    // NOTE: we want tail-recursion here
    if (const gpu_task_producer* const parent = hierarchical::get_parent(); parent != nullptr)
      return parent->get_gpu_task_producer(type);

    // not found:
    return nullptr;
  }
}
