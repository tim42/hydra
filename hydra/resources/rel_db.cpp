//
// created by : Timothée Feuillet
// date: 2022-4-30
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

#include "rel_db.hpp"

namespace neam::resources
{
  void rel_db::force_assign_registered_metadata_types()
  {
    metadata_types = get_metadata_type_map();
  }

  std::set<id_t> rel_db::get_pack_files(const std::string& file) const
  {
    std::set<id_t> ret;
    std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
    get_pack_files_unlocked(file, ret);
    return ret;
  }

  void rel_db::get_pack_files_unlocked(const std::string& file, std::set<id_t>& ret) const
  {
    if (auto it = files_resources.find(file); it != files_resources.end())
    {
      for (const auto& crit : it->second.child_resources)
      {
        if (auto rrit = root_resources.find(crit); rrit != root_resources.end())
        {
          ret.insert(rrit->second.pack_file);
        }
      }

      for (const auto& cfit : it->second.child_files)
        get_pack_files_unlocked(cfit, ret);
    }
  }

  std::set<id_t> rel_db::get_resources(const std::string& file, bool include_files_id) const
  {
    std::set<id_t> ret;
    std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
    get_resources_unlocked(file, ret, include_files_id);
    return ret;
  }

  std::set<id_t> rel_db::get_referenced_metadata_types(const std::string& file) const
  {
    std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
    if (auto it = files_resources.find(file); it != files_resources.end())
    {
      return it->second.referenced_metadata_types;
    }
    return {};
  }

  void rel_db::get_resources_unlocked(const std::string& file, std::set<id_t>& ret, bool include_files_id) const
  {
    if (include_files_id)
      ret.insert(string_id::_runtime_build_from_string(file.c_str(), file.size()));

    if (auto it = files_resources.find(file); it != files_resources.end())
    {
      for (const auto& crit : it->second.child_resources)
      {
        if (auto rrit = root_resources.find(crit); rrit != root_resources.end())
        {
          for (id_t srit : rrit->second.sub_resources)
          {
            ret.insert(srit);
          }
        }
      }

      for (const auto& cfit : it->second.child_files)
        get_resources_unlocked(cfit, ret, include_files_id);
    }
  }

  std::set<std::filesystem::path> rel_db::get_dependent_files(const std::filesystem::path& file) const
  {
    std::set<std::filesystem::path> ret;
    std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
    get_dependent_files_unlocked(file, ret);
    return ret;
  }

  void rel_db::get_dependent_files(const std::filesystem::path& file, std::set<std::filesystem::path>& ret) const
  {
    std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
    get_dependent_files_unlocked(file, ret);
  }

  void rel_db::get_dependent_files_unlocked(const std::filesystem::path& file, std::set<std::filesystem::path>& ret) const
  {
    if (auto it = files_resources.find(file); it != files_resources.end())
    {
      for (const auto& dit : it->second.dependent)
      {
        if (!ret.contains(dit))
        {
          ret.insert(dit);
          get_dependent_files_unlocked(dit, ret);
        }
      }
    }
  }

  void rel_db::consolidate_files_with_dependencies(std::set<std::filesystem::path>& file_list) const
  {
    const std::set<std::filesystem::path> initial_list = file_list;

    std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
    for (const auto& it : initial_list)
    {
      get_dependent_files_unlocked(it, file_list);
    }
  }


