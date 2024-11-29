//
// created by : Timothée Feuillet
// date: 2024-3-8
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

#include "texture_manager.hpp"

#include "resource_array.tpl.hpp"

#include <hydra/engine/hydra_context.hpp>
#include <hydra/vulkan/image_creators/image_concurrent.hpp>
#include <glm/gtc/integer.hpp>


//
// res.entries: the indexed entries, accessible in shader.
// res.entries[index].gpu_data: the gpu ressource.
//
// While streaming-in a texture mip level (or mip levels), the index can get assigned to another resource.
// This happens when contention is very very high / a high number of resources are loaded and used / backing media is slow.
//
// The streaming process only references the index and gpu_data, as gpu_data is kept alive until the streaming process finishes.
// The flag gpu_data.evicted is set to indicate that there's no need to do further operations, as the texture is waiting to be deallocated.
//
// Operations on res.entries[index] are done only when the resource is the same and the captured gpu_data is not flagged as evicted.
// The last streaming chain is also provided for cancelation purposes.
//
// The main problematic point is ot destroying a resource that has operations in-flight or soon-to-be-in-flight.
//
// TODO: sparse textures.
// With sparse textures, only the mip-chain is/are allocated, and mips in the mip-chain that are
// immediately accessible (in cache or in the index) are automatically streamed-in
// (as the operation become simply a gpu upload with no IO interraction)
// Mip-chains don't count toward the memory pool budget.
//
// When a not-already loaded mip is requested, it first look to see if we can allocate it without going above the pool limit (and if the mip itself can fit in a empty pool, if not, it fails).
// If we can't, it first tries to unload mip levels from unused textures until there's enough space to fit the requested mip (and all previous mips),
// then, if there's still not enough space, unload mips from referenced resources whose mip are not used.
// If there's still not enough space and the pool is set to have a fixed size, the mip fails to be loaded.
//



namespace neam::hydra
{
  void texture_manager::image_gpu_data::immediate_resource_release()
  {
    if (image)
    {
      const uint64_t size = image->allocation.size();
      image->allocation.free();
      if (owner)
      {
        owner->total_memory.fetch_sub(size, std::memory_order_release);
      }
    }
  }

