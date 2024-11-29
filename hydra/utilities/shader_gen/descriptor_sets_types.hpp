//
// created by : Timothée Feuillet
// date: 2024-2-25
//
//
// Copyright (c) 2024 Timothée Feuillet
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

#include <variant>

#include <hydra/vulkan/descriptor_set_layout_binding.hpp>

#include "types.hpp"
#include "block.hpp"

namespace neam::hydra::shaders
{
  /// \brief Empty, trivial struct whose sole purpose is do perform aliasing
  /// \warning entries flagged with noop must have the alias_of_previous_entry metadata
  template<typename InnerType>
  struct noop
  {
  };

  namespace internal
  {
    struct descriptor_write_entry
    {
      std::variant
      <
        VkDescriptorImageInfo,
        VkDescriptorBufferInfo,

        std::vector<VkDescriptorImageInfo>,
        std::vector<VkDescriptorBufferInfo>

        // TODO: Add other types + add vectors of other types
      > info;

      auto& image_info() { return std::get<VkDescriptorImageInfo>(info); }
      auto& buffer_info() { return std::get<VkDescriptorBufferInfo>(info); }
      auto& image_array() { return std::get<std::vector<VkDescriptorImageInfo>>(info); }
      auto& buffer_array() { return std::get<std::vector<VkDescriptorBufferInfo>>(info); }
    };
    template<uint32_t EntryCount>
    struct descriptor_write_struct
    {
      std::array<descriptor_write_entry, EntryCount> entries;
      std::array<VkWriteDescriptorSet, EntryCount> descriptors;
    };

    template<typename Type>
    struct descriptor_generator_wrapper
    {
      static constexpr bool is_unbound_array = false;

#if N_HYDRA_SHADERS_ALLOW_GENERATION
      static std::string generate_glsl_code(const std::string& name, uint32_t set, uint32_t binding)
      {
        return Type::generate_glsl_code(name, set, binding);
      }
      static void update_dependencies(std::vector<id_t>& ids)
      {
        Type::update_dependencies(ids);
      }
#endif
      static void fill_descriptor_layout_bindings(std::vector<vk::descriptor_set_layout_binding>& bindings, uint32_t binding)
      {
        Type::fill_descriptor_layout_bindings(bindings, binding, 1);
      }
      template<uint32_t MaxSize>
      static void setup_descriptor(Type& ref, uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws, uint32_t array_index = 0)
      {
        ref.template setup_descriptor_info<MaxSize, false>(binding, vk_ds, dws, 0);
        Type::template setup_descriptor<MaxSize, false>(binding, vk_ds, dws, 0, 1);
      }
    };
    template<typename InnerType>
    struct descriptor_generator_wrapper<noop<InnerType>>
    {
      static constexpr bool is_unbound_array = descriptor_generator_wrapper<InnerType>::is_unbound_array;

#if N_HYDRA_SHADERS_ALLOW_GENERATION
      static std::string generate_glsl_code(const std::string& name, uint32_t set, uint32_t binding)
      {
        return descriptor_generator_wrapper<InnerType>::generate_glsl_code(name, set, binding);
      }
      static void update_dependencies(std::vector<id_t>& ids)
      {
        descriptor_generator_wrapper<InnerType>::update_dependencies(ids);
      }
#endif
    };
    template<typename Type, size_t Count>
    struct descriptor_generator_wrapper<std::array<Type, Count>>
    {
      static constexpr bool is_unbound_array = false;

#if N_HYDRA_SHADERS_ALLOW_GENERATION
      static std::string generate_glsl_code(const std::string& name, uint32_t set, uint32_t binding)
      {
        return fmt::format("{}[{}]", Type::generate_glsl_code(name, set, binding), Count);
      }
      static void update_dependencies(std::vector<id_t>& ids)
      {
        descriptor_generator_wrapper<Type>::update_dependencies(ids);
      }
#endif
      static void fill_descriptor_layout_bindings(std::vector<vk::descriptor_set_layout_binding>& bindings, uint32_t binding)
      {
        Type::fill_descriptor_layout_bindings(bindings, binding, Count);
      }
      template<uint32_t MaxSize>
      static void setup_descriptor(std::array<Type, Count>& ref, uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws)
      {
        Type::template setup_descriptor<MaxSize, true>(binding, vk_ds, dws, 0, Count);
        for (uint32_t i = 0; i < Count; ++i)
          ref[i].template setup_descriptor_info<MaxSize, true>(binding, vk_ds, dws, i);
      }
    };
    template<typename Type, typename Allocator>
    struct descriptor_generator_wrapper<std::vector<Type, Allocator>>
    {
      // Used for validation, this element must be the last one
      static constexpr bool is_unbound_array = true;

#if N_HYDRA_SHADERS_ALLOW_GENERATION
      static std::string generate_glsl_code(const std::string& name, uint32_t set, uint32_t binding)
      {
        return fmt::format("{}[]", Type::generate_glsl_code(name, set, binding));
      }
      static void update_dependencies(std::vector<id_t>& ids)
      {
        Type::update_dependencies(ids);
      }
#endif
      static void fill_descriptor_layout_bindings(std::vector<vk::descriptor_set_layout_binding>& bindings, uint32_t binding)
      {
        Type::fill_descriptor_layout_bindings(bindings, binding, 1024);

        bindings.back().set_binding_flag(VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT|VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
      }
      template<uint32_t MaxSize>
      static void setup_descriptor(std::vector<Type, Allocator>& ref, uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws)
      {
        Type::template setup_descriptor<MaxSize, true>(binding, vk_ds, dws, 0, ref.size());
        for (uint32_t i = 0; i < ref.size(); ++i)
          ref[i].template setup_descriptor_info<MaxSize, true>(binding, vk_ds, dws, i);
      }
    };
  }

