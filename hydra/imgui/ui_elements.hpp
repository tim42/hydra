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

#pragma once

#include <fmt/format.h>
#include <string>
#include <imgui.h>

namespace neam::hydra::imgui
{
  // indices:
  static constexpr uint32_t regular = 0;
  static constexpr uint32_t bold = 1;
  static constexpr uint32_t italic = 2;
  static constexpr uint32_t bold_italic = bold | italic;
  static constexpr uint32_t _mode_count = 4;

  // font types:
  static constexpr uint32_t default_font = 0 * _mode_count;
  static constexpr uint32_t monospace_font = 1 * _mode_count;
  static constexpr uint32_t _font_count = 2 * _mode_count;

  /// \brief Return the font corresponding to the given flags
  static ImFont* get_font(uint32_t idx)
  {
    // first, try to fallback to the default_font, keeping the same mode
    if (idx >= (uint32_t)ImGui::GetIO().Fonts->Fonts.size())
      idx &= (_mode_count - 1);
    // secondly, fallback to the default_font without keeping the mode (it means don't have a font familly loaded)
    if (idx >= (uint32_t)ImGui::GetIO().Fonts->Fonts.size())
      idx = 0;
    return ImGui::GetIO().Fonts->Fonts[idx];
  }

  /// \brief Switch font (pop current + push new one) and adjust the vertical position if the size are not the same
  /// \note You have to call switch_font_pop() + ImGui::NewLine() to correctly get a new line
  static void switch_font_sameline(uint32_t idx, bool pop = true)
  {
    const float lh = ImGui::GetTextLineHeight();
    if (pop)
      ImGui::PopFont();
    ImGui::SameLine();
    ImGui::PushFont(get_font(idx));
    const float nlh = ImGui::GetTextLineHeight();
    const float cpy = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(cpy + (lh-nlh)*0.75f);
  }
  static void switch_font_pop_sameline()
  {
    const float lh = ImGui::GetTextLineHeight();
    ImGui::PopFont();
    ImGui::SameLine();
    const float nlh = ImGui::GetTextLineHeight();
    const float cpy = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(cpy - (lh-nlh)*0.75f);
  }
  /// \brief Create a link to [something] (be it a http/s link or a path to a file).
  /// Clicking on it will open the corresponding application (default web browser, ...)
  void link(const std::string& url, const std::string& text);

  /// \brief Shorter version of the link call, where the text is also the url
  static void link(const std::string& url)
  {
    link(url, url);
  }

  /// \brief
  template<typename Fnc>
  static void help_marker_fnc(Fnc&& fnc, const std::string& help_text = "(?)")
  {
    switch_font_sameline(monospace_font | italic, false);
    ImGui::TextDisabled("%s", help_text.c_str());
    switch_font_pop_sameline();
    ImGui::NewLine();
    if (ImGui::IsItemClicked())
    {
      ImGui::OpenPopup("##help_text");
    }
    else if (!ImGui::IsPopupOpen("##help_text") && ImGui::IsItemHovered())
    {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      fnc();
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
    if (ImGui::BeginPopup("##help_text"))
    {
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      fnc();
      ImGui::PopTextWrapPos();
      ImGui::EndPopup();
    }
  }
}
namespace ImGui
{
  /// \brief Same as text, but use fmt::format
  template<typename... Args>
  void TextFmt(fmt::format_string<Args...> fmt, Args&&... args)
  {
    auto ret = fmt::format(fmt, std::forward<Args>(args)...);
    ImGui::TextUnformatted(&ret.front(), &ret.back() + 1u);
  }

}