  texture_manager::texture_manager(hydra_context& _hctx)
    : hctx(_hctx)
    , image_data_txctx { hctx, hctx.slow_tqueue }
    , default_sampler { hctx.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, -1, 1 }
    , default_texture { hctx.allocator, hctx.device, vk::image::create_image_arg
      (
        hctx.device,
        vk::image_2d
        (
          {1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        )
      )}
  {
    on_index_loaded_tk = hctx.res.on_index_loaded.add(*this, &texture_manager::force_full_reload);

    txctx.debug_context = "texture_manager::txctx";
    image_data_txctx.debug_context = "texture_manager::image_data_txctx";

    default_sampler._set_debug_name("texture_manager::default-sampler");
    default_texture.image._set_debug_name("texture_manager::default-texture::image");
    default_texture.view._set_debug_name("texture_manager::default-texture::view");
  }

  texture_index_t texture_manager::request_texture_index(string_id texture_rid)
  {
    {
      std::lock_guard _l {spinlock_shared_adapter::adapt(texture_id_map_lock)};
      if (auto it = texture_id_map.find(texture_rid); it != texture_id_map.end())
        return it->second;
    }

    const auto evict = [this](texture_index_t index)
    {
      if (index >= (uint32_t)res.entries.size())
        return;
      if (!res.entries[index].gpu_data)
        return;

      auto& entry = res.entries[index];

      // try to end early the work that may still be in progress
      // NOTE: we cannot simply destroy the image and everything as there might be operations queued but not yet submit
      entry.gpu_data->evicted = true;
      entry.gpu_data->upload_chain.cancel();
      txctx.remove_operations_for(entry.gpu_data->image->image);
      image_data_txctx.remove_operations_for(entry.gpu_data->image->image);

      // TODO: Better deallocation. Aggressive memory re-use.
      entry.gpu_data.release();
    };

    texture_index_t index = k_invalid_texture_index;
    {
      std::lock_guard _l {spinlock_exclusive_adapter::adapt(texture_id_map_lock)};
      // first, check that someone didn't add it from under us:
      if (auto it = texture_id_map.find(texture_rid); it != texture_id_map.end())
        return it->second;
      const size_t old_size = res.entries.size();
      index = res.find_or_create_new_entry(configuration.max_entries, configuration.entries_to_allocate_at_once, configuration.evict_no_question_asked);

      if (index == k_invalid_texture_index)
      {
#if 0
        cr::out().warn("texture-manager: failed to allocate space for `{}`: reached max number of in-use texture ({})", texture_rid, configuration.max_entries);
        cr::out().warn("texture-manager: state: .: free (tagged as), -: unused (in list), !: unused (untagged), X: used");
        std::string state_string;
        std::string lnk_string;
        std::string rlnk_string;
        {
          std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};
          for (auto& it : res.entries)
          {
            if (it.entry_state == utilities::resource_array_entry_state_t::free)
              state_string += ".";
            else if (it.entry_state == utilities::resource_array_entry_state_t::unused)
              state_string += "-";
            else if (it.last_frame_with_usage <= res.frame_counter - 3)
              state_string += "!";
            else
              state_string += "X";
          }
          for (uint32_t i = res.first_unused_entry; i != res.k_invalid_index; i = res.entries[i].next)
            lnk_string += fmt::format("{} ", i);
          for (uint32_t i = res.last_unused_entry; i != res.k_invalid_index; i = res.entries[i].prev)
            rlnk_string += fmt::format("{} ", i);
        }
        cr::out().warn("texture-manager: state: [{}]", state_string);
        cr::out().warn("texture-manager: state: >[ {}]", lnk_string);
        cr::out().warn("texture-manager: state: <[ {}]", rlnk_string);
#endif
        return index;
      }
      if (old_size != res.entries.size())
      {
        cr::out().debug("texture-manager: resized resource array to {} entries", res.entries.size());
      }

      texture_id_map.emplace(texture_rid, index);
      // remove the previous entry, if we were re-using an existing entry
      if (res.entries[index].asset_rid != id_t::none)
        texture_id_map.erase(res.entries[index].asset_rid);
    }

    // reset the entry to a pre-init state:
    {
      std::lock_guard _lg {spinlock_exclusive_adapter::adapt(res.entries[index].lock)};

      evict(index);

      utilities::resource_array_entry_base_t base_save = res.entries[index];
      res.entries[index] = texture_entry { .asset_rid = texture_rid};
      *(utilities::resource_array_entry_base_t*)(&res.entries[index]) = base_save;
      res.entries[index].last_frame_with_usage = res.frame_counter;
    }

    cr::out().debug("texture-manager: loading `{}`...", texture_rid);

    // load the texture data
    load_texture_data_unlocked(index, texture_rid);

    return index;
  }

  void texture_manager::indicate_texture_usage(texture_index_t tid, uint32_t targetted_mip_level)
  {
    std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};
    if (tid == k_invalid_texture_index || tid >= (uint32_t)res.entries.size()) [[unlikely]]
      return;