  struct alias_of_previous_entry
  {
    using metadata = alias_of_previous_entry;
  };

  enum mode_t
  {
    none = 0,
    can_read = 1 << 0,
    can_write = 1 << 1,

    readonly = can_read,
    writeonly = can_write,
    readwrite = readonly | writeonly,
  };
  enum texture_type_t
  {
    no_type = 0,

    floating_point    = 1,
    integer           = 2,
    unsigned_integer  = 3,

    _start_list_shift = 2,

    _start_list = 1 << _start_list_shift,
    _prefix_mask = _start_list - 1,

    _1d               = 0 << _start_list_shift,
    _1d_array         = 1 << _start_list_shift,
    _2d               = 2 << _start_list_shift,
    _2d_array         = 3 << _start_list_shift,
    _2d_ms            = 4 << _start_list_shift,
    _2d_ms_array      = 5 << _start_list_shift,
    _3d               = 6 << _start_list_shift,
    _cube             = 7 << _start_list_shift,
    _cube_array       = 8 << _start_list_shift,

    _last_texture_type_kind = _cube_array,
    _last_texture_type = _last_texture_type_kind | unsigned_integer,
    _texture_type_count = _last_texture_type - (_last_texture_type_kind >> _start_list_shift),

    float_1d = _1d | floating_point,
    float_1d_array = _1d_array | floating_point,
    float_2d = _2d | floating_point,
    float_2d_array = _2d_array | floating_point,
    float_2d_ms = _2d_ms | floating_point,
    float_2d_ms_array = _2d_ms_array | floating_point,
    float_3d = _3d | floating_point,
    float_cube = _cube | floating_point,
    float_cube_array = _cube_array | floating_point,

    int_1d = _1d | integer,
    int_1d_array = _1d_array | integer,
    int_2d = _2d | integer,
    int_2d_array = _2d_array | integer,
    int_2d_ms = _2d_ms | integer,
    int_2d_ms_array = _2d_ms_array | integer,
    int_3d = _3d | integer,
    int_cube = _cube | integer,
    int_cube_array = _cube_array | integer,

