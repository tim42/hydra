//
// created by : Timothée Feuillet
// date: 2023-1-21
//
//
// Copyright (c) 2023 Timothée Feuillet
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

#include <hydra/engine/core_context.hpp>

#ifndef N_HCONF_ALLOW_FILESYSTEM_ACCESS
  #define N_HCONF_ALLOW_FILESYSTEM_ACCESS false
#endif

namespace neam::hydra::conf
{
  /// \brief Identify a conf file following the HCONF format:
  ///        [HCNF][Absolute offset of metadata start (4 bytes)][data...][metadata...]
  ///                 |                                                  ^
  ///                 |--------------------------------------------------|
  /// \note Version number (if there's any) is xored with the hconf-marker
  static constexpr uint32_t k_hconf_metadata_marker = 0x464E4348;
  static constexpr uint32_t k_hconf_header_size = sizeof(uint32_t) * 2;

  raw_data context::_to_hconf(raw_data&& data, raw_data&& metadata)
  {
    // no need for a more complex handling of data in this case:
    if (metadata.size == 0)
      return data;

    const uint32_t metadata_offset = k_hconf_header_size + data.size;
    const uint32_t fullsize = metadata_offset + metadata.size;

    raw_data ret = raw_data::allocate(fullsize);

    uint32_t* u32_ret = (uint32_t*)ret.data.get();
    u32_ret[0] = k_hconf_metadata_marker;
    u32_ret[1] = metadata_offset;

    memcpy((uint8_t*)ret.data.get() + k_hconf_header_size, data.get(), data.size);
    memcpy((uint8_t*)ret.data.get() + metadata_offset, metadata.get(), metadata.size);

    return ret;
  }

  void context::_from_hconf(raw_data&& hconf_src, raw_data& data, raw_data& metadata)
  {
    // cannot be hconf if it's under the header size
    if (hconf_src.size < k_hconf_header_size)
    {
      data = std::move(hconf_src);
      metadata.reset();
      return;
    }

    const uint32_t* u32_src = (uint32_t*)hconf_src.data.get();
    if (u32_src[0] == k_hconf_metadata_marker)
    {
      // got hconf:
      const uint32_t metadata_offset = u32_src[1];
      if (metadata_offset > hconf_src.size || metadata_offset < k_hconf_header_size)
      {
        // invalid hconf:
        cr::out().error("conf::context::_from_hconf: input data has valid header but out-of-range metadata offset. Aborting decoding hconf data.");
        data = std::move(hconf_src);
        metadata.reset();
        return;
      }

      // valid hconf file, copy the data:
      metadata = raw_data::allocate(hconf_src.size - metadata_offset);
      data = raw_data::allocate(metadata_offset - k_hconf_header_size);

      memcpy(data.get(), (const uint8_t*)hconf_src.get() + k_hconf_header_size, data.size);
      memcpy(metadata.get(), (const uint8_t*)hconf_src.get() + metadata_offset, metadata.size);
      return;
    }
    else
    {
      // not hconf:
      data = std::move(hconf_src);
      metadata.reset();
    }
  }

