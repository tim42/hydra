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

#include <hydra/resources/resources.hpp>
#include <hydra/hydra_debug.hpp>
#include <hydra/engine/engine.hpp>
#include <hydra/engine/engine_module.hpp>
#include <hydra/engine/core_modules/io_module.cpp>

#include "fs_watcher.hpp"
#include "options.hpp"

namespace neam::hydra
{
  class packer_engine_module : public engine_module<packer_engine_module>
  {
    public: // options
      bool stall_task_manager = false;

      global_options packer_options;
      // number of resources that will be queued at the same time
      uint32_t resources_to_queue = 64;

      // entries that are necessary for the UI.
      // Will be packed first, if they are dirty
      static constexpr std::array k_priority_list =
      {
        "shaders/engine/imgui/imgui.hsf",
      };

      cr::event<uint32_t /* modified */, uint32_t /* indirect_mod */,  uint32_t /* added */, uint32_t /* to_remove */> on_packing_started;
      cr::event<const std::filesystem::path&> on_resource_queued;
      cr::event<const std::filesystem::path&, resources::status> on_resource_packed;
      cr::event<resources::status> on_index_saved;
      cr::event<> on_packing_ended;

    public: // API
      bool is_packing() const { return state.in_progress; }

      size_t get_total_entry_to_pack() const { return state.entry_count; }
      size_t get_packed_entries() const { return state.counter; }

    private:
      static constexpr const char* module_name = "packer";

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
        tgd.add_task_group("pack"_rid);
      }

      void add_task_groups_dependencies(threading::task_group_dependency_tree& tgd) override
      {
//         tgd.add_dependency("io"_rid, "pack"_rid);
      }

      void on_engine_boot_complete() override
      {
        cctx->tm.set_start_task_group_callback("pack"_rid, [this]
        {
          // update configuration:
          engine->get_module<io_module>("io"_rid)->wait_for_submit_queries = stall_task_manager;
//           cctx->io.force_deferred_execution(&cctx->tm, /*threading::k_non_transient_task_group*/cctx->tm.get_group_id("io"_rid));

          if (state.in_progress || state.is_packing)
            return;
          // long duration task: we may end-up running on multiple frames if there's the UI
          cctx->tm.get_task([this]
          {
            pack();
          });
        });
      }

      void on_resource_index_loaded() override
      {
      }

