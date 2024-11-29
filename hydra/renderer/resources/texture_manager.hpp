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

#pragma once

#include <ntools/spinlock.hpp>
#include <ntools/id/string_id.hpp>
#include <ntools/event.hpp>
#include <ntools/type_utilities.hpp>
#include <ntools/mt_check/unordered_map.hpp>

#include <ntools/memory_pool.hpp>
#include <ntools/refcount_ptr.hpp>

#include <hydra/assets/image.hpp>
#include <hydra/utilities/holders.hpp>
#include <hydra/utilities/transfer_context.hpp>
#include <hydra/utilities/refcount_pooled_res_ptr.hpp>


#include "resource_array.hpp"
#include "texture_manager_shader_structs.hpp"

namespace neam::hydra
{
  struct texture_manager_configuration : hydra::conf::hconf<texture_manager_configuration, "configuration/texture_manager.hcnf", conf::location_t::index_program_local_dir>
  {
    uint32_t entries_to_allocate_at_once = 64;
    uint32_t max_entries = 8192;
    uint64_t evict_no_question_asked = 7200; // FIXME: maybe time based? (number of ms?)
    uint64_t max_pool_memory = 2ull * 1024 * 1024 * 1024;
  };
}

N_METADATA_STRUCT(neam::hydra::texture_manager_configuration)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(entries_to_allocate_at_once, neam::metadata::info{.description = c_string_t
    <
      "By how much new entries the array of resource grows everytimes it needs to grow.\n"
      "A bigger number mean bigger updates in case of high contention, for the cost of more unused entries"
    >}),
    N_MEMBER_DEF(max_entries, neam::metadata::info{.description = c_string_t
    <
      "Max number of entries in the array of resource. No extra entry will be held that this amount.\n"
      "If that number is too low, requested resources will not be loaded or will be unloaded as soon as they are not used, leading to extra IO operations"
    >}),
    N_MEMBER_DEF(evict_no_question_asked, neam::metadata::info{.description = c_string_t
    <
      "Past this time (in frame for now), the resource will be evicted if a new resource need to be loaded.\n"
      "A low number might leed to extra turnaround, while a too high number will lead the manager to keep all resource loaded until max_entries is reached.\n"
      "This value is ignored when max_entries is reached (this value can be seen as a chance to avoid growing the resource array).\n"
    >}),
    N_MEMBER_DEF(max_pool_memory, neam::metadata::info{.description = c_string_t
    <
      "Max GPU memory (in bytes) that the textures will use\n"
      "Note that by default the manager will not go above this limit but will try to keep close to it\n"
      "The amount is calculated from the true memory cost of allocations, not the (usually lower) actual required memory for a texture\n"
    >})
  >;
};
namespace neam::hydra
{
  struct hydra_context;

  using texture_index_t = uint32_t;
  static constexpr texture_index_t k_invalid_texture_index = ~0u;

  /// \brief Handle texture stream-in/stream-out, default textures, ...
  /// \todo Stream-out based on usage (need gpu->cpu transfers)
  class texture_manager
  {
    public:
      explicit texture_manager(hydra_context& _hctx);

      /// \brief Return the total GPU memory used by the textures currently loaded by the manager
      uint64_t get_total_gpu_memory() const { return total_memory.load(std::memory_order_relaxed); }

      /// \brief Ask for that texture to be considered for streamed-in, return a unique index for that texture
      /// \note That function can be called multiple time for the same resource, and will always return the same value
      /// \note The returned index is valid until the next call to clear()
      /// \warning If the resource is invalid (which cannot be know immediatly in some cases) or not of the proper type,
      ///          The returned index will be kept valid but shaders will default to either a purple or black texture
      ///          (+ an error will be logged)
      [[nodiscard]] texture_index_t request_texture_index(string_id texture_rid);