    res.entries[tid].last_frame_with_usage = res.frame_counter;
    res.entries[tid].requested_mip_level = targetted_mip_level;
  }

  void texture_manager::clear()
  {
    {
      std::lock_guard _l {spinlock_exclusive_adapter::adapt(texture_id_map_lock)};
      texture_id_map.clear();
      image_data_txctx.clear();
      hctx.dfe.defer_destruction(std::move(gpu_state), (res.clear()));
    }

    has_changed.store(true, std::memory_order_release);

    // end by triggering the event (with no lock held)
    on_texture_pool_cleared.call();
  }

  void texture_manager::load_texture_data_unlocked(texture_index_t tid, string_id rid)
  {
    hctx.res.read_resource<assets::image>(rid).then(&hctx.tm, threading::k_non_transient_task_group, [this, tid, rid](assets::image&& img_res, resources::status st)
    {
      TRACY_SCOPED_ZONE;
      if (st == resources::status::failure) [[unlikely]]
      {
        cr::out().error("texture_manager: failed to load texture `{}` (invalid resource or resource type).\n"
                        "                 This will consume a texture slot until the texture is evicted.", rid);
        return;
      }

      {
        // we need the shared lock to prevent anyone from resizing the array from under us
        std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};

        if (res.entries.size() <= tid)
          return;

        has_changed.store(true, std::memory_order_release);

        auto& entry = res.entries[tid];
        if (entry.asset_rid != rid)
        {
          // sanity check, prevent writing over data from a different texture
          // Might happen when loading the texture took too much time (which outside using a super slow network connection should not happen...)
          // cr::out().warn("texture_manager: loaded texture data for `{}`, but slot was assigned to `{}` in between. Skipping texture.", rid, entry.asset_rid);
          return;
        }
        if (img_res.mips.size() == 0)
        {
          cr::out().warn("texture_manager: loaded texture data for `{}`, but texture has no mip level.", rid);
          return;
        }
        if (entry.invalid_resource == false)
        {
          cr::out().error("texture_manager: load_texture_data_unlocked for `{}`: data was already loaded, overwriting it, but this might do bad things", rid);
          std::lock_guard _gl { spinlock_exclusive_adapter::adapt(entry.lock) };
          entry.gpu_data.release();
        }

        // create the image / sampler:
        {
          std::lock_guard _gl { spinlock_exclusive_adapter::adapt(entry.lock) };

          entry.image_information = std::move(img_res);

          entry.gpu_data = make_dfe_refcount_pooled_ptr(hctx.dfe, gpu_data_pool);

          entry.gpu_data->owner = this;
          entry.gpu_data->valid = false;
          // FIXME: This should be driven by the texture resource
          entry.gpu_data->sampler.emplace(vk::sampler{hctx.device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0, -1000, 1000});
          entry.gpu_data->sampler->_set_debug_name(fmt::format("texture-asset:sampler[{}]", rid));

          // Create the image (FIXME: create a sparse resource)
          glm::uvec2 size = glm::max(glm::uvec2(1, 1), entry.image_information.size.xy());
          {
            vk::image image = vk::image::create_image_arg
            (
              hctx.device,
              vk::image_2d
              (
                size, entry.image_information.format, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                (uint32_t)entry.image_information.mips.size(), VK_IMAGE_LAYOUT_UNDEFINED
              ),
              vk::image_concurrent
              {{
                hctx.gqueue, hctx.cqueue, hctx.tqueue,
              }}
            );

            // prevent a spike, check if the min requirements can fit in the budget
            const uint64_t image_req_size = image.get_memory_requirements().size;
            total_memory.fetch_add(image_req_size, std::memory_order_release);

            memory_budget_fit();

            entry.gpu_data->image.emplace(image_holder{hctx.allocator, hctx.device, std::move(image)});

            // add the second part (wasted memory) to the total memory of the pool
            total_memory.fetch_add(entry.gpu_data->image->allocation.size() - image_req_size, std::memory_order_release);

            memory_budget_fit();
          }
          entry.gpu_data->image->image._set_debug_name(fmt::format("texture-asset[{}]", rid));
          entry.gpu_data->image->view._set_debug_name(fmt::format("texture-asset:view[{}]", rid));
          entry.gpu_data->valid = false;
          entry.gpu_data->loaded_mip_mask.store(0, std::memory_order_release);

          txctx.acquire_custom_layout_transition(entry.gpu_data->image->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

          entry.invalid_resource = false; // we are now a valid resource, just with no data
        }

        // setup the immediate data that we can:
        if (false)
        {
          entry.min_immediate_mip_level = k_invalid_mip;
          for (uint32_t i = 0; i < (uint32_t)entry.image_information.mips.size(); ++i)
          {
            if (hctx.res.is_resource_immediatly_available(entry.image_information.mips[i]))
            {
              entry.min_immediate_mip_level = i;
              break;
            }
          }
          // try to stream the immediate mips:
          if (entry.min_immediate_mip_level != k_invalid_mip)
          {
            const uint8_t actual_mip_to_load = entry.requested_mip_level;
            entry.requested_mip_level = entry.min_immediate_mip_level;
            load_mip_data_unlocked(entry, tid); // guaranteed to be immediate
            entry.requested_mip_level = actual_mip_to_load;
            cr::out().debug("texture_manager: finished seting-up texture `{}` (loaded mip {} which is the highest immediate mip)", rid, entry.min_immediate_mip_level);
          }
        }
      }
    });
  }

  void texture_manager::load_mip_data_unlocked(texture_entry& entry, texture_index_t tid)
  {
    TRACY_SCOPED_ZONE_COLOR(0x7FFF00);

    dfe_refcount_pooled_ptr<image_gpu_data> gpu_data;

    uint8_t mip_to_stream;
    uint8_t previous_streamed_mip_level;
    string_id rid;

    {
      std::lock_guard _gl { spinlock_shared_adapter::adapt(entry.lock) };

      if (entry.invalid_resource || !entry.gpu_data)
        return;
      // skip the resource if no mip level have been requested or the requested mip is already loaded
      if (entry.requested_mip_level == k_invalid_mip || entry.requested_mip_level >= entry.streamed_mip_level)
        return;
      gpu_data = entry.gpu_data.duplicate();

      const uint8_t max_mip_level = (uint8_t)entry.image_information.mips.size() - 1;
      mip_to_stream = std::min(max_mip_level, entry.requested_mip_level);

      previous_streamed_mip_level = std::min<uint8_t>(max_mip_level + 1, entry.streamed_mip_level);
      entry.streamed_mip_level = mip_to_stream; // assign now to prevent streaming data in-loop
      rid = entry.asset_rid;
    }

    // We try to load all the missing mips in one shot, as the io-context will perform a single read operation for them
    // (this has the same memory cost, but is more efficient on IO operations)
    // FIXME: We could use the fact that low level mips will be placed in the index, thus immediatly (NOT async) accessible

    // mip to streams, from previous_streamed_mip_level to mip_to_stream
    const uint32_t mip_count = previous_streamed_mip_level - mip_to_stream;

    // update the changed flag:
    has_changed.store(true, std::memory_order_release);

    std::vector<async::continuation_chain> chains;
    chains.reserve(mip_count);
    for (uint32_t i = 0; i < mip_count; ++i)
    {
      chains.push_back
      (
        // stream the mip:
        hctx.res.read_resource<assets::image_mip>(entry.image_information.mips[mip_to_stream + i])
        // copy it to gpu
        .then([i, rid, mip_to_stream, tid, this, gpu_data = gpu_data.duplicate()](assets::image_mip&& mip, resources::status st)
        {
          TRACY_SCOPED_ZONE_COLOR(0x8FFF00);
          // prevent some of the work if there's an early eviction
          if (gpu_data->evicted || async::is_current_chain_canceled())
            return async::continuation_chain::create_and_complete();

          // one error cause can be cancellation/eviction. So before reporting the error, we make sure the chain/gpu-data are still valid
          if (st != resources::status::success)
          {
            cr::out().error("texture_manager: failed to load mip level {} for texture `{}`. Marking the texture as invalid.", mip_to_stream + i, rid);
            return async::continuation_chain::create_and_complete();
          }

          {
            std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};
            if (res.entries.size() <= tid)
              return async::continuation_chain::create_and_complete();
            auto& entry = res.entries[tid];
            std::lock_guard _gl { spinlock_shared_adapter::adapt(entry.lock) };
            if (entry.asset_rid != rid)
              return async::continuation_chain::create_and_complete();
          }

          // for small transfers, we use immediate transfers + the "fast tx queue".
          // this usually means that lower mip-levels have priority and will be availlable immediately
          // TODO: Add a condition to avoid spamming the immediate transfer stuff
          if (mip.texels.size < 128)
          {
            txctx.transfer(gpu_data->image->image, std::move(mip.texels), mip.size, vk::image_subresource_layers{VK_IMAGE_ASPECT_COLOR_BIT, i}, VK_IMAGE_LAYOUT_GENERAL);
            return async::continuation_chain::create_and_complete();
          }
          // for anything bigger, we use an async transfer. This means higher latency for completion (we wait for slow_tqueue to be done)
          // but we don't lock anything related to the current frame.
          return image_data_txctx.async_transfer(gpu_data->image->image, std::move(mip.texels), mip.size, vk::image_subresource_layers{VK_IMAGE_ASPECT_COLOR_BIT, i}, VK_IMAGE_LAYOUT_GENERAL);
        })
        // update the CPU data to reflect that the mip has been copied to gpu:
        .then([i, rid, mip_to_stream, tid, this, mip_count, gpu_data = gpu_data.duplicate()]()
        {
          TRACY_SCOPED_ZONE_COLOR(0x9FFF00);

          // prevent some of the work if there's an early eviction
          if (gpu_data->evicted || async::is_current_chain_canceled())
            return;

          {
            std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};
            if (res.entries.size() <= tid)
              return;
            auto& entry = res.entries[tid];
            {
              std::lock_guard _gl { spinlock_shared_adapter::adapt(entry.lock) };
              if (entry.asset_rid != rid)
                return;
            }
          }
          const uint32_t bit_index = (mip_count - (mip_to_stream + i) - 1);
          uint64_t loaded_mips = gpu_data->loaded_mip_mask.fetch_or(uint64_t(1) << bit_index, std::memory_order_acq_rel) | (uint64_t(1) << bit_index);

          const uint32_t ffs = __builtin_ffsl(~loaded_mips) - 1;
          // cr::out().debug("texture-manager: loaded mip level {} of `{}` (ffs: {} | {:X}, {})", mip_to_stream + i, rid, ffs, loaded_mips, mip_count);

          // we have a new mip, and our mip generated a continuous mip chain
          if ((ffs > 0) && (ffs >= mip_to_stream))
          {
            std::lock_guard _gl { spinlock_exclusive_adapter::adapt(gpu_data->view_lock) };

            loaded_mips = gpu_data->loaded_mip_mask.load(std::memory_order_acquire);
            const uint32_t new_ffs = __builtin_ffsl(~loaded_mips) - 1;
            // check that no-one pre-empted us:
            if (new_ffs == ffs && !gpu_data->evicted)
            {
              // cr::out().debug("texture-manager: `{}`: created new gpu-data ({} mips)", rid, loaded_mips);

              hctx.dfe.defer_destruction(std::move(gpu_data->image->view));

              gpu_data->image->view = vk::image_view
              (
                hctx.device, gpu_data->image->image, VK_IMAGE_VIEW_TYPE_MAX_ENUM, VK_FORMAT_MAX_ENUM, vk::rgba_swizzle(),
                vk::image_subresource_range
                {
                  VK_IMAGE_ASPECT_COLOR_BIT, glm::uvec2{mip_count - ffs, ffs}
                }
              );
              gpu_data->image->view._set_debug_name(fmt::format("texture-asset:view[{}]<{}, {}>", rid, (float)(mip_count - ffs), (float)mip_count));

              // we have some valid data:
              gpu_data->valid = true;
              // update the changed flag:
              has_changed.store(true, std::memory_order_release);
            }
          }
          return;
        })
      );
    }

    gpu_data->upload_chain = async::multi_chain(std::move(chains))
    .then([rid, mip_to_stream, tid, this, mip_count, gpu_data = gpu_data.duplicate()]
    {
      TRACY_SCOPED_ZONE_COLOR(0xAFFF00);

      // prevent some of the work if there's an early eviction
      if (gpu_data->evicted || async::is_current_chain_canceled())
        return;

      uint8_t last_streamed_mip_level;
      {
        std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};
        if (res.entries.size() <= tid)
          return;
        auto& entry = res.entries[tid];
        std::lock_guard _gl { spinlock_shared_adapter::adapt(entry.lock) };
        if (entry.asset_rid != rid)
          return;
        last_streamed_mip_level = entry.streamed_mip_level;
      }

      if (last_streamed_mip_level == mip_to_stream)
      {
        txctx.release_custom_layout_transition(gpu_data->image->image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // cr::out().debug("texture-manager: fully loaded up-to mip level {} of `{}`", mip_to_stream, rid);

        gpu_data->upload_chain.reset();
      }
    });
  }

  void texture_manager::force_full_reload()
  {
    cr::out().warn("texture-manager: reloading all textures from disk");
    {
      std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};

      for (uint32_t i = 0; i < (uint32_t)res.entries.size(); ++i)
      {
        auto& it = res.entries[i];
        if (it.asset_rid != id_t::none)
        {
          it.streamed_mip_level = k_invalid_mip;
          load_texture_data_unlocked(i, it.asset_rid);
        }
      }
    }
    hctx.hconf.read_or_create_conf(configuration);
  }

  assets::image texture_manager::get_image_asset(texture_index_t index) const
  {
    std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};
    if (index >= (uint32_t)res.entries.size())
      return { .format = VK_FORMAT_UNDEFINED };
    std::lock_guard _gl { spinlock_shared_adapter::adapt(res.entries[index].lock) };
    if (res.entries[index].invalid_resource)
      return { .format = VK_FORMAT_UNDEFINED };
    return res.entries[index].image_information;
  }

  VkImageView texture_manager::_get_vk_image_view_for(texture_index_t index) const
  {
    std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};
    if (index >= (uint32_t)res.entries.size())
      return nullptr;
    std::lock_guard _gl { spinlock_shared_adapter::adapt(res.entries[index].lock) };
    if (!res.entries[index].gpu_data->image)
      return nullptr;
    return res.entries[index].gpu_data->image->view.get_vk_image_view();
  }

  void texture_manager::begin_engine_shutdown()
  {
    cr::out().debug("texture-manager: clearing for engine shutdown");
    clear();
  }

  void texture_manager::process_start_of_frame(vk::submit_info& si)
  {
    TRACY_SCOPED_ZONE;
    if (first_init)
    {
      first_init = false;
      txctx.acquire(default_texture.image, VK_IMAGE_LAYOUT_UNDEFINED);
      txctx.transfer(default_texture.image, raw_data::duplicate(uint32_t(0)));
      txctx.release(default_texture.image, hctx.gqueue, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    res.start_frame([this](auto& it, uint32_t i)
    {
      load_mip_data_unlocked(it, i);
    });

    // softly try to resize the resource array down:
    if (res.entries.size() > 0 && false)
    {
      std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};

      if (res.entries.back().entry_state == utilities::resource_array_entry_state_t::free
          || (res.entries.back().entry_state == utilities::resource_array_entry_state_t::unused
              && res.frame_counter - res.entries.back().last_frame_with_usage >= configuration.evict_no_question_asked))
      {
        uint32_t original_size = res.entries.size();
        {
          std::lock_guard _lx {spinlock_shared_to_exclusive_adapter::adapt(res.entries_lock)};
          std::lock_guard _lh {res.list_header_lock};
          std::lock_guard _lml {spinlock_exclusive_adapter::adapt(texture_id_map_lock)};
          while (res.entries.back().entry_state == utilities::resource_array_entry_state_t::free ||
            ((res.entries.back().entry_state == utilities::resource_array_entry_state_t::unused
                && res.frame_counter - res.entries.back().last_frame_with_usage >= configuration.evict_no_question_asked)))
          {
            if (res.entries.back().asset_rid != id_t::none)
            {
              texture_id_map.erase(res.entries.back().asset_rid);
            }
            if (res.entries.back().entry_state == utilities::resource_array_entry_state_t::unused)
            {
              res.remove_entry_from_unused_list_unlocked(res.entries.back());
            }
            res.entries.pop_back();

            if (res.entries.empty())
              break;
          }
          res.first_free_entry = res.k_invalid_index;
        }

        if (res.entries.size() < original_size)
        {
          cr::out().debug("texture_manager: resizing resource array from {} to {}", original_size, res.entries.size());
          has_changed.store(true, std::memory_order_relaxed);
        }
      }
    }

    if (has_changed.exchange(false, std::memory_order_acq_rel))
    {
      raw_data indirection_raw_data;
      {
        std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};

        // resize the indirection buffer if necessary:
        if (!gpu_state.indirection_buffer || gpu_state.indirection_buffer->buffer.size() / sizeof(uint32_t) - 1 < res.entries.size())
        {
          hctx.dfe.defer_destruction(std::move(gpu_state.indirection_buffer));

          gpu_state.indirection_buffer.emplace
          (
            hctx.allocator,
            vk::buffer
            (
              hctx.device,
              (res.entries.size() + 1) * sizeof(uint32_t),
              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            ),
            hydra::allocation_type::persistent
          );
        }

        // upload the new data to the indirection buffer + build the descriptor set if necessary:
        // FIXME: If necessary!
        // FIXME: Move this first in the function to avoid waiting on the transfer?
        {
          indirection_raw_data = raw_data::allocate(sizeof(uint32_t) * (res.entries.size() + 1));
          auto* data = indirection_raw_data.get_as<shader_structs::texture_indirection_t>();
          data->texture_count = (uint32_t)res.entries.size();

          hctx.dfe.defer_destruction(gpu_state.descriptor_set.reset());
          gpu_state.descriptor_set.texture_manager_indirection = gpu_state.indirection_buffer->buffer;
          gpu_state.descriptor_set.texture_manager_texture_float_1d.resize(res.entries.size() + 1);

          for (uint32_t i = 0; i < (uint32_t)res.entries.size(); ++i)
          {
            auto& it = res.entries[i];

            std::lock_guard _gl { spinlock_shared_adapter::adapt(it.lock) };
            data->indirection[i] = it.invalid_resource ? 0 : (i + 1);


            if (it.entry_state != utilities::resource_array_entry_state_t::free && it.gpu_data && it.gpu_data->valid && !it.invalid_resource && it.gpu_data->image)
            {
              const uint64_t loaded_mips = it.gpu_data->loaded_mip_mask.load(std::memory_order_acquire);

              std::lock_guard _gl { spinlock_shared_adapter::adapt(it.gpu_data->view_lock) };
              if ((loaded_mips & 1) != 0 || !it.gpu_data->image->view.get_vk_image_view())
                gpu_state.descriptor_set.texture_manager_texture_float_1d[i] = {it.gpu_data->image->view, *it.gpu_data->sampler};
              else
                gpu_state.descriptor_set.texture_manager_texture_float_1d[i] = {default_texture.view, default_sampler};
            }
            else
            {
              gpu_state.descriptor_set.texture_manager_texture_float_1d[i] = {default_texture.view, default_sampler};
            }
          }
        }
      }

      // operations to be done without any lock held:
      txctx.acquire(gpu_state.indirection_buffer->buffer, hctx.gqueue);
      txctx.transfer(gpu_state.indirection_buffer->buffer, std::move(indirection_raw_data));
      txctx.release(gpu_state.indirection_buffer->buffer, hctx.gqueue);

      gpu_state.descriptor_set.texture_manager_texture_float_1d.back() = {default_texture.view, default_sampler};

      // update the descriptor-set:
      gpu_state.descriptor_set.update_descriptor_set(hctx);
    }

    // upload data to textures:
    txctx.build(si);
    {
      // necessary, as tqueue has the layout transitions necessary for copies
      vk::semaphore sem { hctx.device };
      si.on(hctx.tqueue).signal(sem);
      si.sync();
      si.on(hctx.slow_tqueue).wait(sem, VK_PIPELINE_STAGE_TRANSFER_BIT/*VK_PIPELINE_STAGE_ALL_COMMANDS_BIT*/ /*VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT*/);
      hctx.dfe.defer_destruction(std::move(sem));
    }
    image_data_txctx.build(si);
  }

  void texture_manager::memory_budget_fit(bool aggressive)
  {
    // check if there's anything to do:
    if (!aggressive && get_total_gpu_memory() < configuration.max_pool_memory)
      return;

    std::lock_guard _l {spinlock_shared_adapter::adapt(res.entries_lock)};
    std::lock_guard _lh(res.list_header_lock);

    // TODO: per-mip memory residency to save a bit of memory

    // iterate over the unused resources, and evict those that are old until we fit in budget
    bool done = false;
    res.for_each_unused_entries_unlocked([this, aggressive, &done](auto& entry, uint32_t index)
    {
      if (done || !entry.gpu_data || entry.invalid_resource)
        return;
      if (get_total_gpu_memory() < configuration.max_pool_memory)
      {
        done = true;
        return;
      }

      // try to end early the work that may still be in progress
      // NOTE: we cannot simply destroy the image and everything as there might be operations queued but not yet submit
      entry.gpu_data->evicted = true;
      entry.gpu_data->upload_chain.cancel();
      txctx.remove_operations_for(entry.gpu_data->image->image);
      image_data_txctx.remove_operations_for(entry.gpu_data->image->image);
      entry.gpu_data.release();

      // remove entry from the free list and add it to the free-list
      res.remove_entry_from_unused_list_unlocked(entry);
      res.add_entry_to_free_list_unlocked(entry, index);
    });
  }
}

