//
// created by : Timothée Feuillet
// date: 2023-6-23
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

#include "index.hpp"

namespace neam::resources
{
  bool index::add_entry(id_t id, const entry& e, raw_data _data)
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

  index::entry index::get_entry(id_t id, unsigned max_depth) const
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

  bool index::check_entry_consistency(const id_t id, const entry& e)
  {
#if N_HYDRA_RESOURCES_STRIP_DEBUG
#define N_HYDRA_RESOURCES_CHECK_FAIL(...) {return false;}
#else
#define N_HYDRA_RESOURCES_CHECK_FAIL(...) { neam::cr::out().error(__VA_ARGS__); return false;}
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
    const bool is_compressed = (e.flags & flags::compressed) != flags::none;

    if (is_embedded && is_plain_file)
      N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: entry cannot be both embedded and plain file")
    if (is_embedded && !is_data)
      N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: entry cannot be embedded and not a data entry")
    if (is_embedded && e.pack_file != id_t::none)
      N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: entry is embedded yet has a pack_file set to something other than none")

    if (is_virtual && is_compressed)
      N_HYDRA_RESOURCES_CHECK_FAIL("inconsistent resource: entry is both compressed and virtual")

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


  // The xored data format is a bit weird, as there is no way to know if we're decoding bad data. It's simply the entries as-is.
  // The goal is to obfuscate and makes it a pain to load without tracing the binary first
  //  or spending time doing a statistical analysis of the file to find a possible initial key.
  // The goal is *not* to make it impossible to decode but to make it difficult.
  void index::xor_and_load(const void* const raw_data_ptr, size_t data_size, bool* has_rejected_entries)
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
          ++index; // move the the next entry
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
          ++index; // index is now the size marker
          const uint32_t embedded_data_size = decode(u32_data[index]);
          const uint32_t u32_embedded_data_size = (embedded_data_size + 3) / 4;
          ++index; // index is now the first byte of the embedded_data or the first byte of the next entry


          if (index + u32_embedded_data_size > u32_data_size)
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
            raw_data rd = raw_data::allocate(u32_embedded_data_size * 4);
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

  std::vector<uint32_t> index::xor_and_save() const
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

    // using this to have stable order:
    std::vector<std::pair<id_t, entry>> sorted_db;
    for (const auto& it : db)
    {
      sorted_db.emplace_back(it);
    }
    std::sort(sorted_db.begin(), sorted_db.end(), [](const auto& a, const auto& b)
    {
      return a.first < b.first;
    });


    uint64_t key = (uint64_t)index_id;
    uint64_t accumulator = (uint64_t)combine((id_t)"caca"_rid, (sorted_db.empty() ? id_t::none : sorted_db.back().first));

    const auto encode = [&key, &accumulator](uint32_t x)
    {
#if !N_HYDRA_RESOURCES_OBFUSCATE
      return x;
#else
      key = neam::ct::invwk_rnd(key);
      accumulator += x * x | 5;
      accumulator = ct::invwk_rnd(accumulator);
      return x ^(uint32_t)(key >> 32);
#endif
    };

    // used to scramble the flags:
    uint64_t scramble_key = neam::ct::invwk_rnd((uint64_t)(accumulator) << 32 | key >> 32);

    for (const auto& it : sorted_db)
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
        entry.current.id = (id_t)((scramble_key & 0xFFFFFFFF00000000ul) | accumulator >> 32);
        scramble_key = neam::ct::invwk_rnd(scramble_key) << 32 | (accumulator & 0xFFFFFFFFul);
        entry.current.flags = flags::key_change | ((flags)(scramble_key) & flags::crap_mask);

        for (size_t i = 0; i < u32_data_for_key_change; ++i)
          data.push_back(encode(entry.u32[i]));

        key = ((uint64_t)entry.current.id) << 32 | key >> 32;
      }
      scramble_key = neam::ct::invwk_rnd(scramble_key);
#endif

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
          const size_t embedded_data_size = (data_it->second.size) / 4;
          const size_t rem_data_size = (data_it->second.size) % 4;

          data.reserve(embedded_data_size + 1);

          for (size_t j = 0; j < embedded_data_size; ++j)
            data.push_back(encode(embedded_data_ptr[j]));
          if (rem_data_size > 0)
          {
            // Avoid writing garbage on the out-of-bound data
            // (accessing up to 3 byte on a 8 byte aligned memory is _fine_, we just have to make sure to never use that memory)
            const uint32_t extra_data = embedded_data_ptr[embedded_data_size] & ((~0u) >> (32 - rem_data_size * 8));
            data.push_back(encode(extra_data));
          }
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
}

