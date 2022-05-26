

#include <unordered_map>
#include <iostream>

#include <ntools/logger/logger.hpp>

#include <ntools/id/id.hpp>
#include "mimetype/mimetype.hpp"
#include "processor.hpp"

namespace neam::resources::processor
{
  struct processor_map_entry_t
  {
    function fnc;
    id_t hash;
  };

  static std::unordered_map<id_t, processor_map_entry_t>& get_processor_map()
  {
    static std::unordered_map<id_t, processor_map_entry_t> map;
    return map;
  }

  bool register_processor(id_t name_id, id_t version_hash, function processor)
  {
    get_processor_map().insert_or_assign(name_id, processor_map_entry_t{processor, version_hash});
    return true;
  }

  bool unregister_processor(id_t type_id)
  {
    get_processor_map().erase(type_id);
    return true;
  }

  function get_processor(const raw_data& data, id_t file_extension)
  {
    const id_t mime = mime::get_mimetype_id(data);
    if (auto it = get_processor_map().find(mime); it != get_processor_map().end())
      return it->second.fnc;
    if (auto it = get_processor_map().find(file_extension); it != get_processor_map().end())
      return it->second.fnc;
    return nullptr;
  }
  id_t get_processor_hash(const raw_data& data, id_t file_extension)
  {
    const id_t mime = mime::get_mimetype_id(data);
    if (auto it = get_processor_map().find(mime); it != get_processor_map().end())
      return it->second.hash;
    if (auto it = get_processor_map().find(file_extension); it != get_processor_map().end())
      return it->second.hash;
    return id_t::invalid;
  }

  function get_processor(const raw_data& data, const std::filesystem::path& p)
  {
    const std::string ext = p.extension();
    return get_processor(data, string_id::_runtime_build_from_string("file-ext:"_rid, ext.c_str(), ext.size()));
  }
  id_t get_processor_hash(const raw_data& data, const std::filesystem::path& p)
  {
    const std::string ext = p.extension();
    return get_processor_hash(data, string_id::_runtime_build_from_string("file-ext:"_rid, ext.c_str(), ext.size()));
  }

  function _get_processor(id_t id)
  {
    if (auto it = get_processor_map().find(id); it != get_processor_map().end())
      return it->second.fnc;
    return nullptr;
  }
  id_t _get_processor_hash(id_t id)
  {
    if (auto it = get_processor_map().find(id); it != get_processor_map().end())
      return it->second.hash;
    return id_t::invalid;
  }
}
