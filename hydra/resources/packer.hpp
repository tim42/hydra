//
// created by : Timothée Feuillet
// date: 2021-12-17
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

#include <ntools/raw_data.hpp>
#include <ntools/async/async.hpp>

#include "concepts.hpp"
#include "enums.hpp"
#include "metadata.hpp"
#include "processor.hpp"

namespace neam::hydra { class core_context; }
namespace neam::resources { class rel_db; }

namespace neam::resources::packer
{
  enum class mode_t
  {
    data,
    simlink,
  };

  struct data
  {
    id_t id;
    raw_data data = {};
    metadata_t metadata = {};

    id_t simlink_to_id = id_t::none;
    mode_t mode = mode_t::data;
  };

  // vector: the sub-resources to write
  // id_t: the root resource id (aka. the pack file to use. must be unique and not be shared by other resources)
  //       if not present in the vector, will not be added to the index.
  //       only serves to indentify the intermediate file to use.
  // status: success, failure, partial success.
  using chain = async::chain<std::vector<data>&&, id_t, status>;

  /// \brief A packer takes a resource data (from a processor or directly from disk) 
  using function = chain(*)(hydra::core_context& ctx, processor::data&& data);

  bool register_packer(id_t type_id, id_t version_hash, function pack_resource);
  bool unregister_packer(id_t type_id);

  function get_packer(id_t type_id);
  id_t get_packer_hash(id_t type_id);

  const std::set<id_t>& get_packer_hashs();

  // helpers:
  template<ct::string_holder IDName, typename Packer>
  class raii_register
  {
    public:
      raii_register() { register_packer(string_id(IDName), Packer::packer_hash, Packer::pack_resource); }
      ~raii_register() { unregister_packer(string_id(IDName)); }
  };

  /// \brief a packer base class (packers are just a plain function, but inheriting from this class may help),
  ///        handles automatic registration and provide some helpers
  template<typename ResourceType, typename BaseType>
  class packer
  {
    public:
      // The child class must define this static function:
      //  static pack_resource (signature: packer::function)
      //
      // The child class must define this static member:
      //  static constexpr id_t packer_hash = "my-company/my-packer:1.0.0"_rid; // can be any format, but should include provider and version

      static constexpr id_t get_root_id(id_t file_id) { return specialize(file_id, ResourceType::type_name); }
      static std::string get_root_name(const rel_db& db, id_t file_id)
      {
        return _get_root_name<ResourceType>(db, file_id);
      }
      template<typename Type>
      static std::string _get_root_name(const rel_db& db, id_t file_id)
      {
        return fmt::format("{}:{}", db.resource_name(file_id), Type::type_name.str);
      }

  private:
      inline static raii_register<ResourceType::type_name, BaseType> _registration;

      // force instantiation of the static member: (and avoid a warning)
      static_assert(&_registration == &_registration);
  };
}

