//
// created by : Timothée Feuillet
// date: 2021-11-25
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

#pragma once

#include <cstdint>
#include <utility>
#include <unordered_map>
#include <random>

#include <ntools/logger/logger.hpp>
#include <ntools/rng.hpp>
#include <ntools/id/id.hpp>
#include <ntools/id/string_id.hpp>

#include <ntools/io/context.hpp>

#include "enums.hpp"

#ifndef N_HYDRA_RESOURCES_OBFUSCATE
  #error N_HYDRA_RESOURCES_OBFUSCATE not defined
#endif
namespace neam::resources
{
  /// \brief Some flags for the resource at index:
  enum class flags : uint64_t
  {
    none = 0,

    // Describe the resource type:

    // default resource type: data
    type_data = 1,

    // pack_file is a id_t and should be used to look-up the real resource entry
    type_simlink = 3,
    // resource is this index. outside id and flags, the data contained in the entry may have its meaning altered.
    type_virtual = 4,

    // When xoring, inserted to say that it's time to change key (id is the new key)
    // NOTE: key change also means that both offset and size are part of the next entry. It's a 2 field entry and not a 5 field one.
    key_change = 127,

    // mask for the resource type. Does not describe a resource type.
    type_mask = 0xFF,

    // invalid resource type
    type_invalid = type_mask,

    // flags are here: (remove them from the crap mask or you'll have surprises)

    // the resource is a standalone file (size is ignored when reading the resource).
    // (if unset, it's assumed to be a packed file)
    standalone_file = 1 << 8,
    // data is embedded in the index itself.
    // It is an error to have standalone_file|embedded_data or a type other than type_data.
    // pack-file must be none. Offset is ignored (must be 0) and size will be set automatically
    embedded_data = 1 << 9,

    // the entry is to be stripped from the index before release (editor-only entries, debug data, ...)
    to_strip = 1 << 10,

    // mask used to store random data in this field
    // NOTE: Modifying the crap-mask means a full rebuild of all indexes (as previously random bits become meaningful)
    crap_mask = 0xFFFFFFFFFFFF0000,
  };

  N_ENUM_UNARY_OPERATOR(flags, ~)
  N_ENUM_BINARY_OPERATOR(flags, |)
  N_ENUM_BINARY_OPERATOR(flags, &)

  /// \brief Simple repository of res id -> pack file hash | offset | size
  /// \note the index can be split into multiple chunks and additively loaded
  /// \note the index does not handle pack files / resource files. It simple manages the index.
  class index
  {
    public:
      struct entry
      {
        id_t id = id_t::invalid;
        enum flags flags = flags::type_invalid;

        id_t pack_file = id_t::none;
        uint64_t offset = 0;
        uint64_t size = 0;

        bool is_valid() const { return index::check_entry_consistency(*this); }
      };


      explicit index(id_t id) : index_id(id) {}
      index() = default;
      ~index() = default;
      index(index&& o) : index_id(o.index_id), db(std::move(o.db)), embedded_data(std::move(o.embedded_data)) {}
//       index(const index& o) : index_id(o.index_id), db(o.db), embedded_data(o.embedded_data) {}
      index& operator = (index&& o)
      {
        if (&o == this) return *this;
        std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
        index_id = (o.index_id);
        db = (std::move(o.db));
        embedded_data = (std::move(o.embedded_data));
        return *this;
      }
//       index& operator = (const index& o)
//       {
//         if (&o == this) return *this;
//         std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
//         std::lock_guard _sl(spinlock_shared_adapter::adapt(o.lock));
//         index_id = (o.index_id);
//         db = (o.db);
//         embedded_data = (o.embedded_data);
//         return *this;
//       }

      id_t get_index_id() const { return index_id; }
      void set_index_id(id_t id) { index_id = id; }

      void clear() { *this = index(); }

      bool add_entry(const entry& e, raw_data _data = {})
      {
        return add_entry(e.id, e, std::move(_data));
      }

