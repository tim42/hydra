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

#include <ntools/macro.hpp>
#include <ntools/ct_string.hpp>

#include <filesystem>

namespace neam::resources
{
  // base resource metadata type: (currently private as it's only for internal use)
  struct default_resource_metadata_t : public base_metadata_entry<default_resource_metadata_t>
  {
    static constexpr ct::string k_metadata_entry_description = "generic metadata used by the resource subsystem";
    static constexpr ct::string k_metadata_entry_name = "resource_metadata";


    bool strip_from_final_build = false;
    bool embed_in_index = false;
    bool skip_compression = false;
  };
}

N_METADATA_STRUCT(neam::resources::default_resource_metadata_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(strip_from_final_build, neam::metadata::info{.description = c_string_t
      <
      "Whether this resource should be completely removed from the final build.\n"
      "Please note that no reference check is done, and the resource is forcefully stripped from the data."
      >}),
    N_MEMBER_DEF(embed_in_index, neam::metadata::info{.description = c_string_t
      <
      "Whether this resource should be placed directly in the index instead of in a pack file\n"
      "Doing so will increase the size of the index and will force the resources to always be loaded in memory\n"
      "This also make sure that no filesystem access is needed to retrieve the resource\n"
      "The default behavior (if false) is controlled by N_RES_MAX_SIZE_TO_EMBED (currently: " N_EXP_STRINGIFY(N_RES_MAX_SIZE_TO_EMBED) "b)."
      >}),
    N_MEMBER_DEF(skip_compression, neam::metadata::info{.description = c_string_t
      <
      "Whether this resource should skip compression.\n"
      "Note that only resources bigger than " N_EXP_STRINGIFY(N_RES_MAX_SIZE_TO_COMPRESS) " bytes will be considered for compression\n"
#if !N_RES_LZMA_COMPRESSION
      "\n"
      "NOTE: LZMA compression disabled, flag will not do anything as everything will skip compression.\n"
      "Flag will be saved an honored when compression is enabled in the build.\n"
#endif
      >})
  >;
};


namespace neam::resources
{
  std::string context::get_prefix_from_filename(const std::string& name)
  {
    std::filesystem::path p(name);
    return p.remove_filename();
  }

  context::status_chain context::boot(id_t boot_index_id, id_t _index_file_id, unsigned max_depth, bool reload)
  {
    if (!reload)
    {
      prefix = io_context.get_prefix_directory();
      root = {};
      has_index = false;
      has_rel_db = false;
      index_file_id = id_t::invalid;
    }
    neam::cr::out().debug("boot: using base prefix: {}", prefix);


    return reload_index(boot_index_id, _index_file_id)
    .then([this, max_depth](status index_st) -> context::status_chain 
    {
      if (index_st == status::failure)
        return status_chain::create_and_complete(status::failure);

      if (index::entry initial_index_file = root.get_entry(k_initial_index); initial_index_file.is_valid())
      {
        if (initial_index_file.pack_file == id_t::none)
        {
          if (const index::entry self_index_entry = root.get_entry(k_self_index); self_index_entry.is_valid())
            index_file_id = self_index_entry.pack_file;
          else
            neam::cr::out().warn("index does not possess a self-reference, index reload might not be possible");

          configuration = {}; // reset the conf

          const index::entry reldb_entry = root.get_entry(k_reldb_index);
          const id_t reldb_fid = reldb_entry.pack_file;
          if (!reldb_entry.is_valid() || !io_context.is_file_mapped(reldb_fid))
          {
            neam::cr::out().debug("index loaded, but no reld-db file present in file-map");
            return status_chain::create_and_complete(index_st);
          }

          neam::cr::out().debug("index loaded, trying to load configuration and rel-db file...");

          // NOTE: we only load the conf if there's a chance to have a rel-db
          auto conf_chain =
#if N_STRIP_DEBUG
          ctx.hconf.read_conf(configuration, k_configuration_name)
#else
          ctx.hconf.read_or_create_conf(configuration, std::string(k_configuration_name.get_string_view()))
#endif
          .then([](bool res) { return res ? status::success : status::failure; });

          auto rel_db_chain = io_context.queue_read(reldb_fid, 0, io::context::whole_file)
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
                neam::cr::out().error("{}: rel-db is not valid", io_context.get_filename(reldb_fid));
                has_rel_db = false;
                final_status = worst(final_status, status::partial_success);
              }
            }
            else
            {
              neam::cr::out().error("{}: rel-db file does not exist (please use strip_repack to strip unused resources)", io_context.get_filename(reldb_fid));
              has_rel_db = false;
            }

            return status_chain::create_and_complete(final_status);
          });

          return async::multi_chain(status::success, cr::construct<std::vector>(std::move(conf_chain), std::move(rel_db_chain)), [](status& state, status add)
          {
            state = worst(state, add);
          }).then([this](status st)
          {
            // send the event:
            ctx.tm.get_task([this]() { on_index_loaded(); });

            neam::cr::out().log("Boot process completed.");
            return st;
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

      // send the event:
      ctx.tm.get_task([this]() { on_index_loaded(); });

      return status_chain::create_and_complete(worst(index_st, status::partial_success));
    });
  }