  std::set<std::filesystem::path> rel_db::get_removed_resources(const std::deque<std::filesystem::path>& file_list) const
  {
    std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));

    std::set<std::filesystem::path> ret;
    for (const auto& it : files_resources)
    {
      // only insert root files (actual FS files)
      if (it.second.parent_file.empty())
        ret.emplace_hint(ret.end(), it.first);
    }

    for (const auto& it : file_list)
      ret.erase(it);
    return ret;
  }

  std::set<std::filesystem::path> rel_db::get_absent_resources(const std::deque<std::filesystem::path>& file_list) const
  {
    std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));

    std::set<std::filesystem::path> ret;
    for (const auto& it : file_list)
    {
      if (!files_resources.contains(it))
        ret.emplace(it);
    }
    return ret;
  }

  std::set<std::filesystem::path> rel_db::get_files_requiring_reimport(const std::set<id_t>& processors, const std::set<id_t>& packers) const
  {
    std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));

    // first go over the files and check for processor changes
    std::set<std::filesystem::path> ret;
    for (const auto& it : files_resources)
    {
      if (!processors.contains(it.second.processor_hash) && it.second.processor_hash != id_t::none)
      // if (!processors.contains(it.second.processor_hash) || it.second.processor_hash == id_t::none)
      {
        // found a processor change
        ret.emplace(get_root_file_unlocked(it.first));
      }
    }

    // go over all the root resources and check for packer changes
    for (const auto& it : root_resources)
    {
      if (!packers.contains(it.second.packer_hash) || it.second.packer_hash == id_t::none)
      {
        // found a processor change (slow)
        ret.emplace(get_root_file_unlocked(it.first));
      }
    }

    return ret;
  }

  std::string rel_db::get_root_file_unlocked(const std::string& file) const
  {
    const std::string* it = &file;

    while (true)
    {
      if (auto fit = files_resources.find(*it); fit != files_resources.end())
      {
        if (fit->second.parent_file.empty())
          break;

        it = &fit->second.parent_file;
        continue;
      }
      break;
    }
    return *it;
  }

  std::string rel_db::get_root_file_unlocked(id_t root_res) const
  {
    for (const auto& it : files_resources)
    {
      if (it.second.child_resources.contains(root_res))
        return get_root_file_unlocked(it.first);
    }
    return {};
  }

  raw_data rel_db::serialize() const
  {
    std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
    return rle::serialize(*this);
  }


  void rel_db::add_file(const std::string& file)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    if (auto it = files_resources.find(file); it != files_resources.end())
      return repack_file_unlocked(file);
    files_resources.emplace(file, file_info_t{});
  }

  void rel_db::add_file(const std::string& parent_file, const std::string& child_file)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    files_resources[parent_file].child_files.insert(child_file);
    if (auto it = files_resources.find(child_file); it != files_resources.end())
    {
      it->second.parent_file = parent_file;
      repack_file_unlocked(child_file);
      return;
    }
    files_resources.emplace(child_file, file_info_t{.parent_file = parent_file});
  }

  void rel_db::add_file_to_file_dependency(const std::string& file, const std::string& dependent_on)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    files_resources[file].depend_on.insert(dependent_on);
    files_resources[dependent_on].dependent.insert(file);
  }

  void rel_db::set_processor_for_file(const std::string& file, id_t version_hash)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    files_resources[file].processor_hash = version_hash;
  }

  // root-resource / pack-file
  void rel_db::add_resource(const std::string& parent_file, id_t root_resource)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    files_resources[parent_file].child_resources.insert(root_resource);
    root_resources.insert_or_assign(root_resource, root_resource_info_t{ .parent_file = parent_file });
  }

  void rel_db::add_resource(id_t root_resource, id_t child_resource)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    root_resources[root_resource].sub_resources.insert(child_resource);
    sub_resources.emplace(child_resource, root_resource);
  }

  void rel_db::set_pack_file(id_t root_resource, id_t pack_file_id)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    root_resources[root_resource].pack_file = pack_file_id;
  }

  void rel_db::set_packer_for_resource(id_t root_resource, id_t packer_hash)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    root_resources[root_resource].packer_hash = packer_hash;
  }

  void rel_db::remove_file(const std::string& file)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    remove_file_unlocked(file);
  }

  void rel_db::remove_file_unlocked(const std::string& file)
  {
    if (auto it = files_resources.find(file); it != files_resources.end())
    {
      // we copy the entry
      const file_info_t entry = std::move(it->second);

      // then remove it:
      files_resources.erase(it);

      // remove all the depndencies:
      for (auto& it : entry.dependent)
      {
        if (auto fit = files_resources.find(it); fit != files_resources.end())
          fit->second.depend_on.erase(it);
      }
      for (auto& it : entry.depend_on)
      {
        if (auto fit = files_resources.find(it); fit != files_resources.end())
          fit->second.dependent.erase(it);
      }

      // we recursively call remove_file on all sub-files:
      // (which is why we removed ourselves first, to avoid potential infinite recursion
      for (const auto& cfit : entry.child_files)
        remove_file_unlocked(cfit);

      for (auto crit : entry.child_resources)
        remove_resource_unlocked(crit);
    }
  }

  void rel_db::repack_file(const std::string& file)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    repack_file_unlocked(file);
  }

  void rel_db::repack_file_unlocked(const std::string& file)
  {
    if (auto it = files_resources.find(file); it != files_resources.end())
    {
      // we grab a ref to the entry
      file_info_t& entry = it->second;

      entry.referenced_metadata_types.clear();

      // remove the depndencies: (only file -> other files)
      for (auto& it : entry.depend_on)
      {
        if (auto fit = files_resources.find(it); fit != files_resources.end())
          fit->second.dependent.erase(it);
      }

      // we recursively call repack_file_unlocked on all sub-files:
      // (which is why we removed ourselves first, to avoid potential infinite recursion
      for (const auto& cfit : entry.child_files)
        repack_file_unlocked(cfit);

      for (auto crit : entry.child_resources)
        remove_resource_unlocked(crit);
    }
  }

  void rel_db::remove_resource_unlocked(id_t root_resource)
  {
    resources_names.erase(root_resource);
    resources_messages.erase(root_resource);

    if (auto it = root_resources.find(root_resource); it != root_resources.end())
    {
      const root_resource_info_t entry = std::move(it->second);
      root_resources.erase(it);

      for (auto srit : entry.sub_resources)
      {
        resources_names.erase(srit);
        resources_messages.erase(srit);
        sub_resources.erase(srit);
      }
    }
  }

  void rel_db::reference_metadata_type(const std::string& file, id_t metadata_type)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    return reference_metadata_type_unlocked(file, metadata_type);
  }

  void rel_db::reference_metadata_type(id_t root_resource, id_t metadata_type)
  {
    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    return reference_metadata_type_unlocked(root_resource, metadata_type);
  }

  void rel_db::reference_metadata_type_unlocked(const std::string& file, id_t metadata_type)
  {
    if (auto it = files_resources.find(file); it != files_resources.end())
    {
      it->second.referenced_metadata_types.insert(metadata_type);

      // propagate the change to parent files (FIXME: is it needed??)
      // if (!it->second.parent_file.empty())
        // return reference_metadata_type_unlocked(it->second.parent_file, metadata_type);
    }
  }

  void rel_db::reference_metadata_type_unlocked(id_t root_resource, id_t metadata_type)
  {
    if (auto it = root_resources.find(root_resource); it != root_resources.end())
    {
      return reference_metadata_type_unlocked(it->second.parent_file, metadata_type);
    }
  }

  rel_db::message_list_t rel_db::get_messages(id_t rid) const
  {
    std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
    if (auto it = resources_messages.find(rid); it != resources_messages.end())
      return it->second;
    return {};
  }

  metadata_type_registration_t rel_db::get_type_metadata(id_t type_id) const
  {
    if (auto it = metadata_types.find(type_id); it != metadata_types.end())
    {
      return it->second;
    }
    return {};
  }

  std::string rel_db::resource_name(id_t rid) const
  {
    {
      std::lock_guard _sl(spinlock_shared_adapter::adapt(lock));
      if (auto it = resources_names.find(rid); it != resources_names.end())
        return it->second;
    }
    return fmt::format("{}", rid);
  }

  void rel_db::resource_name(id_t rid, std::string name)
  {
    std::lock_guard _sl(spinlock_exclusive_adapter::adapt(lock));
    resources_names[rid] = std::move(name);
  }

  void rel_db::log_str(cr::logger::severity s, id_t res, std::string provider, std::string str)
  {
    cr::out().log_fmt(s, "{}: {}: {}", provider, resource_name(res), str);

    std::lock_guard _el(spinlock_exclusive_adapter::adapt(lock));
    resources_messages[res].list.push_back(
    {
     .severity = s,
     .source = provider,
     .message = str,
    });
  }
}

