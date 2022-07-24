//
// created by : Timothée Feuillet
// date: 2022-4-26
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

#include <deque>
#include <set>
#include <string>
#include <filesystem>

#include <ntools/id/id.hpp>
#include <ntools/rle/rle.hpp>

namespace neam::resources
{
  /// \brief handle links between source files, resources, pack files and caches
  /// (pack files are ref-counted and removed when they reach 0 active resources referencing them)
  /// The main goal is to provide the following:
  ///  - no creeping disk usage (remove unused stuff)
  ///  - what's the chain of packer/processors that have built the resource? what version?
  ///  - what's the metadata hash for that resource?
  ///  - what to update if a file is removed/changed
  /// This data is only necessary when importing/packing data
  /// and is stored outside the index/pack files to be easily stripped from final builds.
  class rel_db
  {
    public: // query:
      /// \brief recursively get all pack files related to file
      std::set<id_t> get_pack_files(const std::string& file) const;

      /// \brief recursively get all resources related to file
      std::set<id_t> get_resources(const std::string& file) const;

      /// \brief return all the files that \e directly \e and \e indirectly depend on the given file
      std::set<std::filesystem::path> get_dependent_files(const std::filesystem::path& file) const;

      /// \brief insert all the files that \e directly \e and \e indirectly depend on the given file
      void get_dependent_files(const std::filesystem::path& file, std::set<std::filesystem::path>& ret) const;

      /// \brief add to file_list all the files that have \e direct \e and \e indirect dependencies to them
      void consolidate_files_with_dependencies(std::set<std::filesystem::path>& file_list) const;

      /// \brief get all the missing (primary) resources from the file_list
      std::set<std::filesystem::path> get_removed_resources(const std::deque<std::filesystem::path>& file_list) const;

      /// \brief get all the entries in file_list that aren't present in the file_list
      /// \note might be super slow
      std::set<std::filesystem::path> get_absent_resources(const std::deque<std::filesystem::path>& file_list) const;

      /// \brief Serialize the data, ensuring everything is fine (locks the instance whil'e its serializing)
      raw_data serialize() const;

    public: // setup (should not be used directly !!):
      void add_file(const std::string& file);
      void add_file(const std::string& parent_file, const std::string& child_file);
      void add_file_to_file_dependency(const std::string& file, const std::string& dependent_on);

      void set_processor_for_file(const std::string& file, id_t version_hash);

      // root-resource / pack-file
      void add_resource(const std::string& parent_file, id_t root_resource);
      void add_resource(id_t root_resource, id_t child_resource);
      void set_pack_file(id_t root_resource, id_t pack_file_id);

      void remove_file(const std::string& file);
      void repack_file(const std::string& file);

    private:
      void get_pack_files_unlocked(const std::string& file, std::set<id_t>& ret) const;
      void get_resources_unlocked(const std::string& file, std::set<id_t>& ret) const;

      void get_dependent_files_unlocked(const std::filesystem::path& file, std::set<std::filesystem::path>& ret) const;

      void remove_file_unlocked(const std::string& file);
      void repack_file_unlocked(const std::string& file);
      void remove_resource_unlocked(id_t root_resource);

    public:
      struct file_info_t
      {
        id_t processor_hash = id_t::none;
        id_t metadata_hash = id_t::none;

        std::set<std::string> child_files = {};
        std::set<id_t> child_resources = {};

        std::string parent_file = {};

        std::set<std::string> depend_on = {}; // file -> all files it depends on
        std::set<std::string> dependent = {}; // file -> all files that depend on it
      };
      struct root_resource_info_t
      {
        id_t pack_file;
        std::set<id_t> sub_resources;
      };

    private: // serialized:
      std::map<std::string, file_info_t> files_resources;
      std::map<id_t, root_resource_info_t> root_resources;
      std::map<id_t, id_t> sub_resources; // sub-resource -> root-resource

    private: // non-serialized:
      mutable spinlock lock;

      friend N_METADATA_STRUCT_DECL(neam::resources::rel_db);
  };
}

N_METADATA_STRUCT(neam::resources::rel_db::file_info_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(processor_hash),
    N_MEMBER_DEF(metadata_hash),
    N_MEMBER_DEF(child_files),
    N_MEMBER_DEF(child_resources),
    N_MEMBER_DEF(parent_file),
    N_MEMBER_DEF(depend_on),
    N_MEMBER_DEF(dependent)
  >;
};

N_METADATA_STRUCT(neam::resources::rel_db::root_resource_info_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(pack_file),
    N_MEMBER_DEF(sub_resources)
  >;
};

N_METADATA_STRUCT(neam::resources::rel_db)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(files_resources),
    N_MEMBER_DEF(root_resources),
    N_MEMBER_DEF(sub_resources)
  >;
};
