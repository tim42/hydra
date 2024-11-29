//
// created by : Timothée Feuillet
// date: 2023-1-20
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

#pragma once

#include <ntools/mt_check/map.hpp>
#include <ntools/io/io.hpp>
#include <ntools/rle/rle.hpp>
#include <hydra/resources/resources.hpp>

#include "conf.hpp"

// handle conf assets (.hconf)
// conf assets are a bit of a weird casse, as they can be both packed and not packed.
// (with the restriction that packed confs cannot be modified and don't have metadata).
// If a non-packed asset is present, this asset will be used instead. Otherwise, the packed version will be used.
// This is done so the resource-server can use the conf before the first packing of the assets and
// the engine can request confs before the index is loaded.
// The packed confs require an index/resource context, but otherwise just the io context is necessary
// (the io context init is synchronous, which means it's availlable at context creation / during the engine boot)
//
// As with all io/resources in hydra, conf access is asynchronous.
//
// NOTE: The .hconf packer is in the hydra_packer project

namespace neam::hydra { class core_context; }
namespace neam::hydra::conf
{
  /// \brief Handle all conf-related stuff
  class context
  {
    public:
      context(core_context& _cctx) : cctx(_cctx) {}

      /// \brief Read conf and return the content. Will return the default value of the type if not found
      /// \note default_location is used when the object does not exist
      /// \note If N_STRIP_DEBUG is true, only packed resources and files in the filemap can be accessed.
      ///
      /// Usage: read_conf("path/to/hconf-file.hconf"_rid).then([](ConfType&& conf) { /* do stuff */});
      template<typename ConfType>
      async::chain<ConfType&&, bool> read_conf(string_id conf_id, const std::string& opt_file = {})
      {
        // static_assert(std::is_base_of_v<hconf<ConfType>, ConfType>, "ConfType must inherit from neam::hydra::conf::hconf");
        std::unique_ptr<ConfType> ptr(new ConfType());
        std::lock_guard _el(spinlock_exclusive_adapter::adapt(ptr->lock));
        ptr->init_metadata_unlocked();
        const_cast<string_id&>(ptr->hconf_source) = conf_id;
        return register_for_autoupdate(*ptr, opt_file).then([ptr = std::move(ptr)](bool success) mutable
        {
          return async::chain<ConfType&&, bool>::create_and_complete(std::move(*ptr), success);
        });
      }

      /// \brief Read conf and return the content. Will not change \p conf if not found
      /// \note default_location is used when the object does not exist
      /// \note If N_STRIP_DEBUG is true, only packed resources and files in the filemap can be accessed.
      ///
      /// Usage: read_conf("path/to/hconf-file.hconf"_rid).then([](ConfType&& conf) { /* do stuff */});
      template<typename ConfType>
      async::chain<bool> read_conf(ConfType& conf, neam::string_id conf_id, const std::string& opt_file = {})
      {
        // static_assert(std::is_base_of_v<hconf<ConfType>, ConfType>, "ConfType must inherit from neam::hydra::conf::hconf");
        std::lock_guard _el(spinlock_exclusive_adapter::adapt(conf.lock));
        const_cast<string_id&>(conf.hconf_source) = conf_id;

        check::debug::n_assert(conf.is_being_initialized == false, "read_conf called on a conf object that is already being initialized");
        conf.is_initialized = false; // force read the conf

        return register_for_autoupdate(conf, opt_file);
      }

      /// \brief Read conf and return the content. Will not change \p conf if not found
      /// \note default_location is used when the object does not exist
      /// \note If N_STRIP_DEBUG is true, only packed resources and files in the filemap can be accessed.
      ///
      /// Usage: read_conf("path/to/hconf-file.hconf"_rid).then([](ConfType&& conf) { /* do stuff */});
      template<typename ConfType, ct::string_holder DefaultSource, location_t DefaultLocation>
      async::chain<bool> read_conf(hconf<ConfType, DefaultSource, DefaultLocation>& conf)
      {
        std::lock_guard _el(spinlock_exclusive_adapter::adapt(conf.lock));

        check::debug::n_assert(conf.is_being_initialized == false, "read_conf called on a conf object that is already being initialized");
        conf.is_initialized = false; // force read the conf

        return register_for_autoupdate(conf);
      }

