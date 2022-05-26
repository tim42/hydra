//
// created by : Timothée Feuillet
// date: 2021-12-5
//
//
// Copyright (c) 2021 Timothée Feuillet
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

#include "context.hpp"
#include "packer.hpp"
#include "processor.hpp"
#include "file_map.hpp"
#include "compressor.hpp"
#include "mimetype/mimetype.hpp"
#include <hydra/engine/core_context.hpp>

#include <filesystem>

namespace neam::resources
{
  std::string context::get_prefix_from_filename(const std::string& name)
  {
    std::filesystem::path p(name);
    return p.remove_filename();
  }

  context::status_chain context::boot(id_t boot_index_id, id_t _index_file_id, unsigned max_depth)
  {
    prefix = io_context.get_prefix_directory();
    root = {};
    has_index = false;
    has_rel_db = false;
    index_file_id = id_t::invalid;

    neam::cr::out().debug("boot: using base prefix: {}", prefix);


    return reload_index(boot_index_id, _index_file_id)
    .then([this, max_depth, _index_file_id](status index_st) -> context::status_chain 
    {
      if (index_st == status::failure)
        return status_chain::create_and_complete(status::failure);

      if (index::entry initial_index_file = root.get_entry(k_initial_index); initial_index_file.is_valid())
      {
        if (initial_index_file.pack_file == id_t::none)
        {
          const index::entry reldb_entry = root.get_entry(k_reldb_index);
          const id_t reldb_fid = reldb_entry.pack_file;
          if (!reldb_entry.is_valid() || !io_context.is_file_mapped(reldb_fid))
          {
            neam::cr::out().debug("index loaded, but no reld-db file present in file-map");
            return status_chain::create_and_complete(index_st);
          }

          neam::cr::out().debug("index loaded, trying to load rel-db file...");

          return io_context.queue_read(reldb_fid, 0, io::context::whole_file)
          .then([this, reldb_fid, index_st](raw_data&& file, bool success)
          {
            status final_status = index_st;
            if (success)
            {
              const rle::status st = rle::in_place_deserialize(file, db);
              if (st == rle::status::success)
              {
                neam::cr::out().debug("{}: loaded rel-db", io_context.get_filename(reldb_fid));
                has_rel_db = true;
              }
              else
              {
                neam::cr::out().warn("{}: rel-db is not valid", io_context.get_filename(reldb_fid));
                has_rel_db = false;
                final_status = worst(final_status, status::partial_success);
              }
            }
            else
            {
              neam::cr::out().debug("{}: rel-db file does not exist", io_context.get_filename(reldb_fid));
              has_rel_db = false;
            }
            neam::cr::out().log("Boot process completed.");
            return status_chain::create_and_complete(final_status);
          });
        }
        if (initial_index_file.pack_file == index_file_id)
        {
          neam::cr::out().error("Cannot complete boot process: direct index loop detected");
          return status_chain::create_and_complete(status::failure);
        }
        else
        {
          if (max_depth == 0)
          {
            neam::cr::out().error("Cannot complete the boot process: max chain-load depth reached.");
            return status_chain::create_and_complete(status::failure);
          }

          if (index::entry initial_index_key = root.get_entry(k_index_key); initial_index_key.is_valid())
          {
            neam::cr::out().log("Chain-loading to index: {}", io_context.get_filename(initial_index_file.pack_file));
            // Chain-load the next index:
            return boot(initial_index_key.pack_file, initial_index_file.pack_file, max_depth - 1);
          }
          neam::cr::out().error("Missing index key to chain-load the next index in the sequence.");
          return status_chain::create_and_complete(status::failure);
        }
      }

      neam::cr::out().warn("Index does not match the bootable index format. Stopping the boot process with partial success.");

      return status_chain::create_and_complete(worst(index_st, status::partial_success));
    });
  }