      bool add_entry(id_t id, const entry& e, raw_data _data = {})
      {
        if (!check_entry_consistency(id, e))
        {
#if !N_HYDRA_RESOURCES_STRIP_DEBUG
          neam::cr::out().warn("resources::index::add_entry(): rejecting resource {:X}: resource is not consistent", std::to_underlying(id));
#endif
          return false;
        }

        std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
        if ((e.flags & flags::embedded_data) != flags::none)
        {
          // Will not replace an existing entry, only create one if it does not already exists
          embedded_data.emplace(id, std::move(_data));
        }
        else
        {
          check::debug::n_check(_data.size == 0, "Specifying an embedded data for a resource without the flag");
          // we unset the flag, so we must remove the embedded data
          embedded_data.erase(id);
        }

        db.insert_or_assign(id, e);
        return true;
      }

      void remove_entry(id_t id)
      {
        std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
        db.erase(id);
        embedded_data.erase(id);
      }

      bool has_entry(id_t id) const
      {
        std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
        return db.contains(id);
      }

      entry get_raw_entry(id_t id) const
      {
        std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
        if (const auto it = db.find(id); it != db.end())
        {
          return it->second;
        }
        return {};
      }

      entry get_entry(id_t id, unsigned max_depth = 5) const
      {
        std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
        if (const auto it = db.find(id); it != db.end())
        {
          if ((it->second.flags & flags::type_mask) != flags::type_simlink)
          {
            return it->second;
          }
          else
          {
            if (max_depth == 0)
              return {};

            // pack_file contains the resource id to target
            return get_entry(it->second.pack_file, max_depth - 1);
          }
        }
        return {};
      }

      raw_data* get_embedded_data(id_t id)
      {
        std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
        if (auto it = embedded_data.find(id); it != embedded_data.end())
          return &it->second;
        return nullptr;
      }
      const raw_data* get_embedded_data(id_t id) const
      {
        std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
        if (auto it = embedded_data.find(id); it != embedded_data.end())
          return &it->second;
        return nullptr;
      }

      bool set_embedded_data(id_t id, raw_data&& rd)
      {
        std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
        if (auto it = embedded_data.find(id); it != embedded_data.end())
        {
          it->second = std::move(rd);
          return true;
        }
        return false;
      }

      bool has_embedded_data(id_t id) const
      {
        std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
        return (embedded_data.find(id) != embedded_data.end());
      }

      void add_index(index&& o)
      {
        std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
        auto temp_db = std::move(db);
        db = std::move(o.db);
        // FIXME: this may be slower than a loop over o.db if the size of the db is big
        db.merge(temp_db);
      }

      void add_index(const index& o)
      {
        std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
        std::lock_guard _oel(spinlock_exclusive_adapter::adapt(o.lock));
        auto temp_db = std::move(db);
        db = o.db;
        // FIXME: this may be slower than a loop over o.db if the size of the db is big
        db.merge(temp_db);
      }

      size_t entry_count() const { return db.size(); }

      template<typename Fnc>
      void for_each_entry(Fnc&& fnc) const
      {
        std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
        for (const auto& it : db)
        {
          fnc(it.second);
        }
      }

      static bool check_entry_consistency(const entry& e)
      {
        return check_entry_consistency(e.id, e);
      }

