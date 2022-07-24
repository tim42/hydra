//
// created by : Timothée Feuillet
// date: 2022-7-16
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

#include <ntools/sys_utils.hpp>

#include "ui_elements.hpp"

namespace neam::hydra::imgui
{
  void link(const std::string& url, const std::string& text)
  {
    ImVec4 text_color = ImVec4(0.2f, 0.5f, 1.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    ImGui::TextUnformatted(text.c_str());
    ImGui::PopStyleColor();
    ImVec2 rect_min = ImGui::GetItemRectMin();
    ImVec2 rect_max = ImGui::GetItemRectMax();
    rect_max.y -= (rect_max.y - rect_min.y) * 0.1f; 
    rect_min.y = rect_max.y;
    if (ImGui::IsItemClicked())
    {
      sys::open_url(url);
    }
    else if (ImGui::IsItemHovered())
    {
      ImGui::GetWindowDrawList()->AddLine(rect_min, rect_max, ImGui::GetColorU32(text_color), 1.0f);
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
      if (&url != &text)
      {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::PushFont(hydra::imgui::get_font(hydra::imgui::default_font));
        ImGui::Text("link to: %s", url.c_str());
        ImGui::PopFont();
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
      }
    }
  }
}