  context::status_chain context::make_self_boot(id_t boot_index_id, const std::string& index_path, file_map&& boot_file_map)
  {
    index new_index(boot_index_id);
    new_index.add_entry({ .id = k_initial_index, .flags = flags::type_virtual, .pack_file = id_t::none });
    new_index.add_entry({ .id = k_boot_file_map, .flags = flags::type_data | flags::embedded_data });

    // add the rel-db to the file-map: (so auto-load can work)
    const std::filesystem::path cwd_prefix = io_context.get_prefix_directory() + (io_context.get_prefix_directory().empty() ? "" : "/");
    const std::filesystem::path post_index_prefix = cwd_prefix / boot_file_map.prefix_path;
    const std::string rel_db_file = (index_path + k_rel_db_extension);

    if (!boot_file_map.prefix_path.empty())
    {
      const std::filesystem::path rel_db_path_cwd_rel = cwd_prefix / (index_path + k_rel_db_extension);
      const std::filesystem::path index_path_cwd_rel = cwd_prefix / (index_path);

      const std::string rel_db_post_index_path = rel_db_path_cwd_rel.lexically_relative(post_index_prefix);
      const std::string index_post_index_path = index_path_cwd_rel.lexically_relative(post_index_prefix);

      boot_file_map.files.insert(index_post_index_path);
      boot_file_map.files.insert(rel_db_post_index_path);

      new_index.add_entry({ .id = k_reldb_index, .flags = flags::type_virtual | flags::to_strip, .pack_file = io::context::get_file_id(rel_db_post_index_path) });
      new_index.add_entry({ .id = k_self_index, .flags = flags::type_virtual, .pack_file = io::context::get_file_id(index_post_index_path) });
    }
    else
    {
      boot_file_map.files.insert(rel_db_file);
      boot_file_map.files.insert(index_path);

      new_index.add_entry({ .id = k_reldb_index, .flags = flags::type_virtual | flags::to_strip, .pack_file = io::context::get_file_id(rel_db_file) });
      new_index.add_entry({ .id = k_self_index, .flags = flags::type_virtual, .pack_file = io::context::get_file_id(index_path) });
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
    check::debug::n_check(has_index, "Trying to save while no index has been ever loaded. Are you loading and saving right away?");
    check::debug::n_check(io_context.is_file_mapped(index_file_id), "Index file is not mapped to io, will not save index");

    io::context::write_chain chn = write_index(index_file_id, root);

    // save the rel-db too
    if (has_rel_db)
    {
      const index::entry reldb_entry = root.get_entry(k_reldb_index);
      const id_t reldb_fid = reldb_entry.pack_file;
      if (reldb_entry.is_valid() && io_context.is_file_mapped(reldb_fid))
      {
        io::context::write_chain rel_chn = io_context.queue_write(reldb_fid, 0, db.serialize());
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
        cr::out().warn("Could not correctly load file-map {}", resource_name(rid));
        return status::failure;
      }

      apply_file_map(data);
      return status::success;
    });
  }

