//
// created by : Timothée Feuillet
// date: Sat May 29 2021 16:17:47 GMT-0400 (EDT)
//
//
// Copyright (c) 2021 Timothée Feuillet
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

#include <imgui.h>
#include <ntools/mt_check/deque.hpp>
#include <ntools/logger/logger.hpp>

namespace neam
{
  class imgui_log_window
  {
  public:
    imgui_log_window()
    {
      cr::get_global_logger().register_callback(_stream_callback, this);
    }

    ~imgui_log_window()
    {
      cr::get_global_logger().unregister_callback(_stream_callback, this);
    }

    void clear()
    {
      std::lock_guard _lg { lock };
      entries.clear();
    }

    void show_log_window()
    {
        std::lock_guard _lg { lock };
        const ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;

        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (work_pos.x);
        window_pos.y = (work_pos.y + work_size.y);
        window_pos_pivot.x = 0.0f;
        window_pos_pivot.y = 1.0f;

//         const float font_size = ImGui::GetFontSize();
//         ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
//         ImGui::SetNextWindowSize(ImVec2(work_size.x, 16 * font_size), ImGuiCond_Always);
        if (ImGui::Begin("Log", nullptr, window_flags))
        {
          bool do_clear = ImGui::Button("Clear");
          ImGui::SameLine();
          bool do_copy = ImGui::Button("Copy");
          ImGui::SameLine();
          ImGui::Checkbox("Auto-scroll", &auto_scroll);

          ImGui::Separator();

          if (do_clear)
            clear();
          if (do_copy)
            ImGui::LogToClipboard();

          ImGui::BeginChild("##scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
          ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
          ImGui::PushFont(hydra::imgui::get_font(hydra::imgui::monospace_font));

          ImGuiListClipper clipper;
          clipper.Begin((int)entries.size());
          while (clipper.Step())
          {
            for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
            {
              const char* line_start = entries[line_no].msg.data();
              const char* line_end = entries[line_no].msg.data() + entries[line_no].msg.size();
              ImGui::PushStyleColor(ImGuiCol_Text, entries[line_no].color);
              ImGui::TextUnformatted(line_start, line_end);
              ImGui::PopStyleColor();
            }
          }
          clipper.End();
          ImGui::PopFont();
          ImGui::PopStyleVar();

          if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1);
          ImGui::EndChild();
        }
        ImGui::End();
    }

    private:
      void on_new_entry(neam::cr::logger::severity s, const std::string& msg, std::source_location loc)
      {
        std::lock_guard _lg { lock };
        const std::string new_entry = neam::cr::format_log_to_string(s, msg, loc);

        if (entries.size() >= max_count)
          entries.pop_front();
        ImVec4 color;
//         ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_Text];
        switch (s)
        {
          case neam::cr::logger::severity::debug:
            color = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
            break;
          case neam::cr::logger::severity::message:
            color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            break;
          case neam::cr::logger::severity::warning:
            color = ImVec4(1.00f, 0.72f, 0.00f, 1.0f);
            break;
          case neam::cr::logger::severity::error:
          case neam::cr::logger::severity::critical:
            color = ImVec4(1.00f, 0.05f, 0.00f, 1.0f);
            break;
        };
        entries.push_back({ color, new_entry});
      }

      static void _stream_callback(void* self, neam::cr::logger::severity s, const std::string& msg, std::source_location loc)
      {
        reinterpret_cast<imgui_log_window*>(self)->on_new_entry(s, msg, loc);
      }

      struct entry_t
      {
        ImVec4 color;
        std::string msg;
      };
      spinlock lock;
      std::mtc_deque<entry_t> entries;

      bool auto_scroll = true;

      static constexpr size_t max_count = 10000;
  };
}
