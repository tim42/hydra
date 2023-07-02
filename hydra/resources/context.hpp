//
// created by : Timothée Feuillet
// date: 2021-11-27
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

#include <ntools/async/chain.hpp>
#include <ntools/io/context.hpp>
#include <ntools/event.hpp>
#include <ntools/rle/rle.hpp>
#include <ntools/ct_string.hpp>
#include <ntools/macro.hpp>

#include <hydra/engine/conf/conf.hpp>

#include "enums.hpp"
#include "asset.hpp"
#include "index.hpp"
#include "concepts.hpp"
#include "file_map.hpp"
#include "rel_db.hpp"

namespace neam::hydra { class core_context; }

namespace neam::resources
{
  struct resource_configuration : hydra::conf::hconf<resource_configuration>
  {
    std::vector<std::string> extensions_to_ignore;
  };
}

N_METADATA_STRUCT(neam::resources::resource_configuration)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(extensions_to_ignore, neam::metadata::info{.description = c_string_t
      <
      "List of extensions (including the `.`) or file-types that will be not considered for import\n"
      "To exclude all PNG files by extension, add an entry with `.png`\n"
      "To exclude all PNG files by filetype, add an entry with: `image/png`\n\n"
      "To get filetype of a file, use the command: `file --mime-type the-file-name-here`.\n"
      "Beware of using a too generic file-type as it may prevent other files from being considered for import."
      >})
  >;
};
namespace neam::resources
{
  // base resource metadata type: (currently private as it's only for internal use)
  struct default_resource_metadata_t : public base_metadata_entry<default_resource_metadata_t>
  {
    static constexpr ct::string k_metadata_entry_description = "generic metadata used by the resource subsystem";
    static constexpr ct::string k_metadata_entry_name = "resource_metadata";


    bool strip_from_final_build = false;
    bool embed_in_index = false;
    bool skip_compression = false;
  };
}

N_METADATA_STRUCT(neam::resources::default_resource_metadata_t)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(strip_from_final_build, neam::metadata::info{.description = c_string_t
      <
      "Whether this resource should be completely removed from the final build.\n"
      "Please note that no reference check is done, and the resource is forcefully stripped from the data."
      >}),
    N_MEMBER_DEF(embed_in_index, neam::metadata::info{.description = c_string_t
      <
      "Whether this resource should be placed directly in the index instead of in a pack file\n"
      "Doing so will increase the size of the index and will force the resources to always be loaded in memory\n"
      "This also make sure that no filesystem access is needed to retrieve the resource\n"
      "The default behavior (if false) is controlled by N_RES_MAX_SIZE_TO_EMBED (currently: " N_EXP_STRINGIFY(N_RES_MAX_SIZE_TO_EMBED) "b)."
      >}),
    N_MEMBER_DEF(skip_compression, neam::metadata::info{.description = c_string_t
      <
      "Whether this resource should skip compression.\n"
      "Note that only resources bigger than " N_EXP_STRINGIFY(N_RES_MAX_SIZE_TO_COMPRESS) " bytes will be considered for compression\n"
#if !N_RES_LZMA_COMPRESSION
      "\n"
      "NOTE: LZMA compression disabled, flag will not do anything as everything will skip compression.\n"
      "Flag will be saved an honored when compression is enabled in the build.\n"
#endif
      >})
  >;
};

namespace neam::resources
{
  namespace processor
  {
    struct data;
  }

  /// \brief The comination of io::context and resources::index
  class context
  {
    public:
      using status_chain = async::chain<status>;
      using raw_chain = async::chain<raw_data&&, status>;

      template<typename T>
      using resource_chain = async::chain<T&&, status>;

      static constexpr id_t k_initial_index = "/initial_index:file-id"_rid;
      static constexpr id_t k_reldb_index = "/rel-db:file-id"_rid;
      static constexpr id_t k_self_index = "/self:file-id"_rid;
      static constexpr id_t k_boot_file_map = "/boot.file_map:file-map"_rid;
      static constexpr id_t k_index_key = "/index_key:id"_rid;

      static constexpr char k_metadata_extension[] = ".hrm";
      static constexpr char k_pack_extension[] = ".hpd";
      static constexpr char k_rel_db_extension[] = ".hrdb";

      static constexpr string_id k_configuration_name = "configuration/resources_context.hcnf"_rid;

    public:
      context(io::context& io, hydra::core_context& _ctx) : io_context(io), ctx(_ctx) {}

