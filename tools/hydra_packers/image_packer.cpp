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

#include <numeric>
#include <glm/gtc/packing.hpp>
#include <ntools/integer_tools.hpp>

namespace neam::hydra::packer
{
  // Format conversion + downscaling
  namespace
  {
    enum vk_format_type : uint8_t
    {
      uint,
      sint,
      snorm,
      unorm,
      uscaled = uint, // simple aliases, as for us it doesn't change anything
      sscaled = sint,
      srgb = unorm, // fixme
      sfloat,
      // ufloat, // not supported!
    };
    struct vk_format_split
    {
      bool supported;

      uint8_t component_count;
      uint8_t component_bit_count;
      vk_format_type format_type;
    };

    constexpr vk_format_split split_vk_format(VkFormat format)
    {
      switch (format)
      {
#define N_FORMAT_ENUM_CASE(TK, COMP, COMP_BIT_COUNT) \
        case VK_FORMAT_##TK##_UNORM: return vk_format_split { .supported = true, .component_count = COMP, .component_bit_count = COMP_BIT_COUNT, .format_type = unorm }; \
        case VK_FORMAT_##TK##_SNORM: return vk_format_split { .supported = true, .component_count = COMP, .component_bit_count = COMP_BIT_COUNT, .format_type = snorm }; \
        case VK_FORMAT_##TK##_UINT: return vk_format_split { .supported = true, .component_count = COMP, .component_bit_count = COMP_BIT_COUNT, .format_type = uint }; \
        case VK_FORMAT_##TK##_SINT: return vk_format_split { .supported = true, .component_count = COMP, .component_bit_count = COMP_BIT_COUNT, .format_type = sint }; \
        case VK_FORMAT_##TK##_USCALED: return vk_format_split { .supported = true, .component_count = COMP, .component_bit_count = COMP_BIT_COUNT, .format_type = uscaled }; \
        case VK_FORMAT_##TK##_SSCALED: return vk_format_split { .supported = true, .component_count = COMP, .component_bit_count = COMP_BIT_COUNT, .format_type = sscaled }; \

#define N_FORMAT_ENUM_CASE_SRGB(TK, COMP, COMP_BIT_COUNT) \
        N_FORMAT_ENUM_CASE(TK, COMP, COMP_BIT_COUNT) \
        case VK_FORMAT_##TK##_SRGB: return vk_format_split { .supported = true, .component_count = COMP, .component_bit_count = COMP_BIT_COUNT, .format_type = srgb };

#define N_FORMAT_ENUM_CASE_FLT(TK, COMP, COMP_BIT_COUNT) \
        N_FORMAT_ENUM_CASE(TK, COMP, COMP_BIT_COUNT) \
        case VK_FORMAT_##TK##_SFLOAT: return vk_format_split { .supported = true, .component_count = COMP, .component_bit_count = COMP_BIT_COUNT, .format_type = sfloat };

#define N_FORMAT_ENUM_CASE_NO_NORM(TK, COMP, COMP_BIT_COUNT) \
        case VK_FORMAT_##TK##_UINT: return vk_format_split { .supported = true, .component_count = COMP, .component_bit_count = COMP_BIT_COUNT, .format_type = uint }; \
        case VK_FORMAT_##TK##_SINT: return vk_format_split { .supported = true, .component_count = COMP, .component_bit_count = COMP_BIT_COUNT, .format_type = sint }; \
        case VK_FORMAT_##TK##_SFLOAT: return vk_format_split { .supported = true, .component_count = COMP, .component_bit_count = COMP_BIT_COUNT, .format_type = sfloat };

        N_FORMAT_ENUM_CASE_SRGB(R8, 1, 8);
        N_FORMAT_ENUM_CASE_SRGB(R8G8, 2, 8);
        N_FORMAT_ENUM_CASE_SRGB(R8G8B8, 3, 8);
        N_FORMAT_ENUM_CASE_SRGB(R8G8B8A8, 4, 8);
        N_FORMAT_ENUM_CASE_SRGB(B8G8R8A8, 4, 8);

        N_FORMAT_ENUM_CASE_FLT(R16, 1, 16);
        N_FORMAT_ENUM_CASE_FLT(R16G16, 2, 16);
        N_FORMAT_ENUM_CASE_FLT(R16G16B16, 3, 16);
        N_FORMAT_ENUM_CASE_FLT(R16G16B16A16, 4, 16);

        N_FORMAT_ENUM_CASE_NO_NORM(R32, 1, 32);
        N_FORMAT_ENUM_CASE_NO_NORM(R32G32, 2, 32);
        N_FORMAT_ENUM_CASE_NO_NORM(R32G32B32, 3, 32);
        N_FORMAT_ENUM_CASE_NO_NORM(R32G32B32A32, 4, 32);

        N_FORMAT_ENUM_CASE_NO_NORM(R64, 1, 64);
        N_FORMAT_ENUM_CASE_NO_NORM(R64G64, 2, 64);
        N_FORMAT_ENUM_CASE_NO_NORM(R64G64B64, 3, 64);
        N_FORMAT_ENUM_CASE_NO_NORM(R64G64B64A64, 4, 64);

#undef N_FORMAT_ENUM_CASE
#undef N_FORMAT_ENUM_CASE_SRGB
#undef N_FORMAT_ENUM_CASE_FLT
#undef N_FORMAT_ENUM_CASE_NO_NORM
        default: return vk_format_split { .supported = false, };
      }
    }


    template<uint32_t BitCount, vk_format_type Format>
    [[maybe_unused]] static constexpr void* component_intermediate_v = nullptr;

    template<> [[maybe_unused]] constexpr int8_t component_intermediate_v<8, sint> = 0;
    template<> [[maybe_unused]] constexpr uint8_t component_intermediate_v<8, uint> = 0;
    template<> [[maybe_unused]] constexpr float component_intermediate_v<8, snorm> = 0;
    template<> [[maybe_unused]] constexpr float component_intermediate_v<8, unorm> = 0;

    template<> [[maybe_unused]] constexpr int16_t component_intermediate_v<16, sint> = 0;
    template<> [[maybe_unused]] constexpr uint16_t component_intermediate_v<16, uint> = 0;
    template<> [[maybe_unused]] constexpr float component_intermediate_v<16, snorm> = 0;
    template<> [[maybe_unused]] constexpr float component_intermediate_v<16, unorm> = 0;
    template<> [[maybe_unused]] constexpr float component_intermediate_v<16, sfloat> = 0;

    template<> [[maybe_unused]] constexpr int32_t component_intermediate_v<32, sint> = 0;
    template<> [[maybe_unused]] constexpr uint32_t component_intermediate_v<32, uint> = 0;
    template<> [[maybe_unused]] constexpr float component_intermediate_v<32, sfloat> = 0;

    template<> [[maybe_unused]] constexpr int64_t component_intermediate_v<64, sint> = 0;
    template<> [[maybe_unused]] constexpr uint64_t component_intermediate_v<64, uint> = 0;
    template<> [[maybe_unused]] constexpr double component_intermediate_v<64, sfloat> = 0;

    template<uint32_t BitCount, vk_format_type Format>
    using component_intermediate_t = std::remove_cv_t<decltype(component_intermediate_v<BitCount, Format>)>;

    template<uint32_t BitCount, vk_format_type Format>
    component_intermediate_t<BitCount, Format> read_component(const void* v)
    {
      return *reinterpret_cast<const component_intermediate_t<BitCount, Format>*>(v);
    }
    template<>
    component_intermediate_t<8, unorm> read_component<8, unorm>(const void* v)
    {
      return glm::unpackUnorm1x8(*reinterpret_cast<const uint8_t*>(v));
    }
    template<>
    component_intermediate_t<16, unorm> read_component<16, unorm>(const void* v)
    {
      return glm::unpackUnorm1x16(*reinterpret_cast<const uint16_t*>(v));
    }
    template<>
    component_intermediate_t<8, snorm> read_component<8, snorm>(const void* v)
    {
      return glm::unpackSnorm1x8(*reinterpret_cast<const uint8_t*>(v));
    }
    template<>
    component_intermediate_t<16, snorm> read_component<16, snorm>(const void* v)
    {
      return glm::unpackSnorm1x16(*reinterpret_cast<const uint16_t*>(v));
    }
    template<>
    component_intermediate_t<16, sfloat> read_component<16, sfloat>(const void* v)
    {
      return glm::unpackHalf1x16(*reinterpret_cast<const uint16_t*>(v));
    }
    template<uint32_t BitCount, vk_format_type Format>
    void write_component(void* v, component_intermediate_t<BitCount, Format> c)
    {
      *reinterpret_cast<component_intermediate_t<BitCount, Format>*>(v) = c;
    }
    template<>
    void write_component<8, unorm>(void* v, component_intermediate_t<8, unorm> c)
    {
      *reinterpret_cast<uint8_t*>(v) = glm::packUnorm1x8(c);
    }
    template<>
    void write_component<16, unorm>(void* v, component_intermediate_t<16, unorm> c)
    {
      *reinterpret_cast<uint16_t*>(v) = glm::packUnorm1x16(c);
    }
    template<>
    void write_component<8, snorm>(void* v, component_intermediate_t<8, snorm> c)
    {
      *reinterpret_cast<uint8_t*>(v) = glm::packSnorm1x8(c);
    }
    template<>
    void write_component<16, snorm>(void* v, component_intermediate_t<16, snorm> c)
    {
      *reinterpret_cast<uint16_t*>(v) = glm::packSnorm1x16(c);
    }
    template<>
    void write_component<16, sfloat>(void* v, component_intermediate_t<16, sfloat> c)
    {
      *reinterpret_cast<uint16_t*>(v) = glm::packHalf1x16(c);
    }


    /// \brief Perform base format conversion for a single texel
    template<vk_format_split From, vk_format_split To>
    void convert_single_texel(uint8_t* src, uint8_t* dst)
    {
      constexpr uint8_t read_component_count = std::min(From.component_count, To.component_count);
      constexpr uint8_t write_default_component_count = To.component_count - read_component_count;
      constexpr uint32_t read_incr = From.component_bit_count / 8;
      constexpr uint32_t write_incr = To.component_bit_count / 8;

      for (uint8_t i = 0; i < read_component_count; ++i)
      {
        const component_intermediate_t<From.component_bit_count, From.format_type> vr = read_component<From.component_bit_count, From.format_type>(src + i * read_incr);
        const component_intermediate_t<To.component_bit_count, To.format_type> vw = static_cast<component_intermediate_t<To.component_bit_count, To.format_type>>(vr);
        write_component<To.component_bit_count, To.format_type>(dst + i * write_incr, vw);
      }
      if constexpr (write_default_component_count > 0)
      {
        if constexpr (To.format_type == sfloat || To.format_type == snorm || To.format_type == unorm)
        {
          for (uint8_t i = read_component_count; i < To.component_count; ++i)
            write_component<To.component_bit_count, To.format_type>(dst + i * write_incr, (i == 4 ? 1 : 0));
        }
        else
        {
          for (uint8_t i = read_component_count; i < To.component_count; ++i)
            write_component<To.component_bit_count, To.format_type>(dst + i * write_incr, 0);
        }
      }
    }

