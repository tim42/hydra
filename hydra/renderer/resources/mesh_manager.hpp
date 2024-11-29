//
// created by : Timothée Feuillet
// date: 2024-3-10
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

#include <hydra/utilities/holders.hpp>
#include <hydra/utilities/transfer_context.hpp>

#include <hydra/assets/static_mesh.hpp>

#include "resource_array.hpp"
#include "mesh_manager_shader_structs.hpp"

namespace neam::hydra
{
  struct hydra_context;

  using mesh_index_t = uint32_t;
  static constexpr mesh_index_t k_invalid_mesh_index = ~0u;

  /// \brief Handle mesh stream-in/stream-out and resource management
  /// \todo Stream-out based on usage (need gpu->cpu transfers)
  class mesh_manager
  {
    public:
      explicit mesh_manager(hydra_context& _hctx);

      /// \brief Ask for that mesh to be considered for streamed-in, return a unique index for that mesh
      /// \note That function can be called multiple time for the same resource, and will always return the same value
      /// \note The returned index is valid until the next call to clear()
      /// \warning If the resource is invalid (which cannot be know immediatly in some cases) or not of the proper type,
      ///          an empty mesh will be yielded
      ///          (+ an error will be logged)
      [[nodiscard]] mesh_index_t request_mesh_index(string_id mesh_rid);

      /// \brief Indicate, cpu-side, that the specified texture is being used at a specified mip-level
      ///        Will trigger stream-in of the texture or prevent it from being streamed-out
      /// \note There is a mechanism to perform this operation directly/automatically in the shaders reading the texture
      /// \todo that mechanism
      void indicate_mesh_usage(mesh_index_t mid, uint32_t targetted_lod_level);

      constexpr uint32_t mesh_index_to_gpu_index(mesh_index_t mid) const { return mid + 1; }

      /// \brief Fully clear all textures, invalidating all the texture indices
      /// \note trigger on_texture_pool_cleared
      /// \note the answer to an index reload is not a clear of the pool, but a reload of it
      /// \warning Should prooobably be called outside rendering operations / when no rendering context is active
      void clear();

      /// \brief Force a full reload of all the data the pool hold.
      /// Does not change anything, just reload everything from disk and upload it to the gpu
      void force_full_reload();

      cr::event<> on_mesh_pool_cleared;

      shader_structs::mesh_manager_descriptor_set_t& get_descriptor_set() { return gpu_state.descriptor_set; }
      const shader_structs::mesh_manager_descriptor_set_t& get_descriptor_set() const { return gpu_state.descriptor_set; }
    public: // management:
      void process_start_of_frame(vk::submit_info& si);

    private:
      static constexpr uint32_t k_invalid_index = ~0u;
      static constexpr uint32_t k_invalid_lod = 0xFF;

      static constexpr uint32_t k_entries_to_allocate_at_once = 32; // fixme: maybe a bigger number?
      static constexpr uint32_t k_max_entries = 8192; // fixme: more?
      static constexpr uint64_t k_evict_no_question_asked = 7200; // fixme: maybe time based?


      struct mesh_gpu_data
      {
        std::optional<buffer_holder> buffer;

        mutable skip_copy<shared_spinlock> lock;
      };
      struct mesh_entry : utilities::resource_array_entry_base_t
      {
        uint8_t requested_lod_level = k_invalid_lod;
        uint8_t streamed_lod_level = k_invalid_lod;

        bool invalid_resource = true;

        string_id asset_rid;
        // assets::image image_information;

        mesh_gpu_data gpu_data;
      };
      struct gpu_state_t
      {
        // buffer of type: shader_structs::texture_indirection_t
        std::optional<buffer_holder> indirection_buffer;

        // the descriptor set:
        shader_structs::mesh_manager_descriptor_set_t descriptor_set;
      };

    private:
      void load_texture_data_unlocked(mesh_index_t tid, string_id rid);
      void load_mip_data_unlocked(mesh_entry& entry, mesh_index_t tid);

    private:
      hydra_context& hctx;

      cr::event_token_t on_index_loaded_tk;

      shared_spinlock texture_id_map_lock;
      std::unordered_map<id_t, uint32_t> texture_id_map;

      utilities::resource_array<mesh_entry> res;

      // gpu resources:
      shared_spinlock gpu_state_lock;
      gpu_state_t gpu_state;

      transfer_context txctx { hctx };
  };
}