  std::string context::resource_name(id_t rid) const
  {
    if (io_context.is_file_mapped(rid))
      return std::string{io_context.get_filename(rid)};
    if (has_rel_db)
      return db.resource_name(rid);
    return fmt::format("{}", rid);
  }

  const rel_db& context::get_db() const
  {
    check::debug::n_assert(has_rel_db, "refusing to give a db when no db present");
    return db;
  }

  rel_db& context::_get_non_const_db()
  {
    check::debug::n_assert(has_rel_db, "refusing to give a db when no db present");
    return db;
  }

  io::context::read_chain context::read_raw_resource(id_t rid)
  {
    const index::entry entry = root.get_entry(rid);
    if (!root.has_entry(rid) || !entry.is_valid() || ((entry.flags & flags::type_mask) != flags::type_data))
    {
      cr::out().warn("failed to load resource: {}: resource does not exist or is not a data resource", resource_name(rid));
      return io::context::read_chain::create_and_complete({}, false);
    }

    // check for embedded data, as it's data already in memory
    if ((entry.flags & flags::embedded_data) != flags::none)
    {
      if (raw_data* data = root.get_embedded_data(rid); data != nullptr)
      {
        // duplicate and return the data
        cr::out().debug("loaded resource: {} [size: {}b] (from embedded data)", resource_name(rid), data->size);
        return io::context::read_chain::create_and_complete(raw_data::duplicate(*data), true);
      }
      else
      {
        cr::out().warn("failed to load resource: {}: was marked as embedded data but no embedded data found", resource_name(rid));
        return io::context::read_chain::create_and_complete({}, false);
      }
    }

    if (!io_context.is_file_mapped(entry.pack_file))
    {
      cr::out().warn("failed to load resource: {}: pack file is not in the file-map", resource_name(rid));
      return io::context::read_chain::create_and_complete({}, false);
    }

    return io_context.queue_read(entry.pack_file, entry.offset,
                                 (entry.flags & flags::standalone_file) != flags::none ? io::context::whole_file : entry.size)
           .then([rid, this](raw_data && data, bool success)
    {
      if (!success)
        cr::out().warn("failed to load resource: {} (read failed)", resource_name(rid));
      else
        cr::out().debug("loaded resource: {} [size: {}b]", resource_name(rid), data.size);


      return io::context::read_chain::create_and_complete(std::move(data), success);
    })
    .then([this, res_flags = entry.flags](raw_data&& data, bool success)
    {
      if (!success)
        return io::context::read_chain::create_and_complete({}, false);
      // TODO: unxor data properly

      if ((res_flags & flags::compressed) == flags::none)
        return io::context::read_chain::create_and_complete(std::move(data), success);

#if N_RES_LZMA_COMPRESSION
      return uncompress(std::move(data), &ctx.tm, threading::k_non_transient_task_group)
             .then([success](raw_data && data)
      {
        return io::context::read_chain::create_and_complete(std::move(data), success);
      });
#else
      neam::cr::out().error("read_raw_resource: trying to read a compressed resource without LZMA support");
      return io::context::read_chain::create_and_complete({}, status::failure);
#endif // N_RES_LZMA_COMPRESSION
    });
  }