  context::status_chain context::make_self_boot(id_t boot_index_id, const std::string& index_path, file_map&& boot_file_map)
  {
    index new_index(boot_index_id);
    new_index.add_entry({ .id = k_initial_index, .flags = flags::type_virtual, .pack_file = id_t::none });
    new_index.add_entry({ .id = k_boot_file_map, .flags = flags::type_data | flags::embedded_data });

    // add the rel-db to the file-map: (so auto-load can work)
    const std::filesystem::path prefix = io_context.get_prefix_directory() + (io_context.get_prefix_directory().empty() ? "" : "/") + boot_file_map.prefix_path;
    const std::string rel_db_file = (index_path + k_rel_db_extension);
    const std::filesystem::path rel_db_path = io_context.get_prefix_directory() + (io_context.get_prefix_directory().empty() ? "" : "/") + rel_db_file;

    if (!boot_file_map.prefix_path.empty())
    {
      const std::filesystem::path rel_db_path = io_context.get_prefix_directory() + (io_context.get_prefix_directory().empty() ? "" : "/") + (index_path + k_rel_db_extension);
      const std::string rel_rel_db_path = rel_db_path.lexically_relative(prefix);
      boot_file_map.files.insert(rel_rel_db_path);

      new_index.add_entry({ .id = k_reldb_index, .flags = flags::type_virtual | flags::to_strip, .pack_file = io::context::get_file_id(rel_rel_db_path) });
    }
    else
    {
      boot_file_map.files.insert(rel_db_file);
      new_index.add_entry({ .id = k_reldb_index, .flags = flags::type_virtual | flags::to_strip, .pack_file = io::context::get_file_id(rel_db_file) });
    }

    // Set the embedded file-map:
    status st;
    raw_data file_map_data = file_map::to_raw_data(boot_file_map, st);
    if (st != status::success)
    {
      neam::cr::out().error("make_self_boot: cannot serialize the file-map.");
      return status_chain::create_and_complete(status::failure);
    }
    if (!new_index.set_embedded_data(k_boot_file_map, std::move(file_map_data)))
    {
      neam::cr::out().error("make_self_boot: cannot embed the file-map in the index.");
      return status_chain::create_and_complete(status::failure);
    }

    // Write the index/an empty rel_db:
    id_t ifid = io_context.map_file(index_path);
    id_t rdbfid = io_context.map_file(rel_db_file);
    auto idx_chain = write_index(ifid, new_index);
    auto rdb_chain = io_context.queue_write(rdbfid, 0, rle::serialize<rel_db>({}));
    return async::multi_chain<bool>(true, [](bool& state, bool ret)
    {
      state = state && ret;
    }, idx_chain, rdb_chain)
    .then([=, this](bool success)
    {
      io_context.unmap_file(ifid);
      io_context.unmap_file(rdbfid);
      if (success)
      {
        neam::cr::out().debug("make_self_boot: index/rel_db successfuly saved.");
        return status::success;
      }
      neam::cr::out().error("make_self_boot: failed to write the index/rel_db.");
      return status::failure;
    });
;
  }

  context::status_chain context::make_chain_boot(id_t target_index_id, std::string target_index_path, std::string target_index_file, id_t boot_index_id, const std::string& boot_index_path)
  {
    index new_index(boot_index_id);
    new_index.add_entry({ .id = k_initial_index, .flags = flags::type_virtual, .pack_file = io::context::get_file_id(target_index_file) });
    new_index.add_entry({ .id = k_index_key, .flags = flags::type_virtual, .pack_file = target_index_id });
    new_index.add_entry({ .id = k_boot_file_map, .flags = flags::type_data | flags::embedded_data });

    file_map fm;
    fm.prefix_path = std::move(target_index_path);
    fm.files.insert(std::move(target_index_file));

    // Set the embedded file-map:
    status st;
    raw_data file_map_data = file_map::to_raw_data(fm, st);
    if (st != status::success)
    {
      neam::cr::out().error("make_chain_boot: cannot serialize the file-map.");
      return status_chain::create_and_complete(status::failure);
    }
    if (!new_index.set_embedded_data(k_boot_file_map, std::move(file_map_data)))
    {
      neam::cr::out().error("make_chain_boot: cannot embed the file-map in the index.");
      return status_chain::create_and_complete(status::failure);
    }

    // Write the index:
    id_t ifid = io_context.map_file(boot_index_path);
    return write_index(ifid, new_index)
    .then([=, this](bool success)
    {
      io_context.unmap_file(ifid);
      if (success)
      {
        neam::cr::out().debug("make_chain_boot: index successfuly saved.");
        return status::success;
      }
      neam::cr::out().error("make_chain_boot: failed to write the index.");
      return status::failure;
    });
  }