    /// \brief Perform a box downscale for a single component
    template<vk_format_split Format>
    void downscale_box_single_component(const uint8_t* a, const uint8_t* b, const uint8_t* c, const uint8_t* d, uint8_t* dst)
    {
      const auto va = read_component<Format.component_bit_count, Format.format_type>(a);
      const auto vb = read_component<Format.component_bit_count, Format.format_type>(b);
      const auto vc = read_component<Format.component_bit_count, Format.format_type>(c);
      const auto vd = read_component<Format.component_bit_count, Format.format_type>(d);

      component_intermediate_t<Format.component_bit_count, Format.format_type> vab = std::midpoint(va, vb);
      component_intermediate_t<Format.component_bit_count, Format.format_type> vcd = std::midpoint(vc, vd);
      write_component<Format.component_bit_count, Format.format_type>(dst, std::midpoint(vab, vcd));
    }

    /// \brief Perform a box downscale for a single texel
    template<vk_format_split Format>
    void downscale_box_single_texel(const uint8_t* a, const uint8_t* b, const uint8_t* c, const uint8_t* d, uint8_t* dst)
    {
      constexpr uint32_t incr = Format.component_bit_count / 8;
      for (uint32_t i = 0; i < Format.component_count; ++i)
        downscale_box_single_component<Format>(a + i * incr, b + i * incr, c + i * incr, d + i * incr, dst + i * incr);
    }

    /// \brief downscale a 2D region using a box filter
    /// \todo More filter types
    template<vk_format_split Format>
    void downscale_region(glm::uvec2 src_size, const uint8_t* src, uint8_t* dst)
    {
      constexpr uint32_t component_stride = Format.component_bit_count / 8;
      constexpr uint32_t texel_stride = component_stride * Format.component_count;

      const glm::uvec2 dst_size = glm::max(glm::uvec2{1u, 1u}, (src_size)/ 2u);
      const glm::uvec2 src_max_texel = src_size - 1u;

      const uint32_t src_line_stride = texel_stride * (src_size.x);
      const uint32_t dst_line_stride = texel_stride * (dst_size.x);

      for (glm::uvec2 it {0, 0}; it.y < dst_size.y; ++it.y)
      {
        for (it.x = 0u; it.x < dst_size.x; ++it.x)
        {
          const glm::uvec2 a =                         it * 2u + glm::uvec2(0, 0);
          const glm::uvec2 b = glm::min(src_max_texel, it * 2u + glm::uvec2(0, 1));
          const glm::uvec2 c = glm::min(src_max_texel, it * 2u + glm::uvec2(1, 0));
          const glm::uvec2 d = glm::min(src_max_texel, it * 2u + glm::uvec2(1, 1));
          downscale_box_single_texel<Format>
          (
            src + a.x * texel_stride + a.y * src_line_stride,
            src + b.x * texel_stride + b.y * src_line_stride,
            src + c.x * texel_stride + c.y * src_line_stride,
            src + d.x * texel_stride + d.y * src_line_stride,
            dst + it.x * texel_stride + it.y * dst_line_stride
          );
        }
      }
    }
  }

  struct image_packer : resources::packer::packer<assets::image, image_packer>
  {
    static constexpr id_t packer_hash = "neam/image-packer:0.0.2"_rid;


