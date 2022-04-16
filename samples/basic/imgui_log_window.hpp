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

#include <sstream>
#include <imgui.h>
#include "hydra/tools/logger/logger.hpp"

namespace neam
{
  class imgui_log_window
  {
  public:
    imgui_log_window()
    {
      cr::out.add_stream(log_stream);
      cr::out.get_multiplexed_stream().add_callback(_stream_callback, this);
    }

    ~imgui_log_window()
    {
      cr::out.get_multiplexed_stream().remove_callback(_stream_callback, this);
      cr::out.remove_stream(log_stream);
    }

    void clear()
    {
      entries.clear();
    }

    void show_log_window()
    {
        const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;

        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (work_pos.x);
        window_pos.y = (work_pos.y + work_size.y);
        window_pos_pivot.x = 0.0f;
        window_pos_pivot.y = 1.0f;

        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowSize(ImVec2(work_size.x, 400), ImGuiCond_Always);
        if (ImGui::Begin("Log", nullptr, window_flags))
        {
          bool do_clear = ImGui::Button("Clear");
          ImGui::SameLine();
          bool do_copy = ImGui::Button("Copy");
          ImGui::SameLine();
          ImGui::Checkbox("Auto-scroll", &auto_scroll);

          ImGui::Separator();
          ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

          if (do_clear)
            clear();
          if (do_copy)
            ImGui::LogToClipboard();

          ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
          ImGui::PopStyleVar();

          ImGuiListClipper clipper;
          clipper.Begin((int)entries.size());
          while (clipper.Step())
          {
            for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
            {
              const char* line_start = entries[line_no].data();
              const char* line_end = entries[line_no].data() + entries[line_no].size();
              ImGui::TextUnformatted(line_start, line_end);
            }
          }
          clipper.End();

          if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
          ImGui::EndChild();
          ImGui::End();

        }
    }

  private:
    void on_new_entry()
    {
      const std::string new_entry = log_stream.str();
      log_stream = std::ostringstream{};

      if (entries.size() >= max_count)
        entries.pop_front();
      entries.push_back(new_entry);
    }

    static void _stream_callback(void* self)
    {
      reinterpret_cast<imgui_log_window*>(self)->on_new_entry();
    }

    std::ostringstream log_stream;
    std::deque<std::string> entries;

    bool auto_scroll = true;

    static constexpr size_t max_count = 10000;
  };
}