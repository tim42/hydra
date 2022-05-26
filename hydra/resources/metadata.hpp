//
// created by : Timothée Feuillet
// date: 2022-4-28
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

#include <ntools/id/id.hpp>
#include <ntools/id/string_id.hpp>
#include <ntools/rle/rle.hpp>
#include <ntools/raw_data.hpp>


namespace neam::resources
{
  /// \brief Serializable struct that holds metadata information about a resource
  /// \note Used by packers and processors, but is not part of the packed data
  struct metadata_t
  {
    static metadata_t deserialize(const raw_data& rd);

    /// \brief Retrieve a stored type in the metadata map
    template<typename T>
    bool try_get(id_t id, T& out) const
    {
      if (auto it = data.find(id); it != data.end())
      {
        if constexpr (rle::concepts::SerializableStruct<T>)
        {
          return (rle::in_place_deserialize<T>(it->second, out) == rle::status::success);
        }
        else
        {
          rle::status st = rle::status::success;
          out = rle::deserialize<T>(it->second, st);
          return st == rle::status::success;
        }
      }
      return false;
    }

    /// \brief Retrieve a stored type metadata in the metadata map
    bool try_get_serialization_metadata(id_t id, rle::serialization_metadata& out) const
    {
      if (auto it = serialization_metadata.find(id); it != serialization_metadata.end())
      {
        out = it->second;
        return true;
      }
      return false;
    }

    /// \brief Stores a type and its metadata in the metadata map
    template<typename T>
    bool set(id_t id, const T& in)
    {
      rle::status st = rle::status::success;
      raw_data d = rle::serialize(in, &st);
      if (st != rle::status::success)
        return false;

      data.emplace(id, std::move(d));
      serialization_metadata.emplace(id, rle::generate_metadata<T>());
      return true;
    }

    /// \brief Check whether the type is contained in the metadata map
    bool contains(id_t id) const { return data.contains(id); }

    /// \brief Removes an element from the metadata maps
    void erase(id_t id)
    {
      data.erase(id);
      serialization_metadata.erase(id);
    }

    /// \brief insert o in the current metadata, overriding existing values.
    void add_overrides(metadata_t o)
    {
      o.data.merge(std::move(data));
      o.data.swap(data);
      o.serialization_metadata.merge(std::move(serialization_metadata));
      o.serialization_metadata.swap(serialization_metadata);
    }

    /// \brief Return whether the metadata is readonly or will be saved back.
    /// \note changes to a readonly metadata will not be saved.
    bool is_readonly() const { return file_id == id_t::invalid; }

    /// \brief returns whether the metadata is empty or not
    bool empty() const { return data.empty(); }


    // the data
    // (split in two maps so if there's ever a human readable format for rle, the type metadata is at the bottom of the file)
    std::map<id_t, raw_data> data = {};
    std::map<id_t, rle::serialization_metadata> serialization_metadata = {};

    // non serialized members:
    id_t file_id = id_t::invalid; // to save the metadata back. Is id_t::invalid for read-only metadata. (must be returned by io::context)
    id_t initial_hash = id_t::none;
  };
}

N_METADATA_STRUCT(neam::resources::metadata_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(data),
    N_MEMBER_DEF(serialization_metadata)
  >;
};

namespace neam::resources
{
  inline metadata_t metadata_t::deserialize(const raw_data& rd)
  {
    metadata_t ret;
    if (rle::in_place_deserialize<metadata_t>(rd, ret) == rle::status::success)
    {
      ret.initial_hash = (id_t)ct::hash::fnv1a<64>((const uint8_t*)rd.data.get(), rd.size);
    }
    return ret;
  }
}