  context::status_chain context::add_index(id_t index_id, id_t index_fid)
  {
    check::debug::n_assert(has_index, "Trying to combines indexes while no index has been ever loaded. Are you loading and combining right away?");
    return io_context.queue_read(index_fid, 0, io::context::whole_file)
           .then([=, this](raw_data&& data, bool success)
    {
      if (!check::debug::n_check(success, "Failed to load index {}", io_context.get_filename(index_fid)))
          return status::failure;

      bool has_rejected_entries = false;
      root.add_index(index::read_index(index_id, data, &has_rejected_entries));
      neam::cr::out().debug("Additively loaded index: {} [combined index contains {} entries]", io_context.get_filename(index_fid), root.entry_count());
      return has_rejected_entries ? status::partial_success : status::success;
    });
  }

  context::status_chain context::save_index() const
  {
    check::debug::n_assert(has_index, "Trying to save while no index has been ever loaded. Are you loading and saving right away?");
    io::context::write_chain chn = write_index(index_file_id, root);

    // save the rel-db too
    if (has_rel_db)
    {
      const index::entry reldb_entry = root.get_entry(k_reldb_index);
      const id_t reldb_fid = reldb_entry.pack_file;
      if (reldb_entry.is_valid() && io_context.is_file_mapped(reldb_fid))
      {
        io::context::write_chain rel_chn = io_context.queue_write(reldb_fid, 0, rle::serialize(db));
        return async::multi_chain<bool>(true, [](bool & state, bool ret)
        {
          state = state && ret;
        }, std::move(chn), std::move(rel_chn))
        .then([ =, this, fid = index_file_id](bool success)
        {
          if (!check::debug::n_check(success, "Failed to save index {}", io_context.get_filename(fid)))
            return status::failure;
          neam::cr::out().debug("Saved index: {}", io_context.get_filename(fid));
          return status::success;
        });
      }
    }
    {
      return chn.then([ =, this, fid = index_file_id](bool success)
      {
        if (!check::debug::n_check(success, "Failed to save index {}", io_context.get_filename(fid)))
          return status::failure;
        neam::cr::out().debug("Saved index: {}", io_context.get_filename(fid));
        return status::success;
      });
    }
  }

  io::context::write_chain context::write_index(id_t file_id, const index& idx) const
  {
    return io_context.queue_write(file_id, 0, idx.serialize_index());
  }

  context::status_chain context::load_file_map(const id_t rid)
  {
    return read_resource<file_map>(rid)
    .then([=, this](file_map&& data, status st)
    {
      if (st != status::success)
      {
        cr::out().warn("Could not correctly load file-map {:X}", (uint64_t)rid);
        return status::failure;
      }

      apply_file_map(data);
      return status::success;
    });
  }

