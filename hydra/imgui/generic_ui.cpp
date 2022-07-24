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

#include <imgui.h>
#include <fmt/format.h>
#include <ntools/rle/walker.hpp>
#include <ntools/hash/hash.hpp>
#include <ntools/sys_utils.hpp>

#include "imgui.hpp"
#include "generic_ui.hpp"
#include "ui_elements.hpp"

namespace neam::hydra::imgui
{
  namespace generic_ui
  {
    void PushID(helpers::payload_arg_t payload)
    {
      // IMGUI doesn't really want to change its hash function to a proper hash function
      // So we have to scramble the bits before feeding the id to imgui
      ImGui::PushID(ct::murmur_scramble(payload.id++));
    }

    void member_name_ui(helpers::payload_arg_t payload, const rle::type_metadata& type)
    {
      if (!payload.member_name.empty())
      {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(payload.member_name.c_str());
        ImGui::SameLine();
        help_marker_fnc([&]()
        {
          ImGui::PushFont(get_font(bold));
          ImGui::TextUnformatted("type: ");
          switch_font_sameline(monospace_font|italic);
          ImGui::TextUnformatted(type.name.c_str());
          switch_font_pop_sameline();
          ImGui::NewLine();

          if (payload.ref != nullptr)
          {
            const auto info = payload.ref->attributes.get<metadata::info::metadata>();
            if (!info.description.empty())
            {
              ImGui::PushFont(get_font(bold));
              ImGui::TextUnformatted("description: ");
              switch_font_sameline(italic);
              ImGui::TextUnformatted(info.description.c_str());
              switch_font_pop_sameline();
              ImGui::NewLine();
            }
            if (!info.doc_url.empty())
            {
              ImGui::PushFont(get_font(bold));
              ImGui::TextUnformatted("documentation: ");
              switch_font_sameline(italic);
              link(info.doc_url, "link");
              switch_font_pop_sameline();
              ImGui::NewLine();
            }
          }
        }, "...");
      }
    }

    template<typename Fnc>
    static bool BeginEntry(helpers::payload_arg_t payload, Fnc&& fnc)
    {
      if (!payload.enabled)
      {
        ++payload.disabled_table_stack;
        return false;
      }

      ++payload.table_stack;
      payload.enabled = fnc();
      return payload.enabled;
    }

    template<typename Fnc>
    static void EndEntry(helpers::payload_arg_t payload, Fnc&& fnc)
    {
      if (payload.enabled)
      {
        --payload.table_stack;
        fnc();
        return;
      }

      if (payload.disabled_table_stack == 0)
        payload.enabled = true;
      else
        --payload.disabled_table_stack;
    }

    bool BeginEntryTable(helpers::payload_arg_t payload, const char* name, uint32_t count)
    {
      return BeginEntry(payload, [=, &payload]
      {
        bool ret = ImGui::BeginTable(name, count, ImGuiTableFlags_BordersInner | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings);
        if (ret)
          PushID(payload);
        return ret;
      });
    }
    void EndEntryTable(helpers::payload_arg_t payload)
    {
      EndEntry(payload, []
      {
        ImGui::PopID();
        ImGui::EndTable();
      });
    }
    bool BeginCollapsingHeader(helpers::payload_arg_t payload, const char* label)
    {
      return BeginEntry(payload, [label, &payload]
      {
        PushID(payload);
        bool ret = ImGui::CollapsingHeader(label);
        ImGui::PopID();
        return ret;
      });
    }
    void EndCollapsingHeader(helpers::payload_arg_t payload)
    {
      EndEntry(payload, [] {});
    }
  }

  template<uint32_t Count>
  static void bool_raw_type_helper(const rle::serialization_metadata& md, const rle::type_metadata& type, helpers::payload_arg_t payload, const uint8_t* addr, size_t size)
  {
    bool v[Count];
    memcpy(&v, addr, sizeof(bool)*Count);

    if (generic_ui::BeginEntryTable(payload))
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      generic_ui::member_name_ui(payload, type);
      ImGui::TableSetColumnIndex(1);

      for (uint32_t i = 0; i < Count; ++i)
      {
        ImGui::PushID(i);
        ImGui::Checkbox("", &v[i]);
        ImGui::SameLine();
        ImGui::PopID();
      }
    }
    generic_ui::EndEntryTable(payload);

