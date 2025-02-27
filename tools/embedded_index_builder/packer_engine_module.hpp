//
// created by : Timothée Feuillet
// date: 2022-7-17
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

#include <ntools/chrono.hpp>
#include <ntools/event.hpp>
#include <ntools/ct_string.hpp>

#include <hydra/resources/resources.hpp>
#include <hydra/hydra_debug.hpp>
#include <hydra/engine/engine.hpp>
#include <hydra/engine/engine_module.hpp>
#include <hydra/engine/core_modules/io_module.cpp>
#include <hydra/resources/compressor.hpp>

#include "options.hpp"

namespace neam::hydra
{
  class packer_engine_module : public engine_module<packer_engine_module>
  {
    public: // options
      bool stall_task_manager = false;

      global_options packer_options;
      // number of resources that will be queued at the same time
      uint32_t resources_to_queue = 512;

      std::vector<std::string_view> files_to_pack;

    public: // API
      bool is_packing() const { return state.in_progress; }

      size_t get_total_entry_to_pack() const { return state.entry_count; }
      size_t get_packed_entries() const { return state.counter; }

    private:
      static constexpr neam::string_t module_name = "packer";

      static bool is_compatible_with(runtime_mode m)
      {
        // exclusion flags:
        if ((m & runtime_mode::packer_less) != runtime_mode::none)
          return false;
        // necessary stuff:
        if ((m & runtime_mode::core) == runtime_mode::none)
          return false;

        return true;
      }

      void add_task_groups(threading::task_group_dependency_tree& tgd) override
      {
      }

      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override
      {
      }

      void on_context_initialized() override
      {
      }

      void on_resource_index_loaded() override
      {
        cctx->tm.get_long_duration_task([this]
        {
          pack();
        });
      }

      void queue_import_resource(uint32_t index)
      {
        if (index >= files_to_pack.size())
          return;

        resources::metadata_t md;
        {
          resources::default_resource_metadata_t rm { .embed_in_index = true };
          md.set(rm);
        }
        cctx->res.import_resource(files_to_pack[index], std::move(md))
        .then(&cctx->tm, threading::k_non_transient_task_group, [this, path = files_to_pack[index]](resources::status st)
        {
          const size_t current = ++state.counter;
          if (current % 100 == 0)
          {
            cr::out().log("{} out of {} entries processed ({} %)", current, state.entry_count, current * 100 / state.entry_count);
          }
          queue_import_resource(state.to_import_index++);
          for (uint32_t i = 0; i < 8 && state.import_in_progress < resources_to_queue; ++i)
          {
            ++state.import_in_progress;
            queue_import_resource(state.to_import_index++);
          }

          if (current == state.entry_count)
            state.import_end_state.complete();
        });
      }

