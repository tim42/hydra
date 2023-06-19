//
// created by : Timothée Feuillet
// date: 2022-5-8
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

#include <hydra/resources/packer.hpp>
#include <hydra/resources/processor.hpp>
#include <hydra/assets/raw.hpp>
#include <hydra/assets/serialized.hpp>

namespace neam::hydra::packer
{
  template<auto OnConstructed, auto OnDestructed>
  class raii_caller
  {
    public:
      raii_caller() { OnConstructed(); }
      ~raii_caller() { OnDestructed(); }
  };

  /// \brief Simple pass-through processor/packer
  struct raw_packer : resources::packer::packer<assets::raw_asset, raw_packer>
  {
    static constexpr id_t processor_hash = "neam/raw-processor:0.0.1"_rid;
    static constexpr id_t packer_hash = "neam/raw-packer:0.0.1"_rid;

    struct source_asset_conf_t
    {
      string_id id;
      bool simlink_ser;
    };
    static constexpr source_asset_conf_t source_assets[] =
    {
      { "font/sfnt"_rid, false }, // all ttf fonts are treated as raw data

      // handle generic serialized data:
      { "file-ext:.hser"_rid, true },

      // handle hconf:
      { "file-ext:.hcnf"_rid, false },
    };

    static void on_register()
    {
      for (const auto& it : source_assets)
      {
        resources::processor::register_processor(it.id, processor_hash, &resources::processor::basic_processor<assets::raw_asset::type_name>);
      }
    }

    static void on_unregister()
    {
      for (const auto& it : source_assets)
      {
        resources::processor::unregister_processor(it.id);
      }
    }

    static resources::packer::chain pack_resource(hydra::core_context& /*ctx*/, resources::processor::data&& data)
    {
      const id_t root_id = get_root_id(data.resource_id);

      data.db.resource_name(root_id, get_root_name(data.db, data.resource_id));

      bool simlink_ser = true;
      for (const auto& it : source_assets)
      {
        if (data.resource_type == it.id)
        {
          simlink_ser = it.simlink_ser;
          break;
        }
      }

      std::vector<resources::packer::data> ret;
      ret.push_back(
      {
        .id = root_id,
        .data = std::move(data.data),
        .metadata = std::move(data.metadata),
      });

      if (simlink_ser)
      {
        const id_t ser_id = specialize(data.resource_id, assets::serialized_asset<int>::type_name);
        data.db.resource_name(ser_id, _get_root_name<assets::serialized_asset<int>>(data.db, data.resource_id));
        ret.push_back(
        {
          .id = ser_id,
          .simlink_to_id = root_id,
          .mode = resources::packer::mode_t::simlink,
        });
      }
      return resources::packer::chain::create_and_complete(std::move(ret), root_id, resources::status::success);
    }

    inline static raii_caller<&on_register, &on_unregister> _registration;
    static_assert(&_registration == &_registration);
  };
}