      /// \brief Returns the IO context.
      /// \note Unless trying to access non-resource files directly, please use the resources:: facilities,
      ///       particularly those located in this file
      io::context& get_io_context() { return io_context; }

      /// \brief Setup the context from the boot index to the final stuff
      /// \note the boot index should be named boot.index and boot.pack
      /// \note the boot process is asynchrnous, so as to allow doing other initialization during the boot process
      /// \see make_boot_data()
      ///
      /// process:
      ///  - load the boot index
      ///  - load the boot file-map from the initial data pack
      ///  - get the index file-id + key from the boot index
      ///  - additively load the index
      ///  - additively load the file-map (either an asset or a packed resource, placed at k_boot_file_map)
      ///    that file-map (or the pack containing it) must be referenced in the boot file-map
      ///
      /// If the boot index has file-id:/initial_index set to none, it is treated as the final index and the process stops here
      ///
      status_chain boot(id_t boot_index_id, const std::string& index_path = "boot.index")
      {
        io_context._wait_for_submit_queries();
        io_context.clear_mapped_files();
        prefix = get_prefix_from_filename(index_path);
        io_context.set_prefix_directory(prefix);
        return boot(boot_index_id, io_context.map_unprefixed_file(index_path));
      }

      status_chain boot(id_t boot_index_id, id_t _index_file_id, unsigned max_depth = 5, bool reload = false);

      /// \brief Create base_index_path.index / base_index_path.pack so that they are self-bootable
      /// \note this will not override any loaded index but will alter the mapped files to contain the index / pack / file-map
      /// \see make_chain_boot
      ///
      /// The created index will contain the following entries:
      ///   - k_boot_file_map an embedded file-map)
      ///   - k_initial_index set to id_t::none
      status_chain make_self_boot(id_t boot_index_id, const std::string& index_path = "boot.index", file_map&& boot_file_map = {});

      /// \brief Create base_index_path.index / base_index_path.pack so that they refer to another index
      /// \param target_index_path The prefix to use. After loading this index, all file access will be relative to this directory
      /// \param target_index_file Relative to \e target_index_path Indicates target the index to chain load
      /// \note The target index must be self-bootable or chain-bootable.
      /// \see make_self_boot
      status_chain make_chain_boot(id_t target_index_id, std::string target_index_path, std::string target_index_file, id_t boot_index_id, const std::string& boot_index_path = "boot.index");

      /// \brief Init the context from a clean index (and an optional rel-db)
      /// \note If saved, that index is self-contained, but cannot be reloaded (it does not self-reference, it does not have a prefix)
      void _init_with_clean_index(id_t index_key, bool init_empty_reldb = true);

      status_chain _init_with_index_data(id_t index_key, const void* data, uint32_t data_size);
      template<typename T, uint32_t Count>
      status_chain _init_with_index_data(id_t index_key, const T (&ar)[Count])
      {
        return _init_with_index_data(index_key, ar, Count * sizeof(T));
      }

    public: // index stuff:
      index& get_index() { return root; }
      const index& get_index() const { return root; }

      /// \brief Asynchronously loads an index.
      /// \note Only one index is associated with a context, load_index will replace the loaded one
      /// \note Queries done before the index gets loaded will be using the old index
      status_chain load_index(id_t index_id, const std::string& file_path)
      {
        prefix = get_prefix_from_filename(file_path);
        io_context.set_prefix_directory(prefix);
        index_file_id = io_context.map_unprefixed_file(file_path);
        root = index(index_id);
        return reload_index();
      }

      /// \brief Reloads an already setup/loaded index
      /// \note the operation is asynchronous, all operation done before the index is actually reloaded are done with the current one
      status_chain reload_index()
      {
        return boot(root.get_index_id(), index_file_id, 5, true);
      }

      /// \brief Additively load a new index.
      /// \see index::add_index
      status_chain add_index(id_t index_id, const std::string& file_path)
      {
        check::debug::n_assert(has_index, "Trying to combines indexes while no index has been ever loaded. Are you loading and combining right away?");
        const id_t fid = io_context.map_file(file_path);
        return add_index(index_id, fid);
      }

      /// \brief Additively load a new index.
      /// \see index::add_index
      status_chain add_index(id_t index_id, id_t index_fid);

      /// \brief Saves the current index and the rel-db if present (must have a loaded index)
      /// \note This operation is asynchronous.
      ///       It is possible to save and immediatly load another index without waiting for the IO to complete.
      status_chain save_index() const;