      template<typename ConfType, ct::string_holder DefaultSource, location_t DefaultLocation>
      async::chain<bool> write_conf(const hconf<ConfType, DefaultSource, DefaultLocation>& data)
      {
        auto[rdata, rmetadata] = data.serialize();
        string_id conf_id = data.hconf_source;
        return write_raw_conf(conf_id, std::move(rdata), std::move(rmetadata));
      }

      /// \brief Create a configuration file.
      template<typename ConfType, ct::string_holder DefaultSource, location_t DefaultLocation>
      async::chain<bool> create_conf(hconf<ConfType, DefaultSource, DefaultLocation>&& data, location_t loc, const std::string& file)
      {
        auto[rdata, rmetadata] = data.serialize();
        return write_raw_conf_to_file(loc, file, std::move(rdata), std::move(rmetadata));
      }

      /// \brief Create a configuration file.
      template<typename ConfType, ct::string_holder DefaultSource, location_t DefaultLocation>
      async::chain<bool> create_conf(hconf<ConfType, DefaultSource, DefaultLocation>& data, location_t loc = DefaultLocation)
      {
        auto[rdata, rmetadata] = data.serialize();

        return write_raw_conf_to_file(loc, (std::string)DefaultSource.view(), std::move(rdata), std::move(rmetadata));
      }

      /// \brief Read a hconf file if it exist, create it otherwise.
      template<typename ConfType>
      async::chain<ConfType&&, bool> read_or_create_conf(const std::string& conf_id, location_t loc)
      {
        const string_id id = string_id::_runtime_build_from_string(conf_id.c_str(), conf_id.size());
        // static_assert(std::is_base_of_v<hconf<ConfType>, ConfType>, "ConfType must inherit from neam::hydra::conf::hconf");
        return read_conf<ConfType>(id, conf_id).then([this, loc, id, conf_id](ConfType&& conf, bool success)
        {
          if (success)
          {
            return async::chain<ConfType&&, bool>::create_and_complete(std::move(conf), true);
          }
          else
          {
            return create_conf(std::move(conf), loc, conf_id).then([this, conf_id, id](bool success)
            {
              if (!success)
                return async::chain<ConfType&&, bool>::create_and_complete(ConfType{}, false);
              return read_conf<ConfType>(id, conf_id);
            });
          }
        });
      }

      /// \brief Read a hconf file if it exist, create it otherwise.
      /// \warning \p conf must be kept alive until the whole process is completed
      template<typename ConfType>
      async::chain<bool> read_or_create_conf(ConfType& conf, const std::string& conf_id, location_t loc)
      {
        const string_id id = string_id::_runtime_build_from_string(conf_id.c_str(), conf_id.size());
        return read_conf<ConfType>(conf, id, conf_id).then([this, loc, id, conf_id, &conf](bool success)
        {
          if (success)
          {
            return async::chain<bool>::create_and_complete(true);
          }
          else
          {
            return create_conf(std::move(conf), loc, conf_id).then([this, id, conf_id, &conf](bool success)
            {
              if (!success)
                return async::chain<bool>::create_and_complete(false);
              return read_conf<ConfType>(conf, id, conf_id);
            });
          }
        });
      }

      template<typename ConfType, ct::string_holder DefaultSource, location_t DefaultLocation>
      async::chain<bool> read_or_create_conf(hconf<ConfType, DefaultSource, DefaultLocation>& conf, location_t loc = DefaultLocation)
      {
        return read_conf(conf).then([this, loc, &conf](bool success)
        {
          if (success)
          {
            return async::chain<bool>::create_and_complete(true);
          }
          else
          {
            return create_conf(conf, loc).then([this, &conf](bool success)
            {
              if (!success)
                return async::chain<bool>::create_and_complete(false);
              return read_conf(conf);
            });
          }
        });
      }

      /// \brief Force reload a previously loaded conf
      /// \note Will trigger the hconf_on_data_changed event
      /// \note The conf must be fully loaded first
      template<typename ConfType, ct::string_holder DefaultSource, location_t DefaultLocation>
      async::chain<bool /*success*/> reload_conf(hconf<ConfType, DefaultSource, DefaultLocation>& conf)
      {
        return update_conf(conf.hconf_source);
      }

      /// \brief Asynchronous function that will reload/update still-alive confs objects and trigger events
      /// \note the function will spawn a long-duration task that may spawn more tasks and may do some IO stuff.
      ///       It's best to rate-limit this function. (once every second or more is good)
      /// \warning NOT INTENDED FOR PACKER USE
      void register_watch_for_changes();

