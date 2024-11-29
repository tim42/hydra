//
// created by : Timothée Feuillet
// date: 2024-2-11
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

#include <hydra/hydra_glm.hpp>
#include <ntools/struct_metadata/struct_metadata.hpp>
#include <ntools/enum.hpp>
#include <ntools/id/string_id.hpp>
#include <ntools/type_id.hpp>

#include "types.hpp"

namespace neam::hydra::shaders
{
  namespace internal
  {
    enum class struct_validation
    {
      valid = 0,
      out_of_order_members      = 1 << 0,
      missing_glsl_type_name    = 1 << 1,
      entry_after_unbound_array = 1 << 2,
    };
    N_ENUM_FLAG(struct_validation);

    template<typename T> struct is_array : std::false_type {};
    template<typename T, size_t N> struct is_array<std::array<T, N>> : std::true_type {};
    template<typename T, size_t N> struct is_array<const std::array<T, N>> : std::true_type {};
    template<typename T> struct is_array<unbound_array<T>> : std::true_type {};
    template<typename T> struct is_array<const unbound_array<T>> : std::true_type {};

    template<typename T> struct array_size {};
    template<typename T, size_t N> struct array_size<std::array<T, N>> : std::integral_constant<size_t, N> {};
    template<typename T, size_t N> struct array_size<const std::array<T, N>> : std::integral_constant<size_t, N> {};
    template<typename T> struct array_size<unbound_array<T>> : std::integral_constant<size_t, 0> {};
    template<typename T> struct array_size<const unbound_array<T>> : std::integral_constant<size_t, 0> {};

    using generate_function_t = std::string (*)();

#if N_HYDRA_SHADERS_ALLOW_GENERATION
    void register_block_struct(string_id cpp_name, std::string glsl_name, id_t hash, generate_function_t generate, std::vector<id_t>&& deps);
    void unregister_block_struct(string_id cpp_name);
    std::string generate_all_structs();
    std::string generate_struct_body(id_t cpp_name);
    bool is_struct_registered(id_t cpp_name);
    std::string generate_structs(const std::vector<id_t>& ids);
    // Return all the dependencies
    void get_all_dependencies(id_t cpp_name, std::vector<id_t>& deps, bool insert_self = false);
#endif

    /// \brief Uniform block (+ structs) generator.
    /// Handle validation (check that a C++ struct has the correct layout) + handle some generation
    /// \note the only accepted types are either other structs or types from the shaders namespace
    template<metadata::concepts::StructWithMetadata Struct>
    class block
    {
      private:
        /// \brief Provide a hash of the struct (use the hash constexpr variable instead)
        static consteval id_t compute_hash()
        {
          id_t ret = id_t::none;
#if !N_STRIP_DEBUG
          validate();

          [&]<size_t... Indices>(std::index_sequence<Indices...>)
          {
            ([&]<size_t Index>()
            {
              using member = ct::list::get_type<n_metadata_member_list<Struct>, Index>;


              if constexpr(is_array<typename member::type>::value)
              {
                using inner_type = typename member::type::value_type;
                ret = combine(ret, string_id(glsl_type_name<inner_type>));
                constexpr size_t size = array_size<typename member::type>::value;
                ret = combine(ret, id_t(size));
              }
              else
              {
                ret = combine(ret, string_id(glsl_type_name<typename member::type>));
              }

              ret = combine(ret, string_id(member::name));
            } .template operator()<Indices>(), ...);
          } (std::make_index_sequence<ct::list::size<n_metadata_member_list<Struct>>> {});

#endif
          return ret;
        }
      public:
        /// \brief Hash of the struct. Can be used to detect layout/shader changes or incompatibilities.
        static constexpr id_t hash = compute_hash();

#if N_HYDRA_SHADERS_ALLOW_GENERATION
        /// \brief Generate all the members of the block/struct.
        /// \note this function will return all the memebers on a single line.
        static std::string generate_member_code()
        {
          validate();
          std::string ret;
          [&]<size_t... Indices>(std::index_sequence<Indices...>)
          {
            ([&]<size_t Index>()
            {
              using member = ct::list::get_type<n_metadata_member_list<Struct>, Index>;


              if constexpr(is_array<typename member::type>::value)
              {
                using inner_type = typename member::type::value_type;
                constexpr size_t size = array_size<typename member::type>::value;
                if (size != 0)
                  ret = fmt::format("{}{} {}[{}];", ret, glsl_type_name<inner_type>.view(), member::name.str, size);
                else
                  ret = fmt::format("{}{} {}[];", ret, glsl_type_name<inner_type>.view(), member::name.str);
              }
              else
              {
                ret = fmt::format("{}{} {};", ret, glsl_type_name<typename member::type>.view(), member::name.str);
              }

            } .template operator()<Indices>(), ...);
          } (std::make_index_sequence<ct::list::size<n_metadata_member_list<Struct>>> {});
          return ret;
        }