    uint_1d = _1d | unsigned_integer,
    uint_1d_array = _1d_array | unsigned_integer,
    uint_2d = _2d | unsigned_integer,
    uint_2d_array = _2d_array | unsigned_integer,
    uint_2d_ms = _2d_ms | unsigned_integer,
    uint_2d_ms_array = _2d_ms_array | unsigned_integer,
    uint_3d = _3d | unsigned_integer,
    uint_cube = _cube | unsigned_integer,
    uint_cube_array = _cube_array | unsigned_integer,
  };

  /// \brief Transform a texture type to a contiguous index
  static constexpr uint32_t texture_type_to_index(texture_type_t tt)
  {
    // assumes a well formed texture type:
    return (tt - 1) - (tt >> _start_list_shift);
  }
  // just sanity checks:
  static_assert(texture_type_to_index(_last_texture_type) == _texture_type_count - 1);
  static_assert(texture_type_to_index(float_1d) == 0);

  /// \brief Transform a texture type to a contiguous index
  static constexpr texture_type_t index_to_texture_type(uint32_t idx)
  {
    // assumes a well formed index:
    return (texture_type_t)(((idx * 4) / 3) + 1);
  }
  // just sanity checks:
  static_assert(index_to_texture_type(texture_type_to_index(_last_texture_type)) == _last_texture_type);
  static_assert(index_to_texture_type(texture_type_to_index(float_1d)) == float_1d);
  static_assert(index_to_texture_type(texture_type_to_index(uint_2d_ms)) == uint_2d_ms);

  enum texture_format_t
  {
    no_format,
    // TODO
  };

  template<typename Struct, mode_t Mode = readonly>
  struct buffer
  {
    using struct_type = Struct;
    static constexpr mode_t mode = Mode;

    VkBuffer vk_buffer = nullptr;
    uint32_t offset = 0;



    buffer() = default;
    buffer(const buffer& o) = default;
    template<typename S, mode_t M> buffer(const buffer<S, M>& o) : vk_buffer(o.vk_buffer), offset(o.offset) {}
    buffer(const vk::buffer& b, uint32_t _offset = 0) : vk_buffer(b._get_vk_buffer()), offset(_offset) {}

    buffer& operator = (const buffer& o) = default;
    template<typename S, mode_t M> buffer& operator = (const buffer<S, M>& o) { vk_buffer = o.vk_buffer; offset = o.offset; return *this;}
    buffer& operator = (const vk::buffer& b) { vk_buffer = b._get_vk_buffer(); return *this; }

#if N_HYDRA_SHADERS_ALLOW_GENERATION
    static std::string generate_glsl_code(const std::string& name, uint32_t set, uint32_t binding)
    {
      static constexpr const char* mode_str_array[] = { "readonly writeonly", "readonly", "writeonly", "" };

      return fmt::format("layout(scalar, set = {}, binding = {}) restrict {} buffer _hydra_buffer_{}_{} {{ {} }} {}",
                         set, binding, mode_str_array[mode], set, binding,
                         internal::generate_struct_body((id_t)ct::type_hash<Struct>), name);
    }
    static void update_dependencies(std::vector<id_t>& ids)
    {
      internal::get_all_dependencies((id_t)ct::type_hash<Struct>, ids, false);
    }
#endif
    static void fill_descriptor_layout_bindings(std::vector<vk::descriptor_set_layout_binding>& bindings, uint32_t binding, uint32_t count)
    {
      bindings.emplace_back(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
    }

    template<uint32_t MaxSize, bool IsArray>
    void setup_descriptor_info(uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws, uint32_t array_index)
    {
      const VkDescriptorBufferInfo info =
      {
        .buffer = vk_buffer,
        .offset = offset,
        .range = VK_WHOLE_SIZE,
      };
      if constexpr (!IsArray)
        dws.entries[binding] = { .info = info, };
      else
        dws.entries[binding].buffer_array()[array_index] = info;
    }
    template<uint32_t MaxSize, bool IsArray>
    static void setup_descriptor(uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws, uint32_t array_index, uint32_t array_size)
    {
      if constexpr (IsArray)
      {
        dws.entries[binding] = { .info = std::vector<VkDescriptorBufferInfo>{}, };
        dws.entries[binding].buffer_array().resize(array_size);
      }
      dws.descriptors[binding] =
      {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, vk_ds,
        binding, array_index, array_size, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        nullptr, IsArray ? dws.entries[binding].buffer_array().data() : &dws.entries[binding].buffer_info(), nullptr
      };
    }
  };