    public: // advanced
      /// \brief convert data + metadata to the hconf format
      static raw_data _to_hconf(raw_data&& data, raw_data&& metadata);
      /// \brief from a source data, provide data and metadata (if it's hconf) or simply data if it's raw data directly.
      static void _from_hconf(raw_data&& hconf_src, raw_data& data, raw_data& metadata);

      /// \brief Will go over all the files, check if there has been changes
      void _watch_for_file_changes();

      void _stop_watching_for_file_changes();

    private:
      io::context::read_chain direct_read_raw_conf(string_id conf_id);
      async::chain<raw_data&& /*data*/, raw_data&& /*metadata*/, bool /*success*/> read_raw_conf(string_id conf_id);
      async::chain<bool /*success*/> update_conf(string_id conf_id);
      async::chain<bool /*success*/> write_raw_conf(string_id conf_id, raw_data&& data, raw_data&& metadata);
      async::chain<bool /*success*/> write_raw_conf_to_file(location_t loc, const std::string& file, raw_data&& data, raw_data&& metadata);

      /// \brief Callback for the on_index_loaded event
      void on_index_changed();

    private:
      struct hconf_autowatch_entry
      {
        // called everytime the conf is loaded. on_update_tk in hconf.
        cr::event<const raw_data& /*data*/, const raw_data& /*metadata*/> on_update;

        std::filesystem::file_time_type last_mtime = {}; // for embedded resource: ignored, otherwise, mtime of the pack file/hconf file/standalone pack file
        std::atomic<bool> is_file_being_written = false;

        id_t io_mapped_file = id_t::none; // if none, it means that it's a resource
        location_t location = location_t::none;
        std::string source_file = {};
      };
      template<typename T, ct::string_holder DefaultSource, location_t DefaultLocation>
      bool _register_for_autoupdate_unlocked(hconf<T, DefaultSource, DefaultLocation>& cnf)
      {
        if constexpr (!T::hconf_watch_source_file_change)
          return true;
        if (auto it = confs.find(cnf.hconf_source); it != confs.end())
        {
          cnf.on_update_tk = it->second.on_update.add(cnf, &hconf<T, DefaultSource, DefaultLocation>::deserialize);
          cnf.register_autoupdate = [this](hconf<T, DefaultSource, DefaultLocation>& cnf){ register_for_autoupdate(cnf); };
          return true;
        }
        return false;
      }
      template<typename T, ct::string_holder DefaultSource, location_t DefaultLocation>
      async::chain<bool /*success*/> register_for_autoupdate(hconf<T, DefaultSource, DefaultLocation>& cnf, const std::string& source_file = (std::string)DefaultSource.view())
      {
        const bool need_initialization = (!cnf.is_initialized && !cnf.is_being_initialized);
        if (!T::hconf_watch_source_file_change && !need_initialization)
          return async::chain<bool>::create_and_complete(true);
        bool is_registered;
        {
          std::lock_guard _el(spinlock_shared_adapter::adapt(confs_lock));
          is_registered = _register_for_autoupdate_unlocked<T>(cnf);
        }
        if (!is_registered)
        {
          {
            std::lock_guard _el(spinlock_exclusive_adapter::adapt(confs_lock));
            is_registered = _register_for_autoupdate_unlocked<T>(cnf);
            if (!is_registered)
            {
              auto[it, ins] = confs.emplace(std::piecewise_construct, std::forward_as_tuple(cnf.hconf_source), std::forward_as_tuple());
              it->second.source_file = source_file;
#if !N_STRIP_DEBUG
              if (it->second.source_file.empty() && cnf.hconf_source.get_string() != nullptr)
                it->second.source_file = cnf.hconf_source.get_string_view();
#endif
              _register_for_autoupdate_unlocked<T, DefaultSource, DefaultLocation>(cnf);
            }
          }
        }
        if (!is_registered || need_initialization)
        {
          cnf.is_being_initialized = true;
          cnf.is_initialized = false;
          return update_conf(cnf.hconf_source);
        }
        return async::chain<bool>::create_and_complete(true);
      }

    private:
      core_context& cctx;

      mutable shared_spinlock confs_lock;
      std::mtc_map<string_id, hconf_autowatch_entry> confs;

      cr::event_token_t on_index_loaded_tk;

      mutable spinlock flags_lock;
      bool should_watch_task_exit = false;
      bool has_watch_task = false;
  };
}

