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

    // the entry (be it embedded or in a pack file) is compressed
    // the entry _must_ have data (cannot be a virtual entry)
    compressed = 1 << 11,

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

      bool add_entry(id_t id, const entry& e, raw_data _data = {});

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

      entry get_entry(id_t id, unsigned max_depth = 5) const;

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

      static bool check_entry_consistency(const id_t id, const entry& e);

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
      void xor_and_load(const void* const raw_data_ptr, size_t data_size, bool* has_rejected_entries = nullptr);

      std::vector<uint32_t> xor_and_save() const;

    private:
      id_t index_id = id_t::invalid;
      std::unordered_map<id_t, entry> db;
      std::unordered_map<id_t, raw_data> embedded_data;

      mutable shared_spinlock lock;
  };
}