  /// \brief *texture* and *image* types. (readonly will generate a texture, anything with write will generate an image)
  /// \note Maybe force a readonly image storage
  template<mode_t Mode, texture_type_t Type = float_2d, texture_format_t Format = no_format>
  struct image
  {
    static constexpr mode_t mode = Mode;
    static constexpr texture_type_t type = Type;
    static constexpr texture_format_t format = Format;

    VkImageView vk_image = nullptr;


    image() = default;
    image(const image& o) = default;
    template<mode_t M, texture_type_t T, texture_format_t F> image(const image<M, T, F>& o) : vk_image(o.vk_image) {}
    image(const vk::image_view& im) : vk_image(im.get_vk_image_view()) {}
    image(VkImageView im) : vk_image(im) {}

    image& operator = (const image& o) = default;
    template<mode_t M, texture_type_t T, texture_format_t F> image& operator = (const image<M, T, F>& o) { vk_image = o.vk_image; return *this;}
    image& operator = (const vk::image_view& im) { vk_image = im.get_vk_image_view(); return *this; }
    image& operator = (VkImageView im) { vk_image = im; return *this; }

#if N_HYDRA_SHADERS_ALLOW_GENERATION
    static std::string generate_glsl_code(const std::string& name, uint32_t set, uint32_t binding, const char* force_base_type = nullptr)
    {
      static constexpr const char* mode_str_array[] = { "restrict readonly writeonly", "", "restrict writeonly", "" };
      static constexpr const char* base_type_str_array[] = { "image", "texture", "image", "image" };
      static constexpr const char* type_suffix_str_array[] =
      {
        "1D",
        "1DArray",
        "2D",
        "2DArray",
        "2DMS",
        "2DMSArray",
        "3D",
        "Cube",
        "CubeArray",
      };

      const char* prefix = "";
      switch (type & texture_type_t::_prefix_mask)
      {
        case texture_type_t::floating_point: prefix = ""; break;
        case texture_type_t::integer: prefix = "i"; break;
        case texture_type_t::unsigned_integer: prefix = "u"; break;
      }

      constexpr uint32_t suffix_index = (type >> texture_type_t::_start_list_shift);
      if (force_base_type == nullptr)
        force_base_type = base_type_str_array[mode];
      // FIXME: Account for format if not no_format
      return fmt::format("layout(set = {}, binding = {}) uniform {} {}{}{} {}",
                         set, binding, mode_str_array[mode], prefix, force_base_type, type_suffix_str_array[suffix_index], name);

    }
    static void update_dependencies(std::vector<id_t>&) {}
#endif
    static void fill_descriptor_layout_bindings(std::vector<vk::descriptor_set_layout_binding>& bindings, uint32_t binding, uint32_t count)
    {
      bindings.emplace_back(binding, (mode & can_write ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE), count);
    }