      static bool check_entry_consistency(const id_t id, const entry& e)
      {
#if N_HYDRA_RESOURCES_STRIP_DEBUG
  #define N_HYDRA_RESOURCES_CHECK_FAIL(...) {return false;}
#else
  #define N_HYDRA_RESOURCES_CHECK_FAIL(...) { neam::cr::out().debug(__VA_ARGS__); return false;}
#endif

        // Basic check:
        if (id != e.id)
          N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: id != e.id")
        if (id == id_t::invalid || id == id_t::none)
          N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: id is invalid or none")

        // Check for consistent entry type:
        const flags masked_flags = (e.flags & flags::type_mask);
        if (masked_flags == flags::type_invalid || masked_flags == flags::none || masked_flags == flags::key_change)
          N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: flags cannot be invalid, none or key_change")

        const bool is_data = masked_flags == flags::type_data;
        const bool is_simlink = masked_flags == flags::type_simlink;
        const bool is_virtual = masked_flags == flags::type_virtual;
        const bool is_plain_file = (e.flags & flags::standalone_file) != flags::none;
        const bool is_embedded = (e.flags & flags::embedded_data) != flags::none;

        if (is_embedded && is_plain_file)
          N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: entry cannot be both embedded and plain file")
        if (is_embedded && !is_data)
          N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: entry cannot be embedded and not a data entry")
        if (is_embedded && e.pack_file != id_t::none)
          N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: entry is embedded yet has a pack_file set to something other than none")

        // virtual entry validation stops here
        if (is_virtual)
          return true;

        // size/offset validation:
        // (simlink and plain-files must have 0 offsets and size as it's assumed to change)
        if ((!is_simlink && !is_plain_file && !is_embedded) && e.size == 0)
          N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: entry has a size of 0, which should not be possible for its type")
        if ((is_simlink || is_plain_file || is_embedded) && e.offset != 0 && e.size != 0)
          N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: entry of this type should have a size of 0")

        constexpr size_t max_size = 200ul * 1024ul * 1024ul * 1024ul; // GB
        if (e.size >= max_size || e.offset >= max_size || e.size + e.offset >= max_size)
          N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: entry size or offset exceeds max size")
#undef N_HYDRA_RESOURCES_CHECK_FAIL
        return true;
      }

    public: // de/serialization:

      /// \brief Create and populate an index from a set of data
      /// \note There is no failure, as there is no data check. You'll get a corrupted index
      ///       if the data are not correct / the index_id is not correct
      static index read_index(id_t index_id, const void* raw_data, size_t size, bool* has_rejected_entries = nullptr)
      {
        index idx(index_id);
        // FIXME: add header / version check here ?
        idx.xor_and_load(raw_data, size, has_rejected_entries);
        return idx;
      }

      static index read_index(id_t index_id, const raw_data& data, bool* has_rejected_entries = nullptr)
      {
        return read_index(index_id, data.data.get(), data.size, has_rejected_entries);
      }

      /// \brief serialize the data contained in the index
      raw_data serialize_index() const
      {
        std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));

