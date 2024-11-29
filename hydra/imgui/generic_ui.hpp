//
// created by : Timothée Feuillet
// date: 2022-7-9
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

#include "imgui.hpp"
#include <ntools/raw_data.hpp>
#include <ntools/rle/rle.hpp>

namespace neam::hydra::imgui
{
  /// \brief Generate an imgui UI for a given type
  /// (that type might not necessarily be in the current executable, as long as the metadata is availlable)
  raw_data generate_ui(const raw_data& rd, const rle::serialization_metadata& md);

  inline raw_data generate_ui(const raw_data& rd, const raw_data& md)
  {
    return generate_ui(rd, rle::deserialize<rle::serialization_metadata>(md));
  }

  // helpers:
  template<typename Type>
  raw_data generate_ui(const raw_data& rd)
  {
    return generate_ui(rd, rle::generate_metadata<Type>());
  }

  template<typename Type>
  raw_data generate_ui(const Type& value)
  {
    return generate_ui(rle::serialize<Type>(value), rle::generate_metadata<Type>());
  }

  namespace helpers
  {
    struct payload_t
    {
      rle::encoder ec;

      bool enabled = true;
      uint32_t disabled_table_stack = 0;
      uint32_t table_stack = 0;
      std::string member_name = {};

      const rle::type_reference* ref = nullptr;

      uint32_t id = 0;
    };
    using payload_arg_t = payload_t&;

    /// \brief Allow the type-helpers to go-back to the standard way to recurse over types
    /// \note This alternative \e will call type-helpers (beware of stack-verflows)
    void walk_type(const rle::serialization_metadata& md, const rle::type_metadata& type, const rle::type_reference& type_ref, rle::decoder& dc, payload_arg_t payload);

    /// \brief Allow the type-helpers to go-back to the standard way to recurse over types
    /// \note This alternative \e will \e not call type-helpers
    void walk_type_generic(const rle::serialization_metadata& md, const rle::type_metadata& type, rle::decoder& dc, payload_arg_t payload);


    using walk_type_fnc_t = void (*)(const rle::serialization_metadata& md, const rle::type_metadata& type, rle::decoder& dc, payload_arg_t payload);
    using get_type_name_fnc_t = std::string (*)(const rle::serialization_metadata& md, const rle::type_metadata& type);
    using on_type_raw_fnc_t = void(*)(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload, const uint8_t* addr, size_t size);


    struct type_helper_t
    {
      rle::type_metadata target_type;
      id_t helper_custom_id = id_t::none;
      walk_type_fnc_t walk_type = walk_type_generic;
      get_type_name_fnc_t type_name = nullptr;
    };

    struct raw_type_helper_t
    {
      rle::type_hash_t target_type;
      on_type_raw_fnc_t on_type_raw = nullptr;
    };

    /// \brief Add a new helper.
    void add_generic_ui_type_helper(const type_helper_t& h);
    void remove_generic_ui_type_helper(walk_type_fnc_t fnc);

    /// \brief Add a new raw-type helper.
    void add_generic_ui_raw_type_helper(const raw_type_helper_t& h);
    void remove_generic_ui_raw_type_helper(rle::type_hash_t target_type);

    /// \brief inherit from this class and:
    ///  - define walk_type (signature of walk_type_fnc_t, as static)
    ///  - define static rle::type_metadata get_type_metadata() that returns the type to attach to
    ///  - (optional) define get_type_name (signature of get_type_name_fnc_t, as static)
    ///
    /// See string_logger for a simple example. (it matches containers of char and display them as strings).
    template<typename Child>
    class auto_register_generic_ui_type_helper
    {
      public:
        static constexpr id_t get_custom_helper_id() { return id_t::none; }

      private:
        static void register_child()
        {
          add_generic_ui_type_helper(
            {
              Child::get_type_metadata(),
              Child::get_custom_helper_id(),
              &Child::walk_type,
              Child::get_type_name,
            });
        }
        static void unregister_child()
        {
          remove_generic_ui_type_helper(&Child::walk_type);
        }
        static inline struct raii_register
        {
          raii_register() { register_child(); }
          ~raii_register() { unregister_child(); }
        } _registerer;
        static_assert(&_registerer == &_registerer);
      public:
        static constexpr get_type_name_fnc_t get_type_name = nullptr;
    };

    template<typename Child>
    class auto_register_generic_ui_raw_type_helper
    {
      private:
        static void register_child()
        {
          add_generic_ui_raw_type_helper(
            {
              Child::get_type_hash(),
              &Child::on_type_raw,
            });
        }
        static void unregister_child()
        {
          remove_generic_ui_raw_type_helper(Child::get_type_hash());
        }
        static inline struct raii_register
        {
          raii_register() { register_child(); }
          ~raii_register() { unregister_child(); }
        } _registerer;
        static_assert(&_registerer == &_registerer);
    };
  }

  // ImGui elements for generic-ui
  // Can only be used inside generic-ui
  namespace generic_ui
  {
    /// \brief Push a new (unique) ID to imgui
    /// The normal PopID can be called
    /// \note Calling imgui PushID with payload.id is incorrect and WILL result in collisions
    void PushID(helpers::payload_arg_t payload);

    /// \brief Start a collapsing header. EndCollapsingHeader must be called, independently of what Begin returns
    bool BeginCollapsingHeader(helpers::payload_arg_t payload, const char* label);
    void EndCollapsingHeader(helpers::payload_arg_t payload);

    /// \brief Start a new table. EndEntryTable must be called, independently of what Begin returns
    bool BeginEntryTable(helpers::payload_arg_t payload, const char* name = "table", uint32_t count = 2);
    void EndEntryTable(helpers::payload_arg_t payload);

    /// \brief Generate the UI for a member name (including the help text/some of metadata handling)
    /// \note Uses payload.ref to generate the name.
    void member_name_ui(helpers::payload_arg_t payload, const rle::type_metadata& type);
  }

}