      /// \brief Return whether an index has been loaded
      bool is_index_loaded() const { return has_index; }

      /// \brief Return whether a resource is present
      bool has_resource(id_t rid) const { return is_index_loaded() && root.has_entry(rid); }

      /// \brief Return the created/modified time on the index
      std::filesystem::file_time_type get_index_modified_time() const
      {
        if (!has_index || !io_context.is_file_mapped(index_file_id))
          return {};
        return io_context.get_modified_or_created_time(index_file_id);
      }

      /// \brief Only work for indexes with already embedded rel-db
      /// \see _init_with_clean_index
      /// \see _has_embedded_reldb
      void _embed_reldb();

      bool _has_embedded_reldb() const;

      /// \brief return whether the index is mapped to io or not (that a reload can happen or not)
      bool is_index_mapped() const { return has_index && io_context.is_file_mapped(index_file_id); }

    public:
      /// \brief Load a map-file. Map files contains the list of all the necessary files that io::context can use.
      status_chain load_file_map(const id_t rid);

      /// \brief Add a file to the file-map (and apply the change)
      /// \note index changes require a call to save_index() (not done by this function)
      void add_to_file_map(std::string file);

      /// \brief Add a file to the file-map (and apply the change)
      /// \note index changes require a call to save_index() (not done by this function)
      void remove_from_file_map(const std::set<id_t>& rid);

    public:
      /// \brief Go over all the resources located in the index and repack them. Will remove invalid resources (those that cannot be found).
      /// \note the operation is mostly asynchronous. Data change occuring during the process may lead to corrupted data
      /// \note It will also generate a map-file
      /// \warning it (currently) create pack files based on size (going over all the resources, placing them in a packfile until
      ///           a size limit is reached then creating a new pack file and continuing the process with it)
      /// \todo A better approach, probably based on access patterns to better use the iovec capabilities of io::context
      void repack_data();

    public: // queries:
      std::string resource_name(id_t rid) const;

      /// \brief Return whether a db is loaded or not
      /// \note Some operations (notably resource import and a few others) require a db to be loaded
      ///       DB are stripped from builds as they contains un-necessary information
      bool has_db() const { return has_rel_db; }

      /// \brief Return a const ref to the rel-db.
      /// \warning It is incorrect to call this function when has_db() is false
      const rel_db& get_db() const;

      /// \brief Return a ref to the rel-db.
      /// \warning It is incorrect to call this function when has_db() is false
      rel_db& _get_non_const_db();

    public: // resource handling stuff:
      /// \brief reads and decode a resource.
      /// \note asynchronous
      /// \note only resources with flags::type_data can be read this way
      /// \see read_raw_resource
      template<concepts::Asset T>
      resource_chain<T> read_resource(id_t rid)
      {
        return read_raw_resource(rid).then([](raw_data&& data, bool success)
        {
          if (!success)
            return resource_chain<T>::create_and_complete({}, status::failure);

          status st = status::success;
          T ret = T::from_raw_data(data, st);
          return resource_chain<T>::create_and_complete(std::move(ret), st);
        });
      }

      /// \brief encode and write a resource.
      /// \see write_raw_resource
      /// \note resource serialization is done synchronously for now
      template<concepts::Asset T>
      status_chain write_resource(id_t rid, const T& res)
      {
        // serialize the resource:
        status st = status::success;
        raw_data data = T::to_raw_data(res, st);
        if (st == status::failure)
          return status_chain::create_and_complete(st);

        return write_raw_resource(rid, std::move(data)).then([st](status final_status)
        {
          // propagate partial_success:
          return worst(final_status, st);
        });
      }

    public: // raw resource handling stuff:
      /// \brief reads a raw resource.
      /// \note asynchronous
      /// \note only resources with flags::type_data can be read this way
      io::context::read_chain read_raw_resource(id_t rid);

      /// \brief write a raw resource.
      /// \note asynchronous
      /// \note only resources with flags::whole_file are guaranteed to be always modifiable this way.
      /// (FIXME: support writing to packed resources when the data size is the same or smaller)
      /// \warning The index is not/must not be modified by this function. Compression flag will also be respected.
      ///          If writing a resource would require an index modification to work, the operation will fail
      status_chain write_raw_resource(id_t rid, raw_data&& data);