    memcpy(payload.ec.allocate(sizeof(bool) * Count), &v, sizeof(bool) * Count);
  }

  template<typename T, typename VT, ImGuiDataType DT, uint32_t ComponentCount = 1, ct::string_holder Format = "">
  static void raw_type_helper(const rle::serialization_metadata& md, const rle::type_metadata& type, helpers::payload_arg_t payload, const uint8_t* addr, size_t size)
  {
    T v = *(const T*)addr;

    if (generic_ui::BeginEntryTable(payload))
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      generic_ui::member_name_ui(payload, type);
      ImGui::TableSetColumnIndex(1);
      ImGui::PushItemWidth(-FLT_MIN);
      constexpr uint32_t format_len = sizeof(Format.string);
      constexpr bool is_hex = (format_len > 2
                               && (Format.string[std::max(2u, format_len) - 2] == 'X'
                                   || Format.string[std::max(2u, format_len) - 2] == 'x'));
      if constexpr (is_hex)
      {
        ImGui::TextUnformatted("");
        switch_font_sameline(monospace_font, false);
        ImGui::TextUnformatted("0x");
        ImGui::SameLine();
      }
      const bool has_range = payload.ref != nullptr && payload.ref->attributes.has<metadata::range<VT>>();
      if (has_range)
      {
        const auto range = payload.ref->attributes.get<metadata::range<VT>>();
        if (range.min != range.max)
        {
          if (((range.max - range.min) / (range.step == 0 ? 1 : range.step)) < VT(120))
            ImGui::SliderScalarN("", DT, &v, ComponentCount, &range.min, &range.max, format_len > 2 ? Format.string : nullptr);
          else
            ImGui::DragScalarN("", DT, &v, ComponentCount, range.step > 0 ? range.step : 1, &range.min, &range.max, format_len > 2 ? Format.string : nullptr);
        }
        else
        {
          ImGui::DragScalarN("", DT, &v, ComponentCount, range.step > 0 ? range.step : 1, nullptr, nullptr, format_len > 2 ? Format.string : nullptr);
        }
      }
      else
      {
        ImGui::DragScalarN("", DT, &v, ComponentCount, 1, nullptr, nullptr, format_len > 2 ? Format.string : nullptr);
      }
      if constexpr (is_hex)
      {
        switch_font_pop_sameline();
      }
    }
    generic_ui::EndEntryTable(payload);

    memcpy(payload.ec.allocate(sizeof(T)), &v, sizeof(T));
  }

  template<typename T>
  static void raw_type_helper_char(const rle::serialization_metadata& md, const rle::type_metadata& type, helpers::payload_arg_t payload, const uint8_t* addr, size_t size)
  {
    if (generic_ui::BeginEntryTable(payload))
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      generic_ui::member_name_ui(payload, type);
      ImGui::TableSetColumnIndex(1);
      // FIXME
//       memcpy(payload.ec.allocate(type.size), &v, type.size);
    }
    generic_ui::EndEntryTable(payload);
    memcpy(payload.ec.allocate(sizeof(T)), addr, sizeof(T));
  }

  static std::vector<helpers::type_helper_t> type_helpers;