  io::context::read_chain context::read_raw_resource(id_t rid)
  {
    const index::entry entry = root.get_entry(rid);
    if (!entry.is_valid() || ((entry.flags & flags::type_mask) != flags::type_data))
      return io::context::read_chain::create_and_complete({}, false);

    // check for embedded data, as it's data already in memory
    if ((entry.flags & flags::embedded_data) != flags::none)
    {
      if (raw_data* data = root.get_embedded_data(rid); data != nullptr)
      {
        // duplicate and return the data
        return io::context::read_chain::create_and_complete(raw_data::duplicate(*data), true);
      }
      else
      {
        cr::out().warn("failed to load resource: {:x}: was marked as embedded data but no embedded data found", std::to_underlying(rid));
        return io::context::read_chain::create_and_complete({}, false);
      }
    }

    if (!io_context.is_file_mapped(entry.pack_file))
      return io::context::read_chain::create_and_complete({}, false);

    return io_context.queue_read(entry.pack_file, entry.offset,
                                 (entry.flags & flags::standalone_file) != flags::none ? io::context::whole_file : entry.size)
           .then([rid](raw_data && data, bool success)
    {
      if (!success)
        cr::out().warn("failed to load resource: {:x}", std::to_underlying(rid));
      else
        cr::out().debug("loaded resource: {:x} [size: {}b]", std::to_underlying(rid), data.size);


      return io::context::read_chain::create_and_complete(std::move(data), success);
    })
    .then([](raw_data&& data, bool success)
    {
      // TODO: unxor data properly
      return uncompress(std::move(data))
             .then([success](raw_data && data)
      {
        return io::context::read_chain::create_and_complete(std::move(data), success);
      });
    });
  }

  context::status_chain context::reload_index(id_t index_id, id_t fid)
  {
    return io_context.queue_read(fid, 0, io::context::whole_file)
           .then([=, this](raw_data&& data, bool success)
    {
      if (!check::debug::n_check(success, "Failed to load index {}", io_context.get_filename(fid)))
      {
          has_index = true;
          return status_chain::create_and_complete(status::failure);
      }

      bool has_rejected_entries = false;
      root = index::read_index(index_id, data, &has_rejected_entries);
      neam::cr::out().debug("loaded index: {} [contains {} entries]", io_context.get_filename(fid), root.entry_count());
      has_index = true;
      index_file_id = fid;

      // grab the embedded file-map, if any:
      return load_file_map(k_boot_file_map)
      .then([has_rejected_entries](status fm_st)
      {
        if (fm_st == status::success)
          cr::out().debug("Loaded index file-map successfuly");
        else
          cr::out().log("Could not apply index file-map");

        return (has_rejected_entries || fm_st != status::success) ? status::partial_success : status::success;
      });

    });
  }

  void context::apply_file_map(const file_map& fm, bool additive)
  {
    std::lock_guard _l(file_map_lock);
    if (!additive)
    {
      io_context.clear_mapped_files();
      current_file_map.files.clear();
    }

    cr::out().debug("Applying file-map with {} entries (prefix: {})", fm.files.size(), fm.prefix_path);
    io_context.set_prefix_directory(prefix + (prefix.empty() ? "" : "/") + fm.prefix_path);
    current_file_map.prefix_path = fm.prefix_path;
    cr::out().debug("File-map consolidated prefix: {}", io_context.get_prefix_directory());
    for (const auto& it : fm.files)
    {
      if (io_context.is_file_mapped(io_context.get_file_id(it)))
      {
        cr::out().debug("File-map: file {} is already mapped. (ID collision ?)", it);
      }
      current_file_map.files.insert(it);
      [[maybe_unused]] id_t iid = io_context.map_file(it);
    }
  }

  void context::add_to_file_map(std::string file)
  {
    std::lock_guard _l(file_map_lock);
    current_file_map.files.insert(file);
    [[maybe_unused]] id_t iid = io_context.map_file(file); // apply the change

    status st = status::success;
    raw_data file_map_data = resources::file_map::to_raw_data(current_file_map, st);
    if (st != status::success)
    {
      cr::out().error("Failed to serialize the boot file-map");
      return;
    }

    if (!root.set_embedded_data(k_boot_file_map, std::move(file_map_data)))
    {
      cr::out().error("Failed to embed the boot file-map");
      return;
    }
    cr::out().debug("Added 1 entry from the boot file-map");

  }

