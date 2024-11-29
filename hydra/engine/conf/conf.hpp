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

#pragma once

#include <string>
#include <filesystem>
#include <ntools/event.hpp>
#include <ntools/spinlock.hpp>
#include <ntools/id/string_id.hpp>

namespace neam::hydra::conf
{
  enum class location_t
  {
    io_prefixed,
    index_local_dir, // the "local" folder next to the index. For anything the user can override that is index local.
                     // Only works if the index is a file

    index_program_local_dir, // the "local" folder next to the index, then the folder that has the same name as the program.
                             // For anything that the user can override that is specific to the program/the index.
                             // Only works if the index is a file
    source_dir,
    cwd,
    none, // disable writing the file
  };

  /// \brief all hconf types must inherit from this one
  /// \warning If auto-reload is enabled, you \e MUST make use of \e lock when reading/using the conf.
  ///          To avoid unnecessary contention using spinlock_shared_adapter is strongly suggested (unless you are modifying the object).
  template<typename Child, ct::string_holder DefaultSource = "", location_t DefaultLocation = location_t::source_dir>
  class hconf
  {
    public:
      hconf() = default;
      ~hconf() = default;
      hconf(hconf&& o)
       : hconf_source(o.hconf_source)
      {
        std::lock_guard _oel(spinlock_shared_adapter::adapt(o.lock));
        std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));

        is_being_initialized = o.is_being_initialized;
        is_initialized = o.is_initialized;
        hconf_metadata = std::move(o.hconf_metadata);

        if ((is_initialized || is_being_initialized) && o.register_autoupdate)
          o.register_autoupdate(*this);
      }
      hconf& operator = (hconf&& o)
      {
        if (&o == this) return *this;

        std::lock_guard _oel(spinlock_shared_adapter::adapt(o.lock));
        std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
        on_update_tk.release();
        hconf_source = o.hconf_source;
        is_being_initialized = o.is_being_initialized;
        is_initialized = o.is_initialized;
        hconf_metadata = std::move(o.hconf_metadata);

        if (o.register_autoupdate)
          o.register_autoupdate(*this);
        return *this;
      }


    public:
      // override those in derived types if needed.

      // If true, the instance will be re-deserialized when the file change (it is done asynchronously, so usage of `lock` is mandatory)
      //  This reload is done before the hconf_on_source_file_changed event being fired.
      // If false, the hconf_on_source_file_changed event will be fired, but reloading the resource is left to the user
      static constexpr bool hconf_autoreload_on_source_file_change = true;
      // If false, no event will be fired and the source file will not be watched for changes
      static constexpr bool hconf_watch_source_file_change = true;

      // If true, will forward the calls to deserialize() and serialize() to the final class
      static constexpr bool hconf_has_custom_serialization = false;

      // Event called when the source file has changed (if hconf_autoreload_on_source_file_change is true, after the data has been deserialized).
      // Called without any lock being held.
      mutable cr::event<> hconf_on_data_changed;
#if !N_STRIP_DEBUG
      string_id hconf_source = string_id::_runtime_build_from_string(DefaultSource.view());
#else
      string_id hconf_source = DefaultSource; // setup automatically, used by the auto-reload. Also, identifier of the hconf object, if automanaged
#endif
      static constexpr ct::string_holder default_source = DefaultSource;
      static constexpr location_t default_location = DefaultLocation;

      // if auto-update is enabled, the auto-updater will acquire an exclusive lock the instance
      // note: when reading/updating the data, that lock must be held (shared/exclusive) to avoid race conditions on saves/updates
      mutable shared_spinlock lock;


    public:
      // virtual ~hconf() {/* check::debug::n_assert(!is_being_initialized, "Cannot destruct an hconf object that is being deserialized (source: {})", hconf_source); */}

      const raw_data& get_hconf_metadata() const { return hconf_metadata; }

      bool is_loaded() const { return is_initialized; }

      void remove_watch()
      {
        on_update_tk.release();
        register_autoupdate = {};
      }

    protected:

    private:
      void init_metadata_unlocked()
      {
        if constexpr (!Child::hconf_has_custom_serialization)
          hconf_metadata = rle::serialize(rle::generate_metadata<Child>());
      }

      std::pair<raw_data, raw_data> serialize() const
      {
        std::lock_guard _el(spinlock_shared_adapter::adapt(lock));

        if constexpr (Child::hconf_has_custom_serialization)
        {
          return {static_cast<const Child*>(this)->serialize(), hconf_metadata.duplicate()};
        }
        else
        {
          raw_data metadata;
          if (hconf_metadata.size > 0)
          {
            metadata = hconf_metadata.duplicate();
          }
          else
          {
            metadata = rle::serialize(rle::generate_metadata<Child>());
          }
          return {rle::serialize(*static_cast<const Child*>(this)), std::move(metadata)};
        }
      }

      void deserialize(const raw_data& new_data, const raw_data& new_metadata)
      {
        {
          std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
          hconf_metadata = new_metadata.duplicate();

          if constexpr (Child::hconf_has_custom_serialization)
          {
            static_cast<Child*>(this)->deserialize(new_data);
          }
          else
          {
            if (new_data.size > 0)
              rle::in_place_deserialize(new_data, *static_cast<Child*>(this));
          }
          is_being_initialized = false;
          is_initialized = true;
        }

        if constexpr (!Child::hconf_watch_source_file_change)
        {
          on_update_tk.release();
        }

        hconf_on_data_changed();
      }

    private:
      bool is_being_initialized = false;
      bool is_initialized = false;

    protected:
      raw_data hconf_metadata;

    private:
      cr::event_token_t on_update_tk;
      std::function<void(hconf&)> register_autoupdate; // used when moving conf objects

      friend class context;
  };

  /// \brief Generic conf, can be deserialized from any hconf asset, and be used for generic edition or packing, ...
  struct gen_conf : public hconf<gen_conf, "", location_t::none>
  {
    raw_data conf_data;

    static constexpr bool hconf_has_custom_serialization = true;
    raw_data serialize() const
    {
      return conf_data.duplicate();
    }
    void deserialize(const raw_data& new_data)
    {
      conf_data = new_data.duplicate();
    }

    void _set_conf_metadata(raw_data&& md)
    {
      hconf_metadata = std::move(md);
    }
  };
}