    public: // importing/packing:
      /// \brief (re)Import (process and pack) a resource from a file on disk. The file must be a valid, readable file.
      /// A metadata file (filename + ._hm extension) will also be accessed if it is present.
      /// \note asynchrnous, potentially multi-threaded.
      /// \note index changes require a call to save_index() (not done by this function)
      /// \warning the resource must be a valid path to the file (either relative to the CWD or absolute)
      status_chain import_resource(const std::filesystem::path& resource, std::optional<metadata_t>&& overrides = {});

      /// \brief Import (process and pack) a resource from a memory buffer.
      /// The resource path will only be used for the resource id and determine the processor to apply if it can be determined from the content of the buffer.
      /// It does not need to exist on disk / have a valid path.
      /// \note asynchrnous, potentially multi-threaded.
      /// \note index changes require a call to save_index() (not done by this function)
      /// \note if the metadata provided has a valid file-id, it will be saved back if non-empty (if it is empty, it will be removed).
      /// \warning the resource must be relative to the source folder.
      status_chain import_resource(const std::filesystem::path& resource, raw_data&& data, metadata_t&& metadata);

      /// \brief Helper for packing processed resources.
      status_chain _pack_resource(processor::data&& proc_data);

      /// \brief Where the source folder is
      std::filesystem::path source_folder;


    public: // resource removal
      /// \brief Handle the removal of a source file and all its related metadata/resources/subresources/pack files/...
      /// \note File must be relative to the source folder
      async::continuation_chain on_source_file_removed(const std::filesystem::path& file, bool reimport = false);

    public: // resource management
      /// \brief Return the files that require a repack because of processor/packer version change
      /// Should be called only once
      std::set<std::filesystem::path> get_sources_needing_reimport() const;

      /// \brief Return the files present in the index but missing in 'state'
      std::set<std::filesystem::path> get_removed_sources(const std::deque<std::filesystem::path>& state) const
      {
        check::debug::n_assert(has_rel_db, "cannot return removed sources without a rel db present");
        return db.get_removed_resources(state);
      }
      /// \brief Return the files present in the index but missing in 'state'
      std::set<std::filesystem::path> get_non_imported_sources(const std::deque<std::filesystem::path>& state) const
      {
        check::debug::n_assert(has_rel_db, "cannot return non imported sources without a rel db present");
        return filter_files(db.get_absent_resources(state));
      }

      /// \brief consolidate a file list with files that are dependent (directly and indirectly) on them
      void consolidate_files_with_dependencies(std::set<std::filesystem::path>& file_list) const
      {
        check::debug::n_assert(has_rel_db, "cannot call to consolidate_files_with_dependencies without a rel db present");
        return db.consolidate_files_with_dependencies(filter_files(file_list));
      }

      void consolidate_files_with_dependencies(const std::filesystem::path& file, std::set<std::filesystem::path>& file_list) const
      {
        check::debug::n_assert(has_rel_db, "cannot call to consolidate_files_with_dependencies without a rel db present");
        return db.get_dependent_files(file, file_list);
      }

      /// \brief Remove files that have extensions to be ignored
      std::set<std::filesystem::path> filter_files(std::set<std::filesystem::path>&& files) const;
      std::set<std::filesystem::path>& filter_files(std::set<std::filesystem::path>& files) const;

      raw_data _get_serialized_reldb() const
      {
        check::debug::n_assert(has_rel_db, "cannot call to _get_serialized_reldb without a rel db present");
        return db.serialize();
      }

    public: // management:
      void _prepare_engine_shutdown()
      {
        configuration.remove_watch();
      }

    public: // events:
      // Called after an index has been loaded/reloaded. Called asynchronously.
      cr::event<> on_index_loaded;

    private:
      status_chain reload_index(id_t index_id, id_t fid);

      io::context::write_chain write_index(id_t file_id, const index& idx) const;

      /// \brief apply the map-file to the current state:
      /// Map files contains a line-encoded data with the following format:
      /// First line:  base-path prefix. ('./' for "no" prefix). Must be either empty or end with a /
      /// Other lines: files to map (relatives to the prefix directory)
      void apply_file_map(const file_map& fm, bool additive = false);

    private:
      static std::string get_prefix_from_filename(const std::string& name);

    private:
      io::context& io_context;
      hydra::core_context& ctx;

      // Prefix from CWD to index.Is updated when chain-loading
      std::string prefix;

      index root;
      id_t index_file_id = id_t::none;
      bool has_index = false;

      spinlock file_map_lock;
      file_map current_file_map;
      rel_db db;
      bool has_rel_db = false;

      resource_configuration configuration;
  };
}