  io::context::read_chain context::direct_read_raw_conf(string_id conf_id)
  {
    io::context::read_chain ret;
    // long duration tasks still run during the boot process, so it's safe to use at that time
    // we do a task, as we may have synchronous FS operations (calls to stat())
    cctx.tm.get_long_duration_task([this, conf_id, state = ret.create_state()]() mutable
    {
      const auto update_confs_map = [this, &conf_id](id_t fid, location_t loc, std::filesystem::file_time_type mtime)
      {
        std::lock_guard _el(spinlock_shared_adapter::adapt(confs_lock));
        if (auto it = confs.find(conf_id); it != confs.end())
        {
          it->second.io_mapped_file = fid;
          it->second.location = loc;
          it->second.last_mtime = mtime;
        }
      };
      // first: check if file is in the file-map:
      {
        std::lock_guard _el(spinlock_shared_adapter::adapt(confs_lock));
        if (auto it = confs.find(conf_id); it != confs.end())
        {
          const id_t file_id = it->second.io_mapped_file;
          if (cctx.io.is_file_mapped(file_id))
          {
            update_confs_map(file_id, location_t::none, cctx.io.get_modified_or_created_time(file_id));
            cctx.io.queue_read(file_id, 0, io::context::whole_file).use_state(state);
            return;
          }
        }
      }
#if N_HCONF_ALLOW_FILESYSTEM_ACCESS
      // second: if debug info is not stripped, use that to retrieve the file and map it:
      {
        std::string file;
        {
          std::lock_guard _el(spinlock_shared_adapter::adapt(confs_lock));
          if (auto it = confs.find(conf_id); it != confs.end())
          {
            file = it->second.source_file;
          }
        }
#if !N_STRIP_DEBUG
        if (file.empty() && conf_id.get_string() != nullptr)
          file = conf_id.get_string_view();
#endif
        if (!file.empty())
        {
          //   substep 1: try in the source folder:
          if (!cctx.res.source_folder.empty())
          {
            std::filesystem::path fullpath = cctx.res.source_folder / file;
            if (std::filesystem::is_regular_file(fullpath))
            {
              const id_t source_fid = cctx.io.map_unprefixed_file(fullpath);
              update_confs_map(source_fid, location_t::source_dir, cctx.io.get_modified_or_created_time(source_fid));
              cctx.io.queue_read(source_fid, 0, io::context::whole_file).use_state(state);
              return;
            }
          }

          //   substep 2: try with the prefix:
          {
            std::filesystem::path fullpath = (std::filesystem::path)cctx.io.get_prefix_directory() / file;
            if (std::filesystem::is_regular_file(fullpath))
            {
              const id_t source_fid = cctx.io.map_file(file);
              update_confs_map(source_fid, location_t::io_prefixed, cctx.io.get_modified_or_created_time(source_fid));
              cctx.io.queue_read(source_fid, 0, io::context::whole_file).use_state(state);
              return;
            }
          }

          //   substep 3: try from the process CWD:
          if (std::filesystem::is_regular_file(file))
          {
            const id_t local_fid = cctx.io.map_unprefixed_file(file);
            update_confs_map(local_fid, location_t::cwd, cctx.io.get_modified_or_created_time(local_fid));
            cctx.io.queue_read(local_fid, 0, io::context::whole_file).use_state(state);
            return;
          }
        }
      }
#endif
      // third: fallback to packed resources:
      //  the rid from the file path: (hconf is stored as raw data)
      const id_t rid = specialize(conf_id, "raw");
      update_confs_map(id_t::none, location_t::none, {});

      if (cctx.res.has_resource(rid))
      {
        cctx.res.read_raw_resource(rid).use_state(state);
        return;
      }

      state.complete({}, false);
    });
    return ret;
  }

  async::chain<raw_data&& /*data*/, raw_data&& /*metadata*/, bool /*success*/> context::read_raw_conf(string_id conf_id)
  {
    return direct_read_raw_conf(conf_id).then([](raw_data&& src_data, bool success)
    {
      // handle the failure case:
      if (!success)
        return async::chain<raw_data&& /*data*/, raw_data&& /*metadata*/, bool /*success*/>::create_and_complete({}, {}, false);

      raw_data data;
      raw_data metadata;
      _from_hconf(std::move(src_data), data, metadata);
      return async::chain < raw_data && /*data*/, raw_data && /*metadata*/, bool /*success*/ >::create_and_complete(std::move(data), std::move(metadata), true);
    });
  }

  async::chain<bool /*success*/> context::update_conf(string_id conf_id)
  {
    return read_raw_conf(conf_id).then([this, conf_id](raw_data&& data, raw_data&& metadata, bool success)
    {
      if (!success)
      {
        cr::out().error("hconf: {}: failed to read source file", conf_id);
        if (auto it = confs.find(conf_id); it != confs.end())
        {
          it->second.on_update({}, {});
        }
        return false;
      }

      std::lock_guard _el(spinlock_shared_adapter::adapt(confs_lock));
      if (auto it = confs.find(conf_id); it != confs.end())
      {
        cr::out().debug("hconf: read hconf file: {}", conf_id);
        it->second.on_update(data, metadata);
        return true;
      }
      cr::out().warn("hconf: {}: successfully read source file but no entry in the conf list exists", conf_id);
      return false;
    });
  }