    static raw_data compute_next_mip_level(glm::uvec2 size, const raw_data& prev_mip, VkFormat vk_format)
    {
      const vk_format_split format = split_vk_format(vk_format);

      const uint32_t component_size = format.component_bit_count / 8;
      const uint32_t texel_size = component_size * format.component_count;
      const glm::uvec2 dst_size = glm::max(glm::uvec2{1u, 1u}, (size)/ 2u);

      raw_data ret = raw_data::allocate(dst_size.x * dst_size.y * texel_size);

#define N_DOWNSCALE_BOX_BTC(FMT, COMP) \
            if (format.component_bit_count == 8) \
            {  if constexpr (FMT != sfloat) downscale_region<vk_format_split{.component_count = COMP, .component_bit_count = 8, .format_type = FMT}>(size, prev_mip.get_as<uint8_t>(), ret.get_as<uint8_t>()); } \
            else if (format.component_bit_count == 16) \
              downscale_region<vk_format_split{.component_count = COMP, .component_bit_count = 16, .format_type = FMT}>(size, prev_mip.get_as<uint8_t>(), ret.get_as<uint8_t>()); \
            else if (format.component_bit_count == 32) \
            {  if constexpr (FMT != snorm && FMT != unorm) downscale_region<vk_format_split{.component_count = COMP, .component_bit_count = 32, .format_type = FMT}>(size, prev_mip.get_as<uint8_t>(), ret.get_as<uint8_t>()); } \
            else if (format.component_bit_count == 64) \
            {  if constexpr (FMT != snorm && FMT != unorm) downscale_region<vk_format_split{.component_count = COMP, .component_bit_count = 64, .format_type = FMT}>(size, prev_mip.get_as<uint8_t>(), ret.get_as<uint8_t>()); }

#define N_DOWNSCALE_BOX_COMP(FMT) \
          if (format.component_count == 1) \
          { \
            N_DOWNSCALE_BOX_BTC(FMT, 1); \
          } \
          else if (format.component_count == 2) \
          { \
            N_DOWNSCALE_BOX_BTC(FMT, 2); \
          } \
          else if (format.component_count == 3) \
          { \
            N_DOWNSCALE_BOX_BTC(FMT, 3); \
          } \
          else if (format.component_count == 4) \
          { \
            N_DOWNSCALE_BOX_BTC(FMT, 4); \
          }

      switch (format.format_type)
      {
        case uint: { N_DOWNSCALE_BOX_COMP(uint); break; }
        case sint: { N_DOWNSCALE_BOX_COMP(sint); break; }
        case unorm: { N_DOWNSCALE_BOX_COMP(unorm); break; }
        case snorm: { N_DOWNSCALE_BOX_COMP(snorm); break; }
        case sfloat: { N_DOWNSCALE_BOX_COMP(sfloat); break; }
      }
      return ret;
    }


    static resources::packer::chain pack_resource(hydra::core_context& /*ctx*/, resources::processor::data&& data)
    {
      TRACY_SCOPED_ZONE;
      const id_t root_id = get_root_id(data.resource_id);
      data.db.resource_name(root_id, get_root_name(data.db, data.resource_id));
      data.db.reference_metadata_type<image_metadata>(data.resource_id);

      // TODO: 1D/2D-layer/3D textures
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
      root.size = glm::uvec3(in.size, 1);
      root.format = VK_FORMAT_R8G8B8A8_UNORM;//md.target_format;

      std::vector<resources::packer::data> ret;
      ret.emplace_back(); // reserve a space for the header:

      resources::status status = resources::status::success;

      {
        raw_data prev_mip = raw_data::duplicate(in.texels);
        {
          const id_t mip_id = parametrize(specialize(root_id, assets::image_mip::type_name), "0");
          data.db.resource_name(mip_id, fmt::format("{}:{}(0)", data.db.resource_name(root_id), assets::image_mip::type_name.str));
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
        {
          const uint32_t mip_count = static_cast<uint32_t>(std::floor(std::log2(std::max(root.size.x, root.size.y)))) + 1u;
          glm::uvec2 size { root.size.x, root.size.y };
          for (uint32_t i = 1; i < mip_count; ++i)
          {
            prev_mip = compute_next_mip_level(size, prev_mip, root.format);
            size = glm::max(glm::uvec2{1u, 1u}, (size) / 2u);
            const id_t mip_id = parametrize(specialize(root_id, assets::image_mip::type_name), fmt::format("{}", i).c_str());
            data.db.resource_name(mip_id, fmt::format("{}:{}({})", data.db.resource_name(root_id), assets::image_mip::type_name.str, i));
            root.mips.push_back(mip_id);
            resources::status st = resources::status::success;
            ret.emplace_back(resources::packer::data
            {
              .id = mip_id,
              .data = assets::image_mip::to_raw_data( { .size = {size, 1}, .texels = raw_data::duplicate(prev_mip), }, st),
              .metadata = {}
            });
            status = resources::worst(status, st);
          }
        }
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