        std::vector<uint32_t> data = xor_and_save();
        raw_data ret = raw_data::allocate(data.size() * sizeof(uint32_t));
        memcpy(ret.data.get(), data.data(), ret.size);
        return ret;
      }

    private:
      // The xored data format is a bit weird, as there is no way to know if we're decoding bad data. It's simply the entries as-is.
      // The goal is to obfuscate and makes it a pain to load without tracing the binary first
      //  or spending time doing a statistical analysis of the file to find a possible initial key.
      // The goal is *not* to make it impossible to decode but to make it difficult.
      void xor_and_load(const void* const raw_data_ptr, size_t data_size, bool* has_rejected_entries = nullptr)
      {
        if (has_rejected_entries != nullptr)
          *has_rejected_entries = false;
        // We don't xor with the full key, as the lower 32 bits have low entropy. We only use the upper 32 bits instead.
        uint64_t key = (uint64_t)index_id;

        const uint32_t* const u32_data = reinterpret_cast<const uint32_t*>(raw_data_ptr);
        const size_t u32_data_size = data_size / sizeof(uint32_t);


        constexpr size_t entry_size = sizeof(entry);
        static_assert(entry_size % sizeof(uint32_t) == 0, "index::entry has a size which is not 0 mod 32bit");
        constexpr size_t u32_per_entry = entry_size / sizeof(uint32_t);

        constexpr size_t data_for_key_change = offsetof(entry, flags) + sizeof(entry::flags);
        static_assert(data_for_key_change % sizeof(uint32_t) == 0, "index::entry::flags has a offset+size which is not 0 mod 32bit");
        constexpr size_t u32_data_for_key_change = data_for_key_change / sizeof(uint32_t);

        const auto decode = [&key](uint32_t x)
        {
#if !N_HYDRA_RESOURCES_OBFUSCATE
          return x;
#else
          key = neam::ct::invwk_rnd(key);
          return x ^(uint32_t)(key >> 32);
#endif
        };

        // rough upper estimate
        db.reserve(u32_data_size / u32_per_entry);

        for (size_t index = 0; index + u32_per_entry <= u32_data_size;)
        {
          bool key_change = false;
          union
          {
            // just there for the compiler to not complain about not being able to construct the thing
            uint32_t _init_;

            uint32_t u32[u32_per_entry];
            entry current;
          } entry { 0 };

          for (size_t i = 0; i < u32_per_entry; ++i, ++index)
          {
            entry.u32[i] = decode(u32_data[index]);

#if N_HYDRA_RESOURCES_OBFUSCATE
            // Handle key changes:
            if (i == (u32_data_for_key_change - 1) &&
                ((entry.current.flags & flags::type_mask) == flags::key_change))
            {
              ++index;
              key = ((uint64_t)entry.current.id) << 32 | key >> 32;
              key_change = true;
              break;
            }
#endif
            // handle embedded data:
            if (i == (u32_data_for_key_change - 1) &&
                ((entry.current.flags & flags::embedded_data) != flags::none))
            {
              // decode the data:
              ++index;
              const uint32_t embedded_data_size = decode(u32_data[index]);
              const uint32_t u32_embedded_data_size = (embedded_data_size + 3) / 4;
              ++index;

              if (index + u32_embedded_data_size >= u32_data_size)
              {
#if !N_HYDRA_RESOURCES_STRIP_DEBUG
                neam::cr::out().warn("resources::index::xor_and_load(): rejecting resource {:X}: embedded data is out of bounds", std::to_underlying(entry.current.id));
#endif
                if (has_rejected_entries != nullptr)
                  *has_rejected_entries = true;
                break;
              }

              if (embedded_data_size > 0)
              {
                const uint32_t allocated_size = embedded_data_size + (4 - embedded_data_size % 4);
                raw_data rd = raw_data::allocate(allocated_size);
                rd.size = embedded_data_size;
#if !N_HYDRA_RESOURCES_OBFUSCATE
                memcpy(rd.data.get(), u32_data + index, embedded_data_size);
                index += u32_embedded_data_size;
#else
                uint32_t* embedded_data_ptr = reinterpret_cast<uint32_t*>(rd.data.get());
                for (unsigned embedded_data_index = 0; embedded_data_index < u32_embedded_data_size; ++embedded_data_index)
                {
                  embedded_data_ptr[embedded_data_index] = decode(u32_data[index]);
                  ++index;
                }
#endif
                embedded_data.emplace(entry.current.id, std::move(rd));
              }
              else
              {
                embedded_data.emplace(entry.current.id, raw_data{});
              }

              // we want to insert the data (and have only one code path for that:
              break;
            }
          }

          // key change mean a restart of the process
          if (key_change)
            continue;

          // uncrappify the flags
          entry.current.flags = entry.current.flags & ~flags::crap_mask;

          // insert the entry in the db
          if (!add_entry(entry.current) && has_rejected_entries != nullptr)
            *has_rejected_entries = true;
        }

        db.rehash(db.size());
      }

      std::vector<uint32_t> xor_and_save() const
      {
        std::vector<uint32_t> data;

        constexpr size_t entry_size = sizeof(entry);
        static_assert(entry_size % sizeof(uint32_t) == 0, "index::entry has a size which is not 0 mod 32bit");
        constexpr size_t u32_per_entry = entry_size / sizeof(uint32_t);

        constexpr size_t data_for_key_change = offsetof(entry, flags) + sizeof(entry::flags);
        static_assert(data_for_key_change % sizeof(uint32_t) == 0, "index::entry::flags has a offset+size which is not 0 mod 32bit");
        constexpr size_t u32_data_for_key_change = data_for_key_change / sizeof(uint32_t);

        // very rough upper estimate
        data.reserve(db.size() * u32_per_entry);

        uint64_t key = (uint64_t)index_id;

        const auto encode = [&key](uint32_t x)
        {
#if !N_HYDRA_RESOURCES_OBFUSCATE
          return x;
#else
          key = neam::ct::invwk_rnd(key);
          return x ^(uint32_t)(key >> 32);
#endif
        };

        std::random_device rdev;

        // used to scramble the flags:
        uint64_t scramble_key = neam::ct::invwk_rnd((uint64_t)(rdev()) << 32 | key >> 32);

        for (const auto& it : db)
        {
          if (!check_entry_consistency(it.first, it.second))
          {
#if !N_HYDRA_RESOURCES_STRIP_DEBUG
            neam::cr::out().warn("resources::index: skipping resource {:X}: resource is not consistent", std::to_underlying(it.first));
#endif
            continue;
          }

          scramble_key = neam::ct::invwk_rnd(scramble_key);

          union
          {
            // just there for the compiler to not complain about not being able to construct the thing
            uint32_t _init_;

            uint32_t u32[u32_per_entry];
            entry current;
          } entry { 0 };

          // key change. The higher the mask, the lower the memory / higher the speed,
          //             but also the lower the data scrambling capabilities
#if N_HYDRA_RESOURCES_OBFUSCATE
          if (((scramble_key >> 32) & 0x3F) == 0)
          {
            entry.current.id = (id_t)((scramble_key & 0xFFFFFFFF00000000ul) | rdev());
            scramble_key = neam::ct::invwk_rnd(scramble_key) << 32 | rdev();
            entry.current.flags = flags::key_change | ((flags)(scramble_key) & flags::crap_mask);

            for (size_t i = 0; i < u32_data_for_key_change; ++i)
              data.push_back(encode(entry.u32[i]));

            key = ((uint64_t)entry.current.id) << 32 | key >> 32;
          }
#endif
          scramble_key = neam::ct::invwk_rnd(scramble_key);

          // write entry:
          entry.current = it.second;
#if N_HYDRA_RESOURCES_OBFUSCATE
          entry.current.flags = entry.current.flags | ((flags)(scramble_key) & flags::crap_mask);
#endif

          if ((entry.current.flags & flags::embedded_data) != flags::none)
          {
            // write id, flags (as would a key change) then data size and insert the raw data
            for (size_t i = 0; i < u32_data_for_key_change; ++i)
              data.push_back(encode(entry.u32[i]));

            auto data_it = embedded_data.find(entry.current.id);
            if (data_it != embedded_data.end() && data_it->second.size > 0)
            {
              data.push_back(encode((uint32_t)data_it->second.size));

              const uint32_t* embedded_data_ptr = (const uint32_t*)data_it->second.data.get();
              const size_t embedded_data_size = (data_it->second.size + 3) / 4;

              for (size_t j = 0; j < embedded_data_size; ++j)
                data.push_back(encode(embedded_data_ptr[j]));
            }
            else
            {
              // no data, simply write 0 and continue:
              data.push_back(encode(0));
            }
          }
          else
          {
            // normal way of writing the data:
            for (size_t i = 0; i < u32_per_entry; ++i)
              data.push_back(encode(entry.u32[i]));
          }
        }

        return data;
      }

    private:
      id_t index_id = id_t::invalid;
      std::unordered_map<id_t, entry> db;
      std::unordered_map<id_t, raw_data> embedded_data;

      mutable shared_spinlock lock;
  };
}