  void context::remove_from_file_map(const std::set<id_t>& rids)
  {
    std::lock_guard _l(file_map_lock);
    for (id_t rid : rids)
    {
      current_file_map.files.erase((std::string)io_context.get_filename(rid));
      io_context.close(rid);
      io_context.unmap_file(rid);
    }
    status st = status::success;
    raw_data file_map_data = resources::file_map::to_raw_data(current_file_map, st);
    if (st != status::success)
    {
      cr::out().error("Failed to serialize the boot file-map");
      return;
    }

    if (!root.set_embedded_data(k_boot_file_map, std::move(file_map_data)))
    {
      cr::out().error("Failed to embed the boot file-map");
      return;
    }
    cr::out().debug("Removed {} entries from the boot file-map", rids.size());
  }

  context::status_chain context::import_resource(const std::filesystem::path& resource)
  {
    check::debug::n_assert(has_rel_db, "refusing to import resources without a rel db present");

    std::filesystem::path meta_resource = resource;
    meta_resource += k_metadata_extension;

    cr::out().debug("import_resource: importing resource from file: {}", resource.c_str());
    const id_t fid = io_context.map_unprefixed_file(source_folder / resource);
    const id_t mdfid = io_context.map_unprefixed_file(source_folder / meta_resource);

    // queue the resource read:
    auto res_chn = io_context.queue_read(fid, 0, io::context::whole_file)
    .then([=, this](raw_data&& data, bool success)
    {
      io_context.unmap_file(fid);
      if (!success)
      {
        cr::out().error("import_resource: failed to read the source file: {}", resource.c_str());
        return raw_data{};
      }
      return data;
    });

    // queue the metadata read:
    auto md_chn = io_context.queue_read(mdfid, 0, io::context::whole_file)
    .then(&ctx.tm, threading::k_non_transient_task_group, [=](raw_data&& raw_metadata, bool success)
    {
      metadata_t metadata;

      // the presence of a metadata file is optional (and the absence of it means that the struct is empty)
      if (success)
        metadata = metadata_t::deserialize(raw_metadata);
      else
        cr::out().debug("import_resource: no metadata for the source file: {}", resource.c_str());

      metadata.file_id = mdfid;

      // the later process will handle the unmap for us.
      return metadata;
    });

    // queue the resource removal/file cleanup (in case of a re-import)
    async::chain<bool> rmv_chain = on_source_file_removed(resource)
    .then([]
    {
      return true;
    });

    // multi-chain state:
    struct state_t
    {
      metadata_t metadata;
      raw_data res;
    };

    // add the file back to the rel-db now
    // (it's not done in the other import-resource at it can be called recursively and add_file(file) clear the entries for it)
    db.add_file(resource);

    // as we perform both metadata and resource reads at the same time, we wait for them both:
    return async::multi_chain<state_t>(state_t{}, []<typename Arg>(state_t& state, Arg&& arg)
    {
      if constexpr (std::is_same_v<Arg, metadata_t>)
        state.metadata = std::move(arg);
      else if constexpr (std::is_same_v<Arg, raw_data>)
        state.res = std::move(arg);
      else if constexpr (std::is_same_v<Arg, bool>) {}
      else
        static_assert(!sizeof(Arg), "Unknown type received");
    }, res_chn, md_chn, rmv_chain)
    // the chain the next step in the import process:
    .then([=, this](state_t&& state)
    {
      if (!state.res.data)
        return status_chain::create_and_complete(status::failure);

      return import_resource(resource, std::move(state.res), std::move(state.metadata));
    });
  }

