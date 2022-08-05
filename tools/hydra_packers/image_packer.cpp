//
// created by : Timothée Feuillet
// date: 2022-5-3
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

#include "image_packer.hpp"

namespace neam::hydra::packer
{
    struct image_packer : resources::packer::packer<assets::image, image_packer>
  {
    static constexpr id_t packer_hash = "neam/image-packer:0.0.1"_rid;

    static resources::packer::chain pack_resource(hydra::core_context& /*ctx*/, resources::processor::data&& data)
    {
      const id_t root_id = get_root_id(data.resource_id);
      data.db.resource_name(root_id, get_root_name(data.db, data.resource_id));

      // TODO: mip downsampling
      // TODO: format conversion/support

//       image_metadata md = { .target_format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM };
//       data.metadata.try_get("image"_rid, md);
//       data.metadata.set("image"_rid, md);

      // cannot be const, data must be moved from
      rle::status rst;
      image_packer_input in;
      rst = rle::in_place_deserialize(data.data, in);
      if (rst == rle::status::failure)
      {
        data.db.error<image_packer>(root_id, "failed to deserialize processor data");
        return resources::packer::chain::create_and_complete({}, id_t::invalid, resources::status::failure);
      }


      assets::image root;
      root.size = glm::uvec3(in.size, 0);
      root.format = VK_FORMAT_R8G8B8A8_UNORM;//md.target_format;

      std::vector<resources::packer::data> ret;
      ret.emplace_back(); // reserve a space for the header:

      resources::status status = resources::status::success;

      {
        const id_t mip_id = parametrize(specialize(root_id, assets::image_mip::type_name), "0");
        data.db.resource_name(mip_id, fmt::format("{}:{}(0)", data.db.resource_name(root_id), assets::image_mip::type_name.string));
        root.mips.push_back(mip_id);
        resources::status st = resources::status::success;
        ret.emplace_back(resources::packer::data
        {
          .id = mip_id,
          .data = assets::image_mip::to_raw_data( { .size = root.size, .texels = std::move(in.texels), }, st),
          .metadata = {}
        });
        status = resources::worst(status, st);
      }

      resources::status st = resources::status::success;
      ret.front() =
      {
        .id = root_id,
        .data = assets::image::to_raw_data(root, st),
        .metadata = std::move(data.metadata),
      };
      status = resources::worst(status, st);

      return resources::packer::chain::create_and_complete(std::move(ret), root_id, status);
    }
  };
}