  io::context::write_chain context::write_raw_conf(string_id conf_id, raw_data&& data, raw_data&& metadata)
  {
    raw_data final_data = _to_hconf(std::move(data), std::move(metadata));

    {
      confs_lock.lock_shared();
      if (auto it = confs.find(conf_id); it != confs.end())
      {
        const id_t mapped_file = it->second.io_mapped_file;
        const location_t loc = it->second.location;
        const std::string file = it->second.source_file;
        confs_lock.unlock_shared();

        if (mapped_file != id_t::none && cctx.io.is_file_mapped(mapped_file))
        {
          cr::out().debug("hconf: writing hconf file: {} (file is still io-mapped)", conf_id);
          return cctx.io.queue_write(mapped_file, 0, std::move(final_data));
        }
#if N_HCONF_ALLOW_FILESYSTEM_ACCESS
        if (!file.empty())
        {
          return write_raw_conf_to_file(loc, file, std::move(data), std::move(metadata));
        }
#endif
      }
      else
      {
        confs_lock.unlock_shared();
        cr::out().warn("hconf: writing hconf file: {}: file isn't in the list of resources", conf_id);
      }
    }

    cr::out().debug("hconf: writing hconf file: {} (updating resource file)", conf_id);
    // Use the resource context to write the resource:
    const id_t rid = specialize(conf_id, "raw");
    return cctx.res.write_raw_resource(rid, std::move(final_data)).then([rid](resources::status st)
    {
#if N_HCONF_ALLOW_FILESYSTEM_ACCESS
      if (st == resources::status::failure)
        cr::out().error("failed to write hconf: {}: file isn't mapped / is not a valid resource", rid);
#endif
      return st == resources::status::failure ? false : true;
    });
  }

  io::context::write_chain context::write_raw_conf_to_file(location_t loc, const std::string& file, raw_data&& data, raw_data&& metadata)
  {
    raw_data final_data = _to_hconf(std::move(data), std::move(metadata));

#if N_HCONF_ALLOW_FILESYSTEM_ACCESS
    // we only allow this part of the function to be called when there's filesystem access for hconf
    id_t source_fid;
    switch (loc)
    {
      case location_t::io_prefixed:
        cr::out().debug("hconf: writing hconf file: {} (io-prefixed)", file);
        source_fid = cctx.io.map_file(file);
        break;
      case location_t::source_dir:
        if (!cctx.res.source_folder.empty())
        {
          cr::out().debug("hconf: writing hconf file: {} (in source dir)", file);
          source_fid = cctx.io.map_unprefixed_file(cctx.res.source_folder / file);
          break;
        }
        [[fallthrough]];
      // case location_t::none:
      case location_t::cwd:
      default:
        cr::out().debug("hconf: writing hconf file: {} (unprefixed)", file);
        source_fid = cctx.io.map_unprefixed_file(file);
        break;
    }

    return cctx.io.queue_write(source_fid, 0 /*truncate/create*/, std::move(final_data));
#endif

    return io::context::write_chain::create_and_complete(false);
  }

  void neam::hydra::conf::context::on_index_changed()
  {
    cctx.tm.get_long_duration_task([this]
    {
      // go over all the hconfs from resources, and trigger a reload on them:
      {
        std::lock_guard _sl(spinlock_shared_adapter::adapt(confs_lock));
        for (auto& it : confs)
        {
          if (it.second.io_mapped_file == id_t::none)
          {
            cr::out().debug("hconf: reloading resource {} because of index reload", it.first);
            update_conf(it.first);
          }
        }
      }
    });
  }

  void neam::hydra::conf::context::_watch_for_file_changes()
  {
    {
      std::lock_guard _sl(spinlock_shared_adapter::adapt(confs_lock));
      for (auto& it : confs)
      {
        if (it.second.io_mapped_file != id_t::none)
        {
          if (!cctx.io.is_file_mapped(it.second.io_mapped_file))
          {
            cr::out().debug("hconf: reloading {} as it's missing from the io context", it.second.source_file);
            update_conf(it.first);
          }
          else
          {
            const auto mtime = cctx.io.get_modified_or_created_time(it.second.io_mapped_file);
            if (mtime > it.second.last_mtime)
            {
              it.second.last_mtime = mtime;
              cr::out().debug("hconf: reloading {} as the source file is newer", it.second.source_file);
              update_conf(it.first);
            }
          }
        }
      }
    }

    cctx.tm.get_delayed_task([this] { _watch_for_file_changes(); }, std::chrono::seconds{1});
  }

  void neam::hydra::conf::context::register_watch_for_changes()
  {
    // register the callback for index change:
    on_index_loaded_tk = cctx.res.on_index_loaded.add(*this, &context::on_index_changed);

    cctx.tm.get_delayed_task([this] { _watch_for_file_changes(); }, std::chrono::seconds{1});
  }
}

