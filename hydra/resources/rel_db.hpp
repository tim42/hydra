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
#include <ntools/type_id.hpp>

#include "metadata.hpp"

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
    public: // types:
      struct message_t
      {
        cr::logger::severity severity;
        std::string source;
        std::string message;
      };

      struct message_list_t
      {
        std::vector<message_t> list;
      };

    public: // query:
      /// \brief recursively get all pack files related to file
      std::set<id_t> get_pack_files(const std::string& file) const;

      /// \brief recursively get all resources related to file
      std::set<id_t> get_resources(const std::string& file, bool include_files_id = false) const;

      /// \brief Return the referenced metadata types
      std::set<id_t> get_referenced_metadata_types(const std::string& file) const;

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

      /// \brief get all the files that needs a repack because of a packer/processor change
      /// \note it might reimport more than necessary (a packer change for a sub-resource will trigger a reimport of all resurces related to the source file).
      /// \note might be super slow, but should only be done once
      std::set<std::filesystem::path> get_files_requiring_reimport(const std::set<id_t>& processors, const std::set<id_t>& packers) const;

      /// \brief Serialize the data, ensuring everything is fine (locks the instance whil'e its serializing)
      raw_data serialize() const;

      /// \brief Return the resource name for the corresponding RID
      std::string resource_name(id_t rid) const;

      /// \brief Return the messages for the corresponding RID
      message_list_t get_messages(id_t rid) const;

      /// \brief Return the metadata info for the given type
      metadata_type_registration_t get_type_metadata(id_t type_id) const;

    public: // processor&packers entries:
      void resource_name(id_t rid, std::string name);

      template<typename Prov, typename... Args>
      void error(id_t res, fmt::format_string<Args...> str, Args&&... args)
      {
        _log<Prov>(cr::logger::severity::error, res, str, std::forward<Args>(args)...);
      }
      template<typename Prov, typename... Args>
      void warning(id_t res, fmt::format_string<Args...> str, Args&&... args)
      {
        _log<Prov>(cr::logger::severity::warning, res, str, std::forward<Args>(args)...);
      }
      template<typename Prov, typename... Args>
      void message(id_t res, fmt::format_string<Args...> str, Args&&... args)
      {
        _log<Prov>(cr::logger::severity::message, res, str, std::forward<Args>(args)...);
      }
      template<typename Prov, typename... Args>
      void debug(id_t res, fmt::format_string<Args...> str, Args&&... args)
      {
        _log<Prov>(cr::logger::severity::debug, res, str, std::forward<Args>(args)...);
      }

      template<typename Prov, typename... Args>
      void _log(cr::logger::severity s, id_t res, fmt::format_string<Args...> str, Args&&... args)
      {
        log_str(s, res, ct::type_name<Prov>.str, fmt::format(str, std::forward<Args>(args)...));
      }

    public: // processor specific entries:
      void add_file_to_file_dependency(const std::string& file, const std::string& dependent_on);
      void set_processor_for_file(const std::string& file, id_t version_hash);

      void reference_metadata_type(const std::string& file, id_t metadata_type);
      // helper for types inheriting from base_metadata_entry
      template<typename T>
      void reference_metadata_type(const std::string& file)
      {
        return reference_metadata_type(file, string_id(T::k_metadata_entry_name));
      }

    public: // setup (should not be used directly unless in internal resource code !!):
      void add_file(const std::string& file);
      void add_file(const std::string& parent_file, const std::string& child_file);


      // root-resource / pack-file
      void add_resource(const std::string& parent_file, id_t root_resource);
      void add_resource(id_t root_resource, id_t child_resource);
      void set_pack_file(id_t root_resource, id_t pack_file_id);
      void set_packer_for_resource(id_t root_resource, id_t packer_hash);

      void remove_file(const std::string& file);
      void repack_file(const std::string& file);

      void reference_metadata_type(id_t root_resource, id_t metadata_type);
      // helper for types inheriting from base_metadata_entry
      template<typename T>
      void reference_metadata_type(id_t root_resource)
      {
        return reference_metadata_type(root_resource, string_id(T::k_metadata_entry_name));
      }

    public:
      void force_assign_registered_metadata_types();

    private:
      void get_pack_files_unlocked(const std::string& file, std::set<id_t>& ret) const;
      std::string get_root_file_unlocked(const std::string& file) const;
      std::string get_root_file_unlocked(id_t root_res) const;
      void get_resources_unlocked(const std::string& file, std::set<id_t>& ret, bool include_files_id) const;

      void get_dependent_files_unlocked(const std::filesystem::path& file, std::set<std::filesystem::path>& ret) const;

      void remove_file_unlocked(const std::string& file);
      void repack_file_unlocked(const std::string& file);
      void remove_resource_unlocked(id_t root_resource);

      void log_str(cr::logger::severity s, id_t res, std::string provider, std::string str);

      void reference_metadata_type_unlocked(const std::string& file, id_t metadata_type);
      void reference_metadata_type_unlocked(id_t root_resource, id_t metadata_type);

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

        std::set<id_t> referenced_metadata_types;
      };
      struct root_resource_info_t
      {
        std::string parent_file = {};
        id_t packer_hash = id_t::none;
        id_t pack_file = id_t::none;
        std::set<id_t> sub_resources = {};
      };

    private: // serialized:
      std::map<std::string, file_info_t> files_resources;
      std::map<id_t, root_resource_info_t> root_resources;
      std::map<id_t, id_t> sub_resources; // sub-resource -> root-resource
      std::map<id_t, std::string> resources_names;
      std::map<id_t, message_list_t> resources_messages;

      std::map<id_t, metadata_type_registration_t> metadata_types;
    private: // non-serialized:
      mutable shared_spinlock lock;

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
    N_MEMBER_DEF(dependent),
    N_MEMBER_DEF(referenced_metadata_types)
  >;
};

N_METADATA_STRUCT(neam::resources::rel_db::root_resource_info_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(packer_hash),
    N_MEMBER_DEF(pack_file),
    N_MEMBER_DEF(sub_resources)
  >;
};

N_METADATA_STRUCT(neam::resources::rel_db::message_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(severity),
    N_MEMBER_DEF(source),
    N_MEMBER_DEF(message)
  >;
};

N_METADATA_STRUCT(neam::resources::rel_db::message_list_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(list)
  >;
};

N_METADATA_STRUCT(neam::resources::rel_db)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(files_resources),
    N_MEMBER_DEF(root_resources),
    N_MEMBER_DEF(sub_resources),
    N_MEMBER_DEF(resources_names),
    N_MEMBER_DEF(resources_messages),
    N_MEMBER_DEF(metadata_types)
  >;
};
