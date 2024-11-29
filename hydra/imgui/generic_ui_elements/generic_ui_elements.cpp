//
// created by : Timothée Feuillet
// date: 2024-3-17
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

#include <hydra/imgui/imgui.hpp>
#include <hydra/imgui/generic_ui.hpp>
#include <ntools/c_array.hpp>

namespace neam::hydra::imgui
{
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

      cr::soft_c_array buff = cr::soft_c_array<char, k_buff_size>::create_with_size(count + 64);

      memcpy(buff.begin(), dc.get_address<char>(), count);
      buff[count] = 0;
      dc.skip(count);

      if (!payload.enabled)
      {
        memcpy(payload.ec.encode_and_alocate(count), buff.begin(), count);
        return;
      }

      if (generic_ui::BeginEntryTable(payload))
      {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        generic_ui::member_name_ui(payload, type);
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-FLT_MIN);
        const float lineSize = ImGui::GetFontSize();

        const uint32_t eol_count = std::count(buff.begin(), buff + count, '\n');

        ImGui::InputTextMultiline("", buff.begin(), buff.size(), ImVec2(0, lineSize * 1.25f + lineSize * eol_count), ImGuiInputTextFlags_NoUndoRedo);
      }
      generic_ui::EndEntryTable(payload);
      count = strlen(buff.begin());
      memcpy(payload.ec.encode_and_alocate(count), buff.begin(), count);
    }
  };

  struct generic_ui_id_t : helpers::auto_register_generic_ui_type_helper<generic_ui_id_t>
  {
    static rle::type_metadata get_type_metadata()
    {
      return rle::type_metadata::from(ct::type_hash<id_t>);
    }

    static void walk_type(const rle::serialization_metadata& md, const rle::type_metadata& type, rle::decoder& dc, helpers::payload_arg_t payload)
    {
#if N_STRIP_DEBUG
      return helpers::walk_type_generic(md, type, dc, payload);
#else
      const id_t ui_id = *dc.get_address<id_t>();
      const std::string_view ui_sid = string_id::_from_id_t(ui_id).get_string_view();

      // fallback:
      if (ui_sid.data() == nullptr || ui_sid.size() == 0)
        return helpers::walk_type_generic(md, type, dc, payload);

      // skip:
      dc.skip(sizeof(id_t));
      if (!payload.enabled)
      {
        payload.ec.encode<uint64_t>((uint64_t)ui_id);
        return;
      }

      static constexpr uint32_t k_buff_size = 512;
      cr::soft_c_array buff = cr::soft_c_array<char, k_buff_size>::create_with_size(ui_sid.size() + 64);
      memcpy(buff.data(), ui_sid.data(), ui_sid.size());
      buff[ui_sid.size()] = 0;


      if (generic_ui::BeginEntryTable(payload))
      {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        generic_ui::member_name_ui(payload, type);
        ImGui::TableSetColumnIndex(1);
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::InputText("", buff.begin(), k_buff_size, ImGuiInputTextFlags_NoUndoRedo);
      }
      generic_ui::EndEntryTable(payload);
      const uint32_t count = strlen(buff.begin());
      payload.ec.encode<uint64_t>((uint64_t)(id_t)string_id::_runtime_build_from_string(buff.begin(), count));
#endif
    }
  };
}