  context::status_chain context::import_resource(const std::filesystem::path& resource, raw_data&& data, metadata_t&& metadata)
  {
    check::debug::n_assert(has_rel_db, "refusing to import resources without a rel db present");

    cr::out().debug("import_resource: importing resource: {}", resource.c_str());

    // initial step: we process the resource:
    const processor::function proc = processor::get_processor(data, resource);
    if (!proc)
    {
      // not really an error, in normal mode we skip the file, but in debug we send a warn message
      if (cr::out.min_severity == cr::logger::severity::debug)
      {
        cr::out().warn("import_resource: resource {}: could not find a processor (mimetype: {}, extension: {})",
                       resource.c_str(), mime::get_mimetype(data), resource.extension().c_str());
      }
      return status_chain::create_and_complete(status::failure);
    }

    // we got our processor:
    processor::chain chn;
    ctx.tm.get_long_duration_task([proc, this, resource, data = std::move(data), metadata = std::move(metadata), state = chn.create_state()] mutable
    {
      proc(ctx, {resource, std::move(data), std::move(metadata)}).use_state(state);
    });
    return chn.then([=, this](processor::processed_data&& pd, status s)
    {
      // FIXME: should handle caching results to avoid re-processing data

      // we forward the failure status, but partial success is ok (we will still forward the partial success status)
      if (s == status::failure)
        return status_chain::create_and_complete(status::failure);

      if (pd.to_pack.size() == 0 && pd.to_process.size() == 0)
      {
        // not a debug as it is a bit strange:
        cr::out().warn("import_resource: resource {}: process did not have any outputs.", resource.c_str());
        cr::out().warn("import_resource: resource {}: if those resources are to be excluded, please use the exclusion list", resource.c_str());
        return status_chain::create_and_complete(s);
      }

      // pack all the sub-resources:
      std::vector<status_chain> chains;
      chains.reserve(pd.to_pack.size() + pd.to_process.size());

      for (auto& it : pd.to_process)
      {
        db.add_file(resource, it.file); // add a sub-file
        chains.emplace_back(import_resource(it.file, std::move(it.file_data), std::move(it.metadata)));
      }

      for (auto& it : pd.to_pack)
      {
        db.add_resource(resource, it.resource_id); // add a root resource
        chains.emplace_back(_pack_resource(std::move(it)));
      }

      // wait for everything:
      return async::multi_chain<status>(std::move(s), std::move(chains), [](status & res, status val)
      {
        res = worst(res, val);
      });
    });
  }