#define N_GUI_RAW_TYPE_BASE(Type, VectType, IGDT) \
  {rle::serialization_metadata::hash_of<Type>(), &raw_type_helper<Type, VectType, ImGuiDataType_##IGDT>}

#define N_GUI_RAW_TYPE_VECT_N(Type, Comp, IGDT) \
  {rle::serialization_metadata::hash_of<glm::vec<Comp, Type, glm::qualifier::packed_highp>>(), &raw_type_helper<glm::vec<Comp, Type, glm::qualifier::packed_highp>, Type, ImGuiDataType_##IGDT, Comp>}, \
  {rle::serialization_metadata::hash_of<glm::vec<Comp, Type, glm::qualifier::packed_mediump>>(), &raw_type_helper<glm::vec<Comp, Type, glm::qualifier::packed_mediump>, Type, ImGuiDataType_##IGDT, Comp>}, \
  {rle::serialization_metadata::hash_of<glm::vec<Comp, Type, glm::qualifier::packed_lowp>>(), &raw_type_helper<glm::vec<Comp, Type, glm::qualifier::packed_lowp>, Type, ImGuiDataType_##IGDT, Comp>}
#define N_GUI_RAW_TYPE_VECT(Type, IGDT) \
  N_GUI_RAW_TYPE_VECT_N(Type, 1, IGDT), \
  N_GUI_RAW_TYPE_VECT_N(Type, 2, IGDT), \
  N_GUI_RAW_TYPE_VECT_N(Type, 3, IGDT), \
  N_GUI_RAW_TYPE_VECT_N(Type, 4, IGDT)

#define N_GUI_RAW_TYPE(Type, IGDT) \
  N_GUI_RAW_TYPE_BASE(Type, Type, IGDT), \
  N_GUI_RAW_TYPE_VECT(Type, IGDT)

  static std::map<rle::type_hash_t, helpers::on_type_raw_fnc_t> raw_type_helpers =
  {
    {rle::serialization_metadata::hash_of<bool>(), bool_raw_type_helper<1>},
    // glm bool vectors:
    {rle::serialization_metadata::hash_of<glm::vec<2, bool, glm::qualifier::packed_highp>>(), bool_raw_type_helper<2>},
    {rle::serialization_metadata::hash_of<glm::vec<2, bool, glm::qualifier::packed_mediump>>(), bool_raw_type_helper<2>},
    {rle::serialization_metadata::hash_of<glm::vec<2, bool, glm::qualifier::packed_lowp>>(), bool_raw_type_helper<2>},
    {rle::serialization_metadata::hash_of<glm::vec<3, bool, glm::qualifier::packed_highp>>(), bool_raw_type_helper<3>},
    {rle::serialization_metadata::hash_of<glm::vec<3, bool, glm::qualifier::packed_mediump>>(), bool_raw_type_helper<3>},
    {rle::serialization_metadata::hash_of<glm::vec<3, bool, glm::qualifier::packed_lowp>>(), bool_raw_type_helper<3>},
    {rle::serialization_metadata::hash_of<glm::vec<4, bool, glm::qualifier::packed_highp>>(), bool_raw_type_helper<4>},
    {rle::serialization_metadata::hash_of<glm::vec<4, bool, glm::qualifier::packed_mediump>>(), bool_raw_type_helper<4>},
    {rle::serialization_metadata::hash_of<glm::vec<4, bool, glm::qualifier::packed_lowp>>(), bool_raw_type_helper<4>},

    N_GUI_RAW_TYPE(uint8_t, U8), N_GUI_RAW_TYPE(int8_t, S8),
    N_GUI_RAW_TYPE(uint16_t, U16), N_GUI_RAW_TYPE(int16_t, S16),
    N_GUI_RAW_TYPE(uint32_t, U32), N_GUI_RAW_TYPE(int32_t, S32),
    N_GUI_RAW_TYPE(uint64_t, U64), N_GUI_RAW_TYPE(int64_t, S64),

//     N_GUI_RAW_TYPE_CHAR(unsigned char), N_GUI_RAW_TYPE_CHAR(char),

    N_GUI_RAW_TYPE(unsigned short, U8), N_GUI_RAW_TYPE(short, S8),
    N_GUI_RAW_TYPE(unsigned int, U16), N_GUI_RAW_TYPE(int, S16),
    N_GUI_RAW_TYPE(unsigned long, U32), N_GUI_RAW_TYPE(long, S32),
    N_GUI_RAW_TYPE(unsigned long long, U64), N_GUI_RAW_TYPE(long long, S64),

    N_GUI_RAW_TYPE(float, Float), N_GUI_RAW_TYPE(double, Double),
  };
#undef N_GUI_RAW_TYPE
#undef N_GUI_RAW_TYPE_VECT

  void helpers::add_generic_ui_type_helper(const type_helper_t& h)
  {
    type_helpers.push_back(h);
  }
  void helpers::add_generic_ui_raw_type_helper(const raw_type_helper_t& h)
  {
    raw_type_helpers.emplace(h.target_type, h.on_type_raw);
  }

  struct generic_ui_walker : public rle::empty_walker<helpers::payload_arg_t>
  {
    struct encoder_swap_t
    {
      std::optional<cr::memory_allocator> oma;
      std::optional<rle::encoder> oec;
    };
    struct container_edit_t
    {
      encoder_swap_t est;

      uint32_t resize_to;
    };
    static uint32_t get_type_helper_count() { return (uint32_t)type_helpers.size(); }
    static helpers::type_helper_t* get_type_helper(uint32_t index) { return &type_helpers[index]; }

    static void on_type_raw(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload, const uint8_t* addr, size_t size)
    {
      if (const auto it = raw_type_helpers.find(type.hash); it != raw_type_helpers.end())
      {
        it->second(md, type, payload, addr, size);
        return;
      }

      // not found: generic output:
      switch (size)
      {
        case 1: raw_type_helper<uint8_t, uint8_t, ImGuiDataType_U8, 1, "%02X">(md, type, payload, addr, size); break;
        case 2: raw_type_helper<uint16_t, uint16_t, ImGuiDataType_U16, 1, "%04X">(md, type, payload, addr, size); break;
        case 4: raw_type_helper<uint32_t, uint32_t, ImGuiDataType_U32, 1, "%08X">(md, type, payload, addr, size); break;
        case 8: raw_type_helper<uint64_t, uint64_t, ImGuiDataType_U64, 1, "%016lX">(md, type, payload, addr, size); break;
        default:
        {
          if (generic_ui::BeginEntryTable(payload))
          {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            generic_ui::member_name_ui(payload, type);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("unknown %u byte data", (uint32_t)size);
            memcpy(payload.ec.allocate(type.size), addr, size);
          }
          generic_ui::EndEntryTable(payload);
        }
        break;
      }
    }

    static container_edit_t on_type_container_pre(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload,
                                      uint32_t count, const rle::type_metadata& sub_type)
    {
      if (generic_ui::BeginCollapsingHeader(payload, payload.member_name.c_str()))
      {
        generic_ui::PushID(payload);
        ImGui::Indent();
        ImGui::Text("size:");
        ImGui::SameLine();
        uint32_t one = 1;
        ImGui::InputScalar("##size", ImGuiDataType_U32, &count, &one);
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
          count = 0;
        ImGui::Separator();
        ImGui::PopID();
        payload.ec.encode(count);
      }
      else
      {
        payload.ec.encode(count);
      }
      return { .est = {}, .resize_to = count };
    }
    static void on_type_container_post(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload,
                                       uint32_t count, const rle::type_metadata& sub_type, container_edit_t& ed)
    {
      if (payload.enabled)
        ImGui::Unindent();
      generic_ui::EndCollapsingHeader(payload);
      while (ed.resize_to > 0) // add missing elements:
      {
        --ed.resize_to;
        sub_type.get_default_value(md, payload.ec);
      }
    }
    static void on_type_container_pre_entry(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload,
                                            uint32_t index, uint32_t count, const rle::type_metadata& sub_type, container_edit_t& ed)
    {
      payload.member_name = fmt::format("[{}]", index);
      payload.ref = nullptr;

      if (ed.resize_to > 0)
      {
        --ed.resize_to;
      }
      else
      {
        // we got an element that is being removed, skip it:
        ed.est = { cr::memory_allocator{}, payload.ec };
        payload.ec = {*ed.est.oma};
      }
    }
    static void on_type_container_post_entry(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload,
                                             uint32_t index, uint32_t count, const rle::type_metadata& sub_type, container_edit_t& ed)
    {
      if (ed.est.oec)
      {
        payload.ec = *ed.est.oec;
        ed.est = {};
      }
    }



    static void on_type_tuple_version(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload, uint32_t version)
    {
      payload.ec.encode(version);
    }

    static bool on_type_tuple_pre(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload, uint32_t count)
    {
      bool initial = payload.id == 0;
      if (count == 0) return initial;
      if (!payload.enabled) return initial;

      generic_ui::member_name_ui(payload, type);

      if (!initial)
        ImGui::Indent();
      return initial;
    }
    static void on_type_tuple_post(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload, uint32_t count, bool initial)
    {
      if (count == 0) return;
      if (!payload.enabled) return;

      if (!initial)
        ImGui::Unindent();
    }
    static void on_type_tuple_pre_entry(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload,
                                        uint32_t index, uint32_t count, const rle::type_metadata& sub_type, bool initial)
    {
      payload.member_name = type.contained_types[index].name;
      payload.ref = &type.contained_types[index];
    }
    static void on_type_tuple_post_entry(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload,
                                         uint32_t index, uint32_t count, const rle::type_metadata& sub_type, bool initial)
    {
      payload.ref = nullptr;
    }

    static void on_type_variant_empty(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload)
    {

      if (generic_ui::BeginEntryTable(payload))
      {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        generic_ui::member_name_ui(payload, type);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("type:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        std::vector<const char*> variant;
        variant.reserve(type.contained_types.size() + 1);
        variant.push_back("[empty]");
        for (const auto& it : type.contained_types)
        {
          variant.push_back(md.type(it.hash).name.c_str());
        }
        int index = 0;
        const bool changed = ImGui::Combo("", (int*)&index, variant.data(), variant.size());
        payload.ec.encode(uint32_t(index));
        if (changed && index > 0)
        {
          // insert the default value for the new entry:
          md.type(type.contained_types[index - 1].hash).get_default_value(md, payload.ec);
        }
      }
      else
      {
        payload.ec.encode(uint32_t(0));
      }
      generic_ui::EndEntryTable(payload);
      payload.member_name = {};
      payload.ref = nullptr;
    }
    static encoder_swap_t on_type_variant_pre_entry(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload,
        uint32_t index, const rle::type_metadata& sub_type)
    {
      encoder_swap_t es;
      bool encoded = false;
      if (generic_ui::BeginEntryTable(payload))
      {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        generic_ui::member_name_ui(payload, type);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("type:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        std::vector<const char*> variant;
        variant.reserve(type.contained_types.size() + 1);
        variant.push_back("[empty]");
        for (const auto& it : type.contained_types)
        {
          variant.push_back(md.type(it.hash).name.c_str());
        }
        if (ImGui::Combo("variant type:", (int*)&index, variant.data(), variant.size()))
        {
          encoded = true;
          payload.ec.encode(index);
          if (index > 0)
          {
            // insert the default value for the new entry:
            md.type(type.contained_types[index - 1].hash).get_default_value(md, payload.ec);
          }
          // insert a dummy encoder:
          es.oec = payload.ec;
          es.oma = cr::memory_allocator{};
          payload.ec = {*es.oma};
        }
      }
      generic_ui::EndEntryTable(payload);
      if (!encoded)
      {
        payload.ec.encode(index);
      }

      payload.member_name = {};
      payload.ref = nullptr;
      return es;
    }
    static void on_type_variant_post_entry(const rle::serialization_metadata& md, const rle::type_metadata& type, payload_arg_t payload,
                                           uint32_t index, const rle::type_metadata& sub_type, encoder_swap_t& est)
    {
      if (est.oec)
      {
        payload.ec = *est.oec;
      }
    }
  };

  void helpers::walk_type(const rle::serialization_metadata& md, const rle::type_metadata& type, rle::decoder& dc, payload_arg_t payload)
  {
    return rle::walker<generic_ui_walker>::walk_type(md, type, dc, payload);
  }

  void helpers::walk_type_generic(const rle::serialization_metadata& md, const rle::type_metadata& type, rle::decoder& dc, payload_arg_t payload)
  {
    return rle::walker<generic_ui_walker>::walk_type_generic(md, type, dc, payload);
  }

  raw_data generate_ui(const raw_data& rd, const rle::serialization_metadata& md)
  {
    cr::memory_allocator ma;
    helpers::payload_t payload
    {
      .ec = {ma},
    };
    rle::walker<generic_ui_walker>::walk(rd, md, payload);
    return payload.ec.to_raw_data();
  }


  // builtin helpers:


  struct generic_ui_string : helpers::auto_register_generic_ui_type_helper<generic_ui_string>
  {
    static rle::type_metadata get_type_metadata()
    {
      return rle::type_metadata::from(rle::type_mode::container, {{rle::serialization_metadata::hash_of<char>()}});
    }

    static void walk_type(const rle::serialization_metadata& md, const rle::type_metadata& type, rle::decoder& dc, helpers::payload_arg_t payload)
    {
      uint32_t count = dc.decode<uint32_t>().first;
      static constexpr uint32_t k_buff_size = 512;
      char buff[k_buff_size] = {0};
      memcpy(buff, dc.get_address<char>(), count);
      buff[count] = 0;
      dc.skip(count);

      if (!payload.enabled)
      {
        memcpy(payload.ec.encode_and_alocate(count), buff, count);
        return;
      }

      if (generic_ui::BeginEntryTable(payload))
      {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        generic_ui::member_name_ui(payload, type);
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::InputText("", buff, k_buff_size);
      }
      generic_ui::EndEntryTable(payload);
      count = strlen(buff);
      memcpy(payload.ec.encode_and_alocate(count), buff, count);
    }
  };
}