    template<uint32_t MaxSize, bool IsArray>
    void setup_descriptor_info(uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws, uint32_t array_index)
    {
      VkDescriptorImageInfo info;
      if constexpr (mode & can_write)
      {
        info = VkDescriptorImageInfo
        {
          .imageView = vk_image,
          .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
      }
      else
      {
        info = VkDescriptorImageInfo
        {
          .imageView = vk_image,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
      }
      if constexpr (!IsArray)
        dws.entries[binding] = { .info = info, };
      else
        dws.entries[binding].image_array()[array_index] = info;
    }
    template<uint32_t MaxSize, bool IsArray>
    static void setup_descriptor(uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws, uint32_t array_index, uint32_t array_size)
    {
      if constexpr (IsArray)
      {
        dws.entries[binding] = { .info = std::vector<VkDescriptorImageInfo>{}, };
        dws.entries[binding].image_array().resize(array_size);
      }
      dws.descriptors[binding] =
      {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, vk_ds,
        binding, array_index, array_size, (mode & can_write ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE),
        IsArray ? dws.entries[binding].image_array().data() : &dws.entries[binding].image_info(), nullptr, nullptr
      };
    }
  };

  struct sampler
  {
    VkSampler vk_sampler = nullptr;

    sampler() = default;
    sampler(const sampler& o) = default;
    sampler(const vk::sampler& s) : vk_sampler(s._get_vk_sampler()) {}

    sampler& operator = (const sampler& o) = default;
    sampler& operator = (const vk::sampler& s) { vk_sampler = s._get_vk_sampler(); return *this; }

#if N_HYDRA_SHADERS_ALLOW_GENERATION
    static std::string generate_glsl_code(const std::string& name, uint32_t set, uint32_t binding)
    {
      return fmt::format("layout(set = {}, binding = {}) uniform sampler {}", set, binding, name);
    }
    static void update_dependencies(std::vector<id_t>&) {}
#endif
    static void fill_descriptor_layout_bindings(std::vector<vk::descriptor_set_layout_binding>& bindings, uint32_t binding, uint32_t count)
    {
      bindings.emplace_back(binding, VK_DESCRIPTOR_TYPE_SAMPLER, count);
    }

    template<uint32_t MaxSize, bool IsArray>
    void setup_descriptor_info(uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws, uint32_t array_index)
    {
      const VkDescriptorImageInfo info =
      {
        .sampler = vk_sampler,
      };
      if constexpr (!IsArray)
        dws.entries[binding] = { .info = info, };
      else
        dws.entries[binding].image_array()[array_index] = info;
    }
    template<uint32_t MaxSize, bool IsArray>
    static void setup_descriptor(uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws, uint32_t array_index, uint32_t array_size)
    {
      if constexpr (IsArray)
      {
        dws.entries[binding] = { .info = std::vector<VkDescriptorImageInfo>{}, };
        dws.entries[binding].image_array().resize(array_size);
      }
      dws.descriptors[binding] =
      {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, vk_ds,
        binding, array_index, array_size, VK_DESCRIPTOR_TYPE_SAMPLER,
        IsArray ? dws.entries[binding].image_array().data() : &dws.entries[binding].image_info(), nullptr, nullptr
      };
    }
  };

  template<texture_type_t Type = float_2d, texture_format_t Format = no_format>
  struct combined_image_sampler : image<readonly, Type, Format>, sampler
  {
    combined_image_sampler() = default;
    combined_image_sampler(const combined_image_sampler&) = default;
    combined_image_sampler& operator = (const combined_image_sampler&) = default;

    combined_image_sampler(const vk::image_view& im, const vk::sampler& s) : image<readonly, Type, Format>(im), sampler(s) {}
    combined_image_sampler(const vk::image_view& im, const sampler& s) : image<readonly, Type, Format>(im), sampler(s) {}
    template<mode_t M, texture_type_t T, texture_format_t F>
    combined_image_sampler(const image<M, T, F>& im, const vk::sampler& s) : image<readonly, Type, Format>(im), sampler(s) {}
    template<mode_t M, texture_type_t T, texture_format_t F>
    combined_image_sampler(const image<M, T, F>& im, const sampler& s) : image<readonly, Type, Format>(im), sampler(s) {}

    using image<readonly, Type, Format>::operator =;
    using sampler::operator =;

#if N_HYDRA_SHADERS_ALLOW_GENERATION
    static std::string generate_glsl_code(const std::string& name, uint32_t set, uint32_t binding)
    {
      return image<readonly, Type, Format>::generate_glsl_code(name, set, binding, "sampler");
    }
    static void update_dependencies(std::vector<id_t>&) {}
#endif
    static void fill_descriptor_layout_bindings(std::vector<vk::descriptor_set_layout_binding>& bindings, uint32_t binding, uint32_t count)
    {
      bindings.emplace_back(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count);
    }

    template<uint32_t MaxSize, bool IsArray>
    void setup_descriptor_info(uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws, uint32_t array_index)
    {
      const VkDescriptorImageInfo info =
      {
        .sampler = this->vk_sampler,
        .imageView = this->vk_image,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      if constexpr (!IsArray)
        dws.entries[binding] = { .info = info, };
      else
        dws.entries[binding].image_array()[array_index] = info;
    }
    template<uint32_t MaxSize, bool IsArray>
    static void setup_descriptor(uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws, uint32_t array_index, uint32_t array_size)
    {
      if constexpr (IsArray)
      {
        dws.entries[binding] = { .info = std::vector<VkDescriptorImageInfo>{}, };
        dws.entries[binding].image_array().resize(array_size);
      }
      dws.descriptors[binding] =
      {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, vk_ds,
        binding, array_index, array_size, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        IsArray ? dws.entries[binding].image_array().data() : &dws.entries[binding].image_info(), nullptr, nullptr
      };
    }
  };

  /// \brief Represent a UBO entry
  /// \note inherit from buffer so as to make them interchangeable (you can affect a buffer from a ubo / ...)
  template<typename Struct>
  struct ubo : buffer<Struct, readonly>
  {
    using buffer<Struct, readonly>::operator=;
#if N_HYDRA_SHADERS_ALLOW_GENERATION
    static std::string generate_glsl_code(const std::string& name, uint32_t set, uint32_t binding)
    {
      return fmt::format("layout(scalar, set = {}, binding = {}) uniform restrict readonly _hydra_ubo_{}_{} {{ {} }} {}",
                         set, binding, set, binding,
                         internal::generate_struct_body((id_t)ct::type_hash<Struct>), name);
    }
    static void update_dependencies(std::vector<id_t>& ids)
    {
      internal::get_all_dependencies((id_t)ct::type_hash<Struct>, ids, false);
    }
#endif
    static void fill_descriptor_layout_bindings(std::vector<vk::descriptor_set_layout_binding>& bindings, uint32_t binding, uint32_t count)
    {
      bindings.emplace_back(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
    }

    template<uint32_t MaxSize, bool IsArray>
    void setup_descriptor_info(uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws, uint32_t array_index)
    {
      dws.entries[binding] =
      {
        .info = VkDescriptorBufferInfo
        {
          .buffer = this->vk_buffer,
          .offset = this->offset,
          .range = sizeof(Struct),
        },
      };
    }
    template<uint32_t MaxSize, bool IsArray>
    static void setup_descriptor(uint32_t binding, VkDescriptorSet vk_ds, internal::descriptor_write_struct<MaxSize>& dws, uint32_t array_index, uint32_t array_size)
    {
      if constexpr (IsArray)
      {
        dws.entries[binding] = { .info = std::vector<VkDescriptorBufferInfo>{}, };
        dws.entries[binding].buffer_array().resize(array_size);
      }
      dws.descriptors[binding] =
      {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, vk_ds,
        binding, array_index, array_size, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        nullptr, IsArray ? dws.entries[binding].buffer_array().data() : &dws.entries[binding].buffer_info(), nullptr
      };
    }
  };
}

