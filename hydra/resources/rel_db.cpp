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
  std::set<id_t> rel_db::get_pack_files(const std::string& file) const
  {
    std::set<id_t> ret;
    std::lock_guard _l(lock);
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

  std::set<id_t> rel_db::get_resources(const std::string& file) const
  {
    std::set<id_t> ret;
    std::lock_guard _l(lock);
    get_resources_unlocked(file, ret);
    return ret;
  }

  void rel_db::get_resources_unlocked(const std::string& file, std::set<id_t>& ret) const
  {
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
        get_resources_unlocked(cfit, ret);
    }
  }

  std::set<std::filesystem::path> rel_db::get_dependent_files(const std::filesystem::path& file) const
  {
    std::set<std::filesystem::path> ret;
    std::lock_guard _l(lock);
    get_dependent_files_unlocked(file, ret);
    return ret;
  }

  void rel_db::get_dependent_files(const std::filesystem::path& file, std::set<std::filesystem::path>& ret) const
  {
    std::lock_guard _l(lock);
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

    std::lock_guard _l(lock);
    for (const auto& it : initial_list)
    {
      get_dependent_files_unlocked(it, file_list);
    }
  }


  std::set<std::filesystem::path> rel_db::get_removed_resources(const std::deque<std::filesystem::path>& file_list) const
  {
    std::lock_guard _l(lock);

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
    std::lock_guard _l(lock);

    std::set<std::filesystem::path> ret;
    for (const auto& it : file_list)
    {
      if (!files_resources.contains(it))
        ret.emplace(it);
    }
    return ret;
  }

  raw_data rel_db::serialize() const
  {
    std::lock_guard _l(lock);
    return rle::serialize(*this);
  }


  void rel_db::add_file(const std::string& file)
  {
    std::lock_guard _l(lock);
    if (auto it = files_resources.find(file); it != files_resources.end())
      return repack_file_unlocked(file);
    files_resources.emplace(file, file_info_t{});
  }

  void rel_db::add_file(const std::string& parent_file, const std::string& child_file)
  {
    std::lock_guard _l(lock);
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
    std::lock_guard _l(lock);
    files_resources[file].depend_on.insert(dependent_on);
    files_resources[dependent_on].dependent.insert(file);
  }

  void rel_db::set_processor_for_file(const std::string& file, id_t version_hash)
  {
    std::lock_guard _l(lock);
    files_resources[file].processor_hash = version_hash;
  }

  // root-resource / pack-file
  void rel_db::add_resource(const std::string& parent_file, id_t root_resource)
  {
    std::lock_guard _l(lock);
    files_resources[parent_file].child_resources.insert(root_resource);
    root_resources.insert_or_assign(root_resource, root_resource_info_t{});
  }

  void rel_db::add_resource(id_t root_resource, id_t child_resource)
  {
    std::lock_guard _l(lock);
    root_resources[root_resource].sub_resources.insert(child_resource);
    sub_resources.emplace(child_resource, root_resource);
  }

  void rel_db::set_pack_file(id_t root_resource, id_t pack_file_id)
  {
    std::lock_guard _l(lock);
    root_resources[root_resource].pack_file = pack_file_id;
  }

  void rel_db::remove_file(const std::string& file)
  {
    std::lock_guard _l(lock);
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
    std::lock_guard _l(lock);
    repack_file_unlocked(file);
  }

  void rel_db::repack_file_unlocked(const std::string& file)
  {
    if (auto it = files_resources.find(file); it != files_resources.end())
    {
      // we grab a ref to the entry
      file_info_t& entry = it->second;

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
    if (auto it = root_resources.find(root_resource); it != root_resources.end())
    {
      const root_resource_info_t entry = std::move(it->second);
      root_resources.erase(it);

      for (auto srit : entry.sub_resources)
        sub_resources.erase(srit);
    }
  }
}