  context::status_chain context::_pack_resource(processor::data&& proc_data)
  {
    check::debug::n_assert(has_rel_db, "refusing to pack resources without a rel db present");

    const string_id res_id = proc_data.resource_id;

    const packer::function pack = packer::get_packer(proc_data.resource_type);
    if (pack == nullptr)
    {
      cr::out().error("pack_resource: resource {}: failed to find packer for type: {}", proc_data.resource_id, proc_data.resource_type);
      return status_chain::create_and_complete(status::failure);
    }

    cr::out().debug("pack_resource: resource {}: type: {}", proc_data.resource_id, proc_data.resource_type);

    packer::chain chain;
    ctx.tm.get_long_duration_task([pack, this, proc_data = std::move(proc_data), state = chain.create_state()]() mutable
    {
      pack(ctx, std::move(proc_data)).use_state(state);
    });
    return chain.then([=, this](std::vector<packer::data>&& v, id_t pack_id, status s)
    {
      // we forward the failure status, but partial success is ok (we will still forward the partial success status)
      if (s == status::failure)
      {
        cr::out().error("pack_resource: failed to pack resource {}", res_id);
        return status_chain::create_and_complete(status::failure);
      }

      if (v.size() == 0)
      {
        // not a debug as it is a bit strange:
        cr::out().warn("pack_resource: resource {} does not contain sub-resources, will not add resource to index", res_id);
        return status_chain::create_and_complete(s);
      }

      const std::string filename = fmt::format("res-{:X}{}", std::to_underlying(pack_id), k_pack_extension);
      const id_t pack_file = io_context.map_file(filename);

      db.set_pack_file(res_id, pack_file);

      std::vector<status_chain> chains;
      chains.reserve(v.size() * 2 + 1);

      add_to_file_map(filename);

      cr::out().debug("pack_resource: pack-file {} contains {} sub-rsources", filename, v.size());

      // We have a correctly packed resource, insert it to the index (as a standalone file)
      uint64_t offset = 0;
      for (auto& it : v)
      {
        static_assert(N_RES_MAX_SIZE_TO_EMBED > 0, "N_RES_MAX_SIZE_TO_EMBED must be > 0");

        db.add_resource(res_id, it.id);

        // save the resource + add it to the index:
        if (it.data.size > N_RES_MAX_SIZE_TO_EMBED)
        {
          // compress and write to disk:
          chains.emplace_back(compress(std::move(it.data))
          .then([=, this, id = it.id](raw_data&& data)
          {
            const uint64_t sz = data.size;
            return io_context.queue_write(pack_file, offset, std::move(data), false)
            .then([sz](bool success) -> uint64_t
            {
              if (!success) return 0;
              return sz;
            });
          })
          .then([=, this, id = it.id](uint64_t sz)
          {
            if (!sz) // a size of 0 is impossible (as we check for > N_RES_MAX_SIZE_TO_EMBED)
            {
              cr::out().error("pack_resource: failed to write sub-resource to pack-file: {} (sub resource: {})", filename, id);
              return status_chain::create_and_complete(status::failure);
            }

            root.add_entry(index::entry
            {
              .id = id,
              .flags = flags::type_data,
              .pack_file = pack_file,
              .offset = offset,
              .size = sz,
            });

            return status_chain::create_and_complete(status::success);
          }));

          offset += it.data.size;
        }
        else // size <= N_RES_MAX_SIZE_TO_EMBED
        {
          // entries of size 0 should always be embedded (it's a smaller footprint than a full index entry):
          // having max size to embed being too big may cause memory issues as those resources will always be in memory
          root.add_entry(index::entry
          {
            .id = it.id,
            .flags = flags::type_data | flags::embedded_data,
          });
          // we don't compress embedded data, as the index is compressed (we might also end-up with extra data, as it's a very small emtry
          root.set_embedded_data(it.id, std::move(it.data));
        }

        // save/remove the metadata file:
        if (it.metadata.file_id != id_t::invalid)
        {
          if (it.metadata.empty())
          {
            if (it.metadata.initial_hash != id_t::none)
            {
              chains.push_back(io_context.queue_deferred_remove(it.metadata.file_id)
              .then([](bool)
              {
                // we don't care about whether we succeded or not (the file might not even exist)
                return status::success;
              }));
            }
          }
          else
          {
            // write the metadata, but only if it has chanegd
            raw_data raw_metadata = rle::serialize(it.metadata);
            const id_t final_hash = (id_t)ct::hash::fnv1a<64>((const uint8_t*)raw_metadata.data.get(), raw_metadata.size);
            if (final_hash != it.metadata.initial_hash)
            {
              chains.push_back(io_context.queue_write(it.metadata.file_id, 0, std::move(raw_metadata))
              .then([](bool res)
              {
                return res ? status::success : status::failure;
              }));
            }
          }
        }
      }

      return async::multi_chain<status>(std::move(s), std::move(chains), [](status& res, status val)
      {
        res = worst(res, val);
      })
      .then([res_id, sub_res_count = v.size()](status s)
      {
        cr::out().log("pack_resource: packed resource {} (with {} sub-resources)", res_id, sub_res_count);
        return s;
      });
    });
  }

  async::continuation_chain context::on_source_file_removed(const std::filesystem::path& file)
  {
    check::debug::n_assert(has_rel_db, "cannot handle a removed source file without a rel db present");

    // FIXME: should be one call
    const std::set<id_t> packs = db.get_pack_files(file);
    const std::set<id_t> resources = db.get_resources(file);
    db.remove_file(file);


    std::vector<async::continuation_chain> chains;
    chains.reserve(packs.size() + 1);

    remove_from_file_map(packs);

    for (id_t pack : packs)
      chains.push_back(io_context.queue_deferred_remove(pack).to_continuation());

    for (id_t res : resources)
      root.remove_entry(res);

    return async::multi_chain(std::move(chains));
  }
}