  context::status_chain context::write_raw_resource(id_t rid, raw_data&& data)
  {
    const index::entry entry = root.get_entry(rid);
    if (!root.has_entry(rid) || !entry.is_valid() || ((entry.flags & flags::type_mask) != flags::type_data))
    {
      cr::out().warn("failed to write resource: {}: resource does not exist or is not a data resource", resource_name(rid));
      return status_chain::create_and_complete(status::failure);
    }

    // check for embedded data, as it's invalid (would require an index modification)
    if ((entry.flags & flags::embedded_data) != flags::none)
    {
      cr::out().warn("failed to write resource: {}: resource is an embedded resource (would require index modification)", resource_name(rid));
      return status_chain::create_and_complete(status::failure);
    }

    // FIXME: support other modes of resources:
    if ((entry.flags & flags::standalone_file) != flags::none)
    {
      // uncompressed resources are simply written as is for now:
      if ((entry.flags & flags::compressed) == flags::none)
      {
        return io_context.queue_write(rid, 0, std::move(data)).then([](bool success)
        {
          return success ? status::success : status::failure;
        });
      }
      else
      {
#if N_RES_LZMA_COMPRESSION
        return compress(std::move(data), &ctx.tm, threading::k_non_transient_task_group)
        .then([this, rid](raw_data&& data)
        {
          return io_context.queue_write(rid, 0, std::move(data)).then([](bool success)
          {
            return success ? status::success : status::failure;
          });
        });
#else
      cr::out().warn("failed to write resource: {}: resource is to be compressed, but engine is built without LZMA support", resource_name(rid));
      return status_chain::create_and_complete(status::failure);
#endif
      }
    }

    cr::out().warn("failed to write resource: {}: resource is not a flags::standalone_file. Currently only standalone_file can be written to.", resource_name(rid));
    return status_chain::create_and_complete(status::failure);
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
    if (rids.empty())
      return;
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

    // check for excluded resources (by extension):
    {
      const std::string extension = resource.extension();
      for (const auto& it : configuration.extensions_to_ignore)
      {
        if (it == extension)
        {
          cr::out().debug("import_resource: skipping import for file: {}: extension is in the list of files to ignore", resource.c_str());
          // indicate success as nothing has failed
          return status_chain::create_and_complete(status::success);
        }
      }
    }

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
    .then(/*&ctx.tm, threading::k_non_transient_task_group, */[=](raw_data&& raw_metadata, bool success)
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
    async::chain<bool> rmv_chain;
    ctx.tm.get_long_duration_task([this, resource, state = rmv_chain.create_state()] mutable
    {
      on_source_file_removed(resource, true /* reimport */)
      .then([this, resource]
      {
        // add the file back to the rel-db now
        // (it's not done in the other import-resource at it can be called recursively and add_file(file) clear the entries for it)
        db.add_file(resource);
        return true;
      }).use_state(state);
    });

    // multi-chain state:
    struct state_t
    {
      metadata_t metadata;
      raw_data res;
    };


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
    .then(/*&ctx.tm, threading::k_non_transient_task_group, */[=, this](state_t&& state)
    {
      if (!state.res.data)
        return status_chain::create_and_complete(status::failure);

      return import_resource(resource, std::move(state.res), std::move(state.metadata));
    });
  }

  context::status_chain context::import_resource(const std::filesystem::path& resource, raw_data&& data, metadata_t&& metadata)
  {
    check::debug::n_assert(has_rel_db, "refusing to import resources without a rel db present");

    // FIXME: maybe somewhere else:
    // check for mimetype and extension:
    {
      const std::string mimetype = mime::get_mimetype(data);
      const std::string extension = resource.extension();
      for (const auto& it : configuration.extensions_to_ignore)
      {
        if (it == mimetype)
        {
          cr::out().debug("import_resource: skipping import for file: {}: mime type ({}) is in the list of types to ignore", resource.c_str(), mimetype);
          // indicate success as nothing has failed
          return status_chain::create_and_complete(status::success);
        }
        if (it == extension)
        {
          cr::out().debug("import_resource: skipping import for file: {}: extension is in the list of files to ignore", resource.c_str());
          // indicate success as nothing has failed
          return status_chain::create_and_complete(status::success);
        }
      }

    }

    cr::out().debug("import_resource: importing resource: {}", resource.c_str());

    // we got our processor:
    processor::chain chn;
    ctx.tm.get_long_duration_task([/*proc, */this, resource, data = std::move(data), metadata = std::move(metadata), state = chn.create_state()] mutable
    {
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

        db.set_processor_for_file(resource, id_t::none);

        // failure to find a processor does not indicate a failure state
        // it just means the resource is unknown
        state.complete({}, status::success);
        return;
      }

      db.set_processor_for_file(resource, processor::get_processor_hash(data, resource));
      proc(ctx, {resource, std::move(data), std::move(metadata), db}).use_state(state);
    });
    return chn.then([=, this](processor::processed_data&& pd, status s)
    {
      // FIXME: should handle caching results to avoid re-processing data

      // maintain the db state, even in case of import failure
      for (auto& it : pd.to_process)
        db.add_file(resource, it.file); // add a sub-file
      for (auto& it : pd.to_pack)
        db.add_resource(resource, it.resource_id); // add a root resource

      // we forward the failure status, but partial success is ok (we will still forward the partial success status)
      if (s == status::failure)
        return status_chain::create_and_complete(status::failure);

      if (pd.to_pack.size() == 0 && pd.to_process.size() == 0)
      {
        // not a debug as it is a bit strange:
//         cr::out().warn("import_resource: resource {}: process did not have any outputs.", resource.c_str());
//         cr::out().warn("import_resource: resource {}: if those resources are to be excluded, please use the exclusion list", resource.c_str());
        return status_chain::create_and_complete(s);
      }

      // pack all the sub-resources:
      std::vector<status_chain> chains;
      chains.reserve(pd.to_pack.size() + pd.to_process.size());

      for (auto& it : pd.to_process)
        chains.emplace_back(import_resource(it.file, std::move(it.file_data), std::move(it.metadata)));

      for (auto& it : pd.to_pack)
        chains.emplace_back(_pack_resource(std::move(it)));

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
    const id_t packer_hash = packer::get_packer_hash(proc_data.resource_type);

    if (pack == nullptr)
    {
      cr::out().error("pack_resource: resource {}: failed to find packer for type: {}", resource_name(proc_data.resource_id), proc_data.resource_type);
      return status_chain::create_and_complete(status::failure);
    }

    cr::out().debug("pack_resource: resource {}: type: {}", resource_name(proc_data.resource_id), proc_data.resource_type);

    packer::chain chain;
    ctx.tm.get_long_duration_task([pack, this, proc_data = std::move(proc_data), state = chain.create_state()]() mutable
    {
      pack(ctx, std::move(proc_data)).use_state(state);
    });
    struct post_compression_data
    {
      packer::data data;
      bool is_compressed = false;
    };
    async::chain<std::vector<post_compression_data>, id_t, status> final_chain;
    chain.then([=, state = final_chain.create_state(), this](std::vector<packer::data>&& v, id_t pack_id, status s) mutable
    {
      std::vector<post_compression_data> pcv;
      // we forward the failure status, but partial success is ok (we will still forward the partial success status)
      if (s == status::failure)
      {
        cr::out().error("pack_resource: failed to pack resource {}", resource_name(res_id));

        // forward the data:
        for (auto& it : v)
          pcv.push_back({std::move(it), false});

        state.complete(std::move(pcv), pack_id, s);
        return;
      }

      // launch compression tasks for what can be compressed
      std::vector<async::chain<packer::data, uint32_t>> compress_chains;
      pcv.resize(v.size());
      compress_chains.reserve(v.size());

      for (uint32_t i = 0; i < v.size(); ++i)
      {
        default_resource_metadata_t resource_metadata;
        v[i].metadata.try_get<default_resource_metadata_t>(resource_metadata);

#if N_RES_LZMA_COMPRESSION
        if (v[i].data.size > N_RES_MAX_SIZE_TO_COMPRESS && (v[i].mode == packer::mode_t::data) && !resource_metadata.skip_compression)
        {
          // compress
          compress_chains.push_back(compress(std::move(v[i].data), &ctx.tm, threading::k_non_transient_task_group)
          .then([i, d = std::move(v[i])](raw_data&& data) mutable
          {
            d.data = std::move(data);
            return async::chain<packer::data, uint32_t>::create_and_complete(std::move(d), i);
          }));
        }
        else
#endif // N_RES_LZMA_COMPRESSION
        {
          pcv[i] = { std::move(v[i]), false, }; // uncompressed
        }
      }
      async::multi_chain(std::move(pcv), std::move(compress_chains), [](auto& state, packer::data&& data, uint32_t index)
      {
        state[index] = { std::move(data), true };
      }).then([pack_id, s, state = std::move(state)](std::vector<post_compression_data>&& v) mutable
      {
        state.complete(std::move(v), pack_id, s);
      });
    });
    return final_chain.then([=, this](std::vector<post_compression_data>&& v, id_t pack_id, status s)
    {
      // update the db info, even in case of failure:
      const std::string filename = fmt::format("res-{:X}{}", std::to_underlying(pack_id), k_pack_extension);
      const id_t pack_file = io_context.map_file(filename);

      db.set_pack_file(res_id, pack_file);
      db.set_packer_for_resource(res_id, packer_hash);
      db.reference_metadata_type<default_resource_metadata_t>(res_id);
      for (auto& it : v)
        db.add_resource(res_id, it.data.id);

      // forward fialure state:
      if (s == status::failure)
        return status_chain::create_and_complete(s);

      if (v.size() == 0)
      {
        // not a debug as it is a bit strange:
        cr::out().warn("pack_resource: resource {} does not contain sub-resources, will not add resource to index", resource_name(res_id));
        return status_chain::create_and_complete(s);
      }


      std::vector<status_chain> chains;
      chains.reserve(v.size() * 2 + 1);

      add_to_file_map(filename);

      cr::out().debug("pack_resource: pack-file {} contains {} sub-rsources", filename, v.size());

      uint64_t offset = 0;
      for (auto& it : v)
      {
        static_assert(N_RES_MAX_SIZE_TO_EMBED > 0, "N_RES_MAX_SIZE_TO_EMBED must be > 0");

        auto& pack_data = it.data;
        default_resource_metadata_t resource_metadata;
        pack_data.metadata.try_get<default_resource_metadata_t>(resource_metadata);

        flags extra_flags = flags::none;
        if (resource_metadata.strip_from_final_build)
          extra_flags |= flags::to_strip;

        if (pack_data.id == id_t::none || pack_data.id == id_t::invalid)
        {
          cr::out().error("pack_resource: sub-resource of pack-file: {} has an invalid resource-id", filename);
          continue;
        }
        db.add_resource(res_id, pack_data.id);

        if (pack_data.mode == packer::mode_t::data)
        {
          // save the resource + add it to the index:
          if (pack_data.data.size > N_RES_MAX_SIZE_TO_EMBED && !resource_metadata.embed_in_index)
          {
            const uint64_t sz = pack_data.data.size;
            const uint64_t file_offset = offset;
            offset += sz;

            // write to disk:
            chains.emplace_back(io_context.queue_write(pack_file, file_offset, std::move(pack_data.data), false)
              .then([=, this, id = pack_data.id, is_compressed = it.is_compressed](bool success)
            {
              if (!success)
              {
                cr::out().error("pack_resource: failed to write sub-resource to pack-file: {} (sub resource: {})", filename, resource_name(id));
                return status_chain::create_and_complete(status::failure);
              }

              const bool index_success = root.add_entry(index::entry
              {
                .id = id,
                .flags = flags::type_data | extra_flags | (is_compressed ? flags::compressed : flags::none),
                .pack_file = pack_file,
                .offset = file_offset,
                .size = sz,
              });
              return status_chain::create_and_complete(index_success ? status::success : status::failure);
            }));
          }
          else // size <= N_RES_MAX_SIZE_TO_EMBED
          {
            // entries of size 0 should always be embedded (it's a smaller footprint than a full index entry):
            // having max size to embed being too big may cause memory issues as those resources will always be in memory
            root.add_entry(index::entry
            {
              .id = pack_data.id,
              .flags = flags::type_data | extra_flags | flags::embedded_data | (it.is_compressed ? flags::compressed : flags::none),
            }, std::move(pack_data.data));
          }

          // save/remove the metadata file:
          if (pack_data.metadata.file_id != id_t::invalid)
          {
            if (pack_data.metadata.empty())
            {
              if (pack_data.metadata.initial_hash != id_t::none)
              {
                chains.push_back(io_context.queue_deferred_remove(pack_data.metadata.file_id)
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
              raw_data raw_metadata = rle::serialize(pack_data.metadata);
              const id_t final_hash = (id_t)ct::hash::fnv1a<64>((const uint8_t*)raw_metadata.data.get(), raw_metadata.size);
              if (final_hash != pack_data.metadata.initial_hash)
              {
                chains.push_back(io_context.queue_write(pack_data.metadata.file_id, 0, std::move(raw_metadata))
                .then([](bool res)
                {
                  return res ? status::success : status::failure;
                }));
              }
            }
          }
        }
        else if (pack_data.mode == packer::mode_t::simlink)
        {
            root.add_entry(index::entry
            {
              .id = pack_data.id,
              .flags = flags::type_simlink | extra_flags,
              .pack_file = pack_data.simlink_to_id,
            }, std::move(pack_data.data));
        }
      }

      return async::multi_chain<status>(std::move(s), std::move(chains), [](status& res, status val)
      {
        res = worst(res, val);
      })
      .then([this, res_id, sub_res_count = v.size()](status s)
      {
        cr::out().debug("pack_resource: packed resource {} (with {} sub-resources)", resource_name(res_id), sub_res_count);
        return s;
      });
    });
  }

  async::continuation_chain context::on_source_file_removed(const std::filesystem::path& file, bool reimport)
  {
    check::debug::n_assert(has_rel_db, "cannot handle a removed source file without a rel db present");

    // FIXME: should be one call
    const std::set<id_t> packs = db.get_pack_files(file);
    const std::set<id_t> resources = db.get_resources(file);

      if (reimport)
        db.repack_file(file);
      else
        db.remove_file(file);

    std::vector<async::continuation_chain> chains;
    chains.reserve(packs.size() + 1);

    for (id_t res : resources)
      root.remove_entry(res);
    for (id_t pack : packs)
      chains.push_back(io_context.queue_deferred_remove(pack).to_continuation());

    // once everything is removed, remove those files from the file-map
    return async::multi_chain(std::move(chains)).then([this, packs = std::move(packs)]
    {
      remove_from_file_map(packs);
    });
  }

  std::set<std::filesystem::path> context::get_sources_needing_reimport() const
  {
    check::debug::n_assert(has_rel_db, "cannot return sources needing reimport without a rel db present");
    return filter_files(db.get_files_requiring_reimport(processor::get_processor_hashs(), packer::get_packer_hashs()));
  }

  std::set<std::filesystem::path> context::filter_files(std::set<std::filesystem::path>&& files) const
  {
    for (auto it = files.begin(); it != files.end();)
    {
      const std::string extension = it->extension();
      for (const auto& ext : configuration.extensions_to_ignore)
      {
        if (ext == extension)
        {
          it = files.erase(it);
          continue;
        }
      }
      ++it;
    }
    return files;
  }

  std::set<std::filesystem::path>& context::filter_files(std::set<std::filesystem::path>& files) const
  {
    for (auto it = files.begin(); it != files.end();)
    {
      const std::string extension = it->extension();
      for (const auto& ext : configuration.extensions_to_ignore)
      {
        if (ext == extension)
        {
          it = files.erase(it);
          continue;
        }
      }
      ++it;
    }
    return files;
  }
}