        /// \brief So we can generate those structs automatically + add their hashes for dependencies
        static std::vector<id_t> compute_dependencies()
        {
          std::vector<id_t> ret;
          [&]<size_t... Indices>(std::index_sequence<Indices...>)
          {
            ([&]<size_t Index>()
            {
              using member = ct::list::get_type<n_metadata_member_list<Struct>, Index>;

              if constexpr (is_array<typename member::type>::value)
              {
                using inner_type = typename member::type::value_type;
                if constexpr (metadata::concepts::StructWithMetadata<inner_type>)
                {
                  ret.push_back((id_t)ct::type_hash<inner_type>);
                }
              }
              else
              {
                // We only push structs with metadata, as this avoids all builtin types
                if constexpr (metadata::concepts::StructWithMetadata<typename member::type>)
                {
                  ret.push_back((id_t)ct::type_hash<typename member::type>);
                }
              }
            } .template operator()<Indices>(), ...);
          } (std::make_index_sequence<ct::list::size<n_metadata_member_list<Struct>>> {});
          return ret;
        }
#endif
      private:
#if !N_STRIP_DEBUG
        /// \brief Generate compilation errors when the provided struct isn't valid
        static consteval void validate()
        {
          constexpr struct_validation validation = inner_validate();
          static_assert(!has_flag(validation, struct_validation::out_of_order_members), "struct has out-of-order members");
          static_assert(!has_flag(validation, struct_validation::missing_glsl_type_name), "struct has members whose type doesn't provide a glsl_type_name overload");
          static_assert(!has_flag(validation, struct_validation::entry_after_unbound_array), "struct has members that are present after an unbound array entry");
          static_assert(!std::is_same_v<decltype(glsl_type_name<Struct>), std::nullptr_t>, "struct doesn't provide a glsl_type_name overload");
        }
        static consteval struct_validation inner_validate()
        {
          struct_validation ret = struct_validation::valid;
          bool should_be_end = false;
          uint32_t offset = 0;
          [&]<size_t... Indices>(std::index_sequence<Indices...>)
          {
            ([&]<size_t Index>()
            {
              using member = ct::list::get_type<n_metadata_member_list<Struct>, Index>;

              // out-of-order
              if (member::offset < offset)
                ret |= struct_validation::out_of_order_members;
              if (should_be_end)
                  ret |= struct_validation::entry_after_unbound_array;

              offset = member::offset + member::size;

              // missing glsl type name
              if constexpr (is_array<typename member::type>::value)
              {
                constexpr size_t size = array_size<typename member::type>::value;
                using inner_type = typename member::type::value_type;
                if constexpr (std::is_same_v<decltype(glsl_type_name<inner_type>), std::nullptr_t>)
                  ret |= struct_validation::missing_glsl_type_name;
                if (size == 0)
                  should_be_end = true;
              }
              else
              {
                if constexpr (std::is_same_v<decltype(glsl_type_name<typename member::type>), std::nullptr_t>)
                  ret |= struct_validation::missing_glsl_type_name;
              }
            } .template operator()<Indices>(), ...);
          } (std::make_index_sequence<ct::list::size<n_metadata_member_list<Struct>>> {});

          return ret;
        }
#endif
    };

#if N_HYDRA_SHADERS_ALLOW_GENERATION
    template<typename BlockStruct>
    class raii_register
    {
      public:
        raii_register() { register_block_struct(ct::type_name<BlockStruct>, std::string{glsl_type_name<BlockStruct>.view()}, block<BlockStruct>::hash, block<BlockStruct>::generate_member_code, block<BlockStruct>::compute_dependencies()); }
        ~raii_register() { unregister_block_struct(ct::type_name<BlockStruct>); }
    };
#endif
  }

  /// \brief block / struct cpp < - > glsl helper. Generate glsl code for shaders.
  template<typename Struct>
  class block_struct
  {
    public:
    private:
#if N_HYDRA_SHADERS_ALLOW_GENERATION
      inline static internal::raii_register<Struct> _registration;

      // force instantiation of the static member: (and avoid a warning)
      static_assert(&_registration == &_registration);
#endif
  };
}

