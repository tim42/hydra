
#include <unordered_map>
#include <iostream>

#include <ntools/logger/logger.hpp>

#include <ntools/id/id.hpp>
#include "packer.hpp"

namespace neam::resources::packer
{
  struct packer_map_entry_t
  {
    function fnc;
    id_t hash;
  };

  static std::unordered_map<id_t, packer_map_entry_t>& get_packer_map()
  {
    static std::unordered_map<id_t, packer_map_entry_t> map;
    return map;
  }

  bool register_packer(id_t type_id, id_t version_hash, function pack_resource)
  {
    get_packer_map().insert_or_assign(type_id, packer_map_entry_t{pack_resource, version_hash});
    return true;
  }

  bool unregister_packer(id_t type_id)
  {
    get_packer_map().erase(type_id);
    return true;
  }

  function get_packer(id_t type_id)
  {
    if (auto it = get_packer_map().find(type_id); it != get_packer_map().end())
      return it->second.fnc;
    return nullptr;
  }
  id_t get_packer_hash(id_t type_id)
  {
    if (auto it = get_packer_map().find(type_id); it != get_packer_map().end())
      return it->second.hash;
    return id_t::invalid;
  }
}