      async::continuation_chain save_index()
      {
        // Assign the metadata types from this binary to the rel-db
        cctx->res._get_non_const_db().force_assign_registered_metadata_types();
        cctx->res._embed_reldb();

        // Get the raw index data:
        const raw_data index = cctx->res.get_index().serialize_index();
        const uint64_t key = (uint64_t)cctx->res.get_index().get_index_id();
        // We cannot have template for this, we are CWD agnostic and cannot rely on any existing file
        const std::string index_source = fmt::format(R"header(
//
// HYDRA Serialized Index
// File is automatically generated, please don't edit by hand
//

#include "{}"

namespace {}
{{
  // size: {} bytes, key: {:#x}
  const uint32_t index_data[{}] =
  {{
    {:#0x}
  }};
}}
)header",
          packer_options.output.filename().replace_extension("hpp"),
          packer_options.namespace_name, index.size, key, index.size / 4,
          fmt::join(std::span((const uint32_t*)index.get(), index.size / 4), ",\n    ")
        );

        const std::string index_header = fmt::format(R"header(
//
// HYDRA Serialized Index
// File is automatically generated, please don't edit by hand
//
#pragma once

#include <cstdint>

namespace {}
{{
  // size: {} bytes, key: {:#x}
  extern const uint32_t index_data[{}];
  constexpr uint64_t index_key = {:#x};
}}
)header", packer_options.namespace_name, index.size, key, index.size / 4, key);

        const uint32_t index_source_size = (uint32_t)index_source.size();
        const uint32_t index_binary_size = index.size;

        const id_t hfid = cctx->io.map_unprefixed_file(packer_options.output.replace_extension("hpp"));
        auto hchn = cctx->io.queue_read(hfid, 0, io::context::whole_file).then([this, index_header, hfid](raw_data&& dt, bool success, uint32_t)
        {
          raw_data header_data = raw_data::allocate_from(index_header);
          bool is_same = raw_data::is_same(header_data, dt);
          if (!success || !is_same)
            return cctx->io.queue_write(hfid, io::context::truncate, std::move(header_data)).to_continuation();
          cr::out().log("Index header file is identical, skipping writing the file");
          return async::continuation_chain::create_and_complete();
        });
        const id_t sfid = cctx->io.map_unprefixed_file(packer_options.output.replace_extension("cpp"));
        auto schn = cctx->io.queue_read(sfid, 0, io::context::whole_file).then([this, index_source, index_binary_size, index_source_size, sfid](raw_data&& dt, bool success, size_t)
        {
          raw_data source_data = raw_data::allocate_from(index_source);
          bool is_same = raw_data::is_same(source_data, dt);
          if (!success || !is_same)
          {
            return cctx->io.queue_write(sfid, io::context::truncate, std::move(source_data)).then([this, index_binary_size, index_source_size](raw_data&& /*data*/, bool success, size_t /*write_size*/)
            {
              if (success)
                cr::out().log("Saved index in {} (source size: {} bytes, binary size: {})", packer_options.output, index_source_size, index_binary_size);
              else
                cr::out().error("Failed to save index in {}", packer_options.output);
            });
          }
          cr::out().log("Index source file is identical, skipping writing the file");
          return async::continuation_chain::create_and_complete();
        });
        return async::multi_chain_simple(hchn, schn);
      }

      void pack()
      {
        // reset the state
        state.clear();

        if (files_to_pack.size() > 0)
        {
          cctx->unstall_all_threads();

          state.in_progress = true;

          cr::out().log("Packing {} files in {}", files_to_pack.size(), packer_options.source_folder.c_str());

          state.gbl_chains.reserve(1);

          state.entry_count = files_to_pack.size();

          state.import_end_state = {};
          state.gbl_chains.push_back(state.import_end_state.create_chain());

          {
            // do the resource import
            // (we increment to_import_index right away so that we do queue resources_to_queue
            // ((otherwise, if a resource is completed while we loop and it queues another one, we won't queue resources_to_queue resources)
            constexpr uint32_t initial_resource_count = 64;
            state.to_import_index += initial_resource_count;
            for (uint32_t i = 0; i < initial_resource_count && i < files_to_pack.size(); ++i)
              queue_import_resource(i);
          }

          // after everything, save the changes:
          async::multi_chain(std::move(state.gbl_chains))
          .then([this]
          {
            {
              save_index().then([this]()
              {
                state.in_progress = false;
                state.is_packing = false;

                // Done!
                engine->sync_teardown();
              });
            }
          });
          cr::out().debug("waiting to {} entry to complete...", state.entry_count);
        }
        else
        {
          save_index().then([this]()
          {
            state.in_progress = false;
            state.is_packing = false;

            // Done!
            engine->sync_teardown();
          });
        }
      }

    private: // state
      struct packer_state_t
      {
        bool in_progress = false;
        bool is_packing = false;

        std::atomic<uint32_t> to_import_index = 0;
        std::atomic<uint32_t> import_in_progress = 0;

        async::continuation_chain::state import_end_state;
        std::vector<async::continuation_chain> gbl_chains;
        size_t entry_count = 0;
        std::atomic<size_t> counter = 0;


        void clear()
        {
          is_packing = true;
          in_progress = false;
          to_import_index = 0;
          import_in_progress = 0;
          gbl_chains.clear();
          entry_count = 0;
          counter = 0;
        }
      };

      packer_state_t state = {};

      friend class engine_t;
      friend engine_module<packer_engine_module>;
  };
}