      void queue_import_resource(uint32_t index)
      {
        if (index >= state.to_import.size())
          return;

        on_resource_queued(state.to_import[index]);

        cctx->res.import_resource(state.to_import[index])
        .then(&cctx->tm, threading::k_non_transient_task_group, [this, path = state.to_import[index]](resources::status st)
        {
          on_resource_packed(path, st);
          state.need_save = true;
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

      void pack()
      {
        // we are already packing stuff, wait for the operation to be done
        if (state.in_progress || state.is_packing)
          return;

        if (chrono.get_accumulated_time() < packer_options.watch_delay && !packer_options.force && !initial_round)
          return;

        std::set<std::filesystem::path> packer_proc_dirty_files;
        if (initial_round)
          packer_proc_dirty_files = cctx->res.get_sources_needing_reimport();

        initial_round = false;

        // reset the state
        state.clear();

        ts_file_id = cctx->io.map_unprefixed_file(packer_options.ts_file);
        index_file_id = cctx->io.map_unprefixed_file(packer_options.index);


        std::deque<std::filesystem::path> all_files = neam::fs_tools::get_all_files(packer_options.source_folder);

        // filter mod files
        std::deque<std::filesystem::path> mod_files;
        std::set<std::filesystem::path> to_rm_files;
        std::set<std::filesystem::path> new_files;

        // FIXME: all three operations can be done at the same time.
        bool do_force = packer_options.force;
        packer_options.force = false;

        if (do_force)
          mod_files = all_files;
        else
          mod_files = neam::fs_tools::filter_files_newer_than(all_files, packer_options.source_folder, fs_tools::get_oldest_timestamp(packer_options.ts_file, packer_options.index));

        if (!do_force)
        {
          to_rm_files = cctx->res.get_removed_sources(all_files);
          new_files = cctx->res.get_non_imported_sources(all_files);
        }

        // merge mod files / new files:
        std::set<std::filesystem::path> to_import_prefilter;
        for (auto& it : mod_files)
          to_import_prefilter.emplace(std::move(it));
        for (const auto& it : new_files)
        {
          // metadata are ignored from the new files, only modified matters.
          if (it.extension() != resources::context::k_metadata_extension)
            to_import_prefilter.emplace(it);
        }

        // filter to import files: (remove un-necessary stuff)
        std::set<std::filesystem::path> to_import_set;
        for (auto it : to_import_prefilter)
        {
          // the the metadata changed, re-import the source file:
          if (it.extension() == resources::context::k_metadata_extension)
          {
            to_import_set.insert(std::move(it.replace_extension()));
          }
          else
          {
            to_import_set.insert(std::move(it));
          }
        }
        for (auto& it : packer_proc_dirty_files)
          to_import_set.insert(it);

        // handle dependencies:
        const size_t initial_size = to_import_set.size();
        cctx->res.consolidate_files_with_dependencies(to_import_set);
        for (auto& it : to_rm_files)
        {
          cctx->res.consolidate_files_with_dependencies(it, to_import_set);
        }
        for (auto& it : to_rm_files)
        {
          to_import_set.erase(it);
        }

        state.to_import.reserve(to_import_set.size());

        // start by pushing entries in the priority list first:
        for (const auto& it : k_priority_list)
        {
          if (auto fit = to_import_set.find(it); fit != to_import_set.end())
          {
            state.to_import.push_back(std::move(*fit));
            to_import_set.erase(fit);
          }
        }

        for (auto& it : to_import_set)
          state.to_import.push_back(/*std::move*/(it));


        // TODO: FILTER + handle .hrm changes (repack the linked resource)
        //
        // TODO: handle file removal
        // TODO: handle file creation (via copy, using stat ctime and mtime)

        if (state.to_import.size() > 0 || to_rm_files.size() > 0)
        {
          cctx->unstall_all_threads();

          state.in_progress = true;

          cr::out().log("found {} changed (+{} by dependency, {} because of code change), {} new, {} removed files in {}",
                        mod_files.size(), state.to_import.size() - initial_size, packer_proc_dirty_files.size(),
                        new_files.size(), to_rm_files.size(),
                        packer_options.source_folder.c_str());

          if (packer_options.print_source_name)
          {
            if (state.to_import.size() > 0)
            {
              cr::out().log("Modified/New files:");
              for (auto& it: state.to_import)
              {
                cr::out().log("  {}   [new: {}, code change: {}]", it.c_str(), new_files.contains(it), packer_proc_dirty_files.contains(it));
              }
            }

            if (to_rm_files.size() > 0)
            {
              cr::out().log("Removed files:");
              for (auto& it: to_rm_files)
              {
                cr::out().log("  {}", it.c_str());
              }
            }
          }

          on_packing_started(mod_files.size(), state.to_import.size() - initial_size, new_files.size(), to_rm_files.size());

          state.gbl_chains.reserve(1 + to_rm_files.size());

          state.entry_count = state.to_import.size();
          state.need_save = to_rm_files.size() > 0;

          state.import_end_state = {};
          if (state.to_import.size() > 0)
            state.gbl_chains.push_back(state.import_end_state.create_chain());

          state.gbl_chains.push_back(cctx->io.queue_write(ts_file_id, io::context::truncate, raw_data::allocate_from(std::string("[timestamp file, do not touch]\n")))
                                     .then([this](raw_data&& /*data*/, bool /*success*/, size_t /*write_size*/)
          {
            // do the resource import
            // (we increment to_import_index right away so that we do queue resources_to_queue
            // ((otherwise, if a resource is completed while we loop and it queues another one, we won't queue resources_to_queue resources)
            constexpr uint32_t initial_resource_count = 8 + k_priority_list.size();
            state.to_import_index += initial_resource_count;
            for (uint32_t i = 0; i < initial_resource_count && i < state.to_import.size(); ++i)
              queue_import_resource(i);
          }));

          for (const auto& it : to_rm_files)
            state.gbl_chains.push_back(cctx->res.on_source_file_removed(it));

          // after everything, save the changes:
          async::multi_chain(std::move(state.gbl_chains))
          .then([this]
          {
            if (state.need_save)
            {
              // Assign the metadata types from this binary to the rel-db
              cctx->res._get_non_const_db().force_assign_registered_metadata_types();

              cctx->res.save_index().then([this](resources::status st)
              {
                if (st == resources::status::failure)
                  cr::out().error("failed to save index {}", packer_options.index.c_str());
                else
                  cr::out().log("index saved on disk");

                on_index_saved(st);

                chrono.reset();
                state.in_progress = false;
                state.is_packing = false;
                on_packing_ended();
              });
            }
            else
            {
              chrono.reset();
              state.in_progress = false;
              state.is_packing = false;
              on_packing_ended();
            }
            if (!packer_options.watch)
            {
              engine->sync_teardown();
            }
          });
          cr::out().debug("waiting to {} entry to complete...", state.entry_count);
        }
        else // nothing to be done, stall or exit
        {
          state.in_progress = false;
          state.is_packing = false;

          if (!packer_options.watch)
          {
            engine->sync_teardown();
          }
          else
          {
            cctx->stall_all_threads_except(2);
          }
        }
      }

    private: // state
      neam::id_t ts_file_id;
      neam::id_t index_file_id;

      cr::chrono chrono;
      bool initial_round = true;

      struct packer_state_t
      {
        bool in_progress = false;
        bool is_packing = false;
        bool need_save = false;

        std::vector<std::filesystem::path> to_import;
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
          to_import.clear();
          to_import_index = 0;
          import_in_progress = 0;
          gbl_chains.clear();
          entry_count = 0;
          counter = 0;
          need_save = false;
        }
      };

      packer_state_t state = {};

      friend class engine_t;
      friend engine_module<packer_engine_module>;
  };
}