      /// \brief Indicate, cpu-side, that the specified texture is being used at a specified mip-level
      ///        Will trigger stream-in of the texture or prevent it from being streamed-out
      /// \note There is a mechanism to perform this operation directly/automatically in the shaders reading the texture
      /// \todo that mechanism
      void indicate_texture_usage(texture_index_t tid, uint32_t targetted_mip_level);

      constexpr uint32_t texture_index_to_gpu_index(texture_index_t tid) const { return tid + 1; }

      /// \brief Fully clear all textures, invalidating all the texture indices
      /// \note trigger on_texture_pool_cleared
      /// \note the answer to an index reload is not a clear of the pool, but a reload of it
      /// \warning Should prooobably be called outside rendering operations / when no rendering context is active
      void clear();

      /// \brief Force a full reload of all the data the pool hold.
      /// Does not change anything, just reload everything from disk and upload it to the gpu
      void force_full_reload();

      cr::event<> on_texture_pool_cleared;

      shader_structs::texture_manager_descriptor_set_t& get_descriptor_set() { return gpu_state.descriptor_set; }
      const shader_structs::texture_manager_descriptor_set_t& get_descriptor_set() const { return gpu_state.descriptor_set; }

      /// \brief Allows to query the size / ...
      /// \note If the image hasn't been loaded yet, a default object will be returned (with format set to undefined)
      assets::image get_image_asset(texture_index_t index) const;

    public: // advanced:
      VkImageView _get_vk_image_view_for(texture_index_t index) const;

    public: // management:
      void process_start_of_frame(vk::submit_info& si);

      void begin_engine_shutdown();

      void memory_budget_fit(bool aggressive = false);

    private:
      static constexpr uint32_t k_invalid_index = ~0u;
      static constexpr uint32_t k_invalid_mip = 0xFF;

      struct image_gpu_data : public cr::refcounted_t
      {
        mutable skip_copy<shared_spinlock> view_lock; // protects image.view
        std::optional<image_holder> image;
        std::optional<vk::sampler> sampler;

        skip_copy<std::atomic<uint64_t>> loaded_mip_mask;

        async::continuation_chain upload_chain;

        bool valid = false;
        bool evicted = false;

        texture_manager* owner = nullptr;

        void immediate_resource_release();
      };

      cr::memory_pool<image_gpu_data> gpu_data_pool;

      struct texture_entry : utilities::resource_array_entry_base_t
      {
        shaders::texture_type_t type = shaders::no_type;

        uint8_t requested_mip_level = k_invalid_mip;
        uint8_t streamed_mip_level = k_invalid_mip;
        uint8_t min_immediate_mip_level = k_invalid_mip;

        bool invalid_resource = true;

        string_id asset_rid;
        assets::image image_information;

        mutable skip_copy<shared_spinlock> lock;
        dfe_refcount_pooled_ptr<image_gpu_data> gpu_data;
      };
      struct gpu_state_t
      {
        // buffer of type: shader_structs::texture_indirection_t
        std::optional<buffer_holder> indirection_buffer;

        // the descriptor set:
        shader_structs::texture_manager_descriptor_set_t descriptor_set;
      };

    private:
      void load_texture_data_unlocked(texture_index_t tid, string_id rid);
      void load_mip_data_unlocked(texture_entry& entry, texture_index_t tid);

    private:
      hydra_context& hctx;

      cr::event_token_t on_index_loaded_tk;

      shared_spinlock texture_id_map_lock;
      std::mtc_unordered_map<id_t, uint32_t> texture_id_map;

      utilities::resource_array<texture_entry> res;

      // gpu resources:
      shared_spinlock gpu_state_lock;
      gpu_state_t gpu_state;

      transfer_context image_data_txctx;
      transfer_context txctx { hctx };

      vk::sampler default_sampler;
      image_holder default_texture;
      bool first_init = true;
      std::atomic<bool> has_changed { true };
      std::atomic<uint64_t> total_memory { 0 };

      texture_manager_configuration configuration;
  };
}

