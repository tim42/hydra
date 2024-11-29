//
// created by : Timothée Feuillet
// date: 2023-4-22
//
//
// Copyright (c) 2023 Timothée Feuillet
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

#include <filesystem>
#include <vector>

#include <imgui.h>
#include <hydra/imgui/imgui.hpp>

#include <ntools/fs_utils.hpp>
#include <ntools/event.hpp>

namespace neam::hydra::imgui
{
  class folder_view
  {
    public:
      enum class mode_t
      {
        /// \brief Simple, one line per file table view. May allow for more information to be shown (like size and type).
        list,

        /// \brief Show rectangular icons for files and folders. Might include previews.
        /// \note previews are not done yet.
        /// \note mode not done yet.
        icons,

        /// \brief The default mode
        default_mode = list
      } mode = mode_t::default_mode;

      /// \brief Root path. Will not allow to go below this.
      std::filesystem::path root = "/";

      /// \brief Current folder in the view. (relative to root)
      std::filesystem::path cwd;

      /// \brief Triggered once, when a file/folder is selected. The path is the absolute path to that file.
      cr::event<std::filesystem::path> on_selected;

      uint32_t extra_columns = 0;
      std::function<void(const std::filesystem::directory_entry& entry)> entry_extra_ui;

      /// \brief Render the folder view
      /// \note This is not a window, but a window child. The caller is responsible to create the parent window
      void render(ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar)
      {
        if (ImGui::BeginChild("imgui::folder_view", {0,0}, false, flags))
        {
          render_header();

          switch (mode)
          {
            case mode_t::list:
              render_list();
              break;
            case mode_t::icons:
              render_icons();
              break;
          }
        }
        ImGui::EndChild();
      }

    private:
      struct dir_entry_t
      {
        std::filesystem::directory_entry entry;
        std::filesystem::file_status status;
        std::filesystem::file_status entry_status;
      };


      std::vector<dir_entry_t> get_dir_entries() const
      {
        std::vector<dir_entry_t> ret;
        ret.reserve(100);

        std::error_code c;
        const std::filesystem::path current_full_dir = (root / cwd).lexically_normal();

        for (auto const& dir_entry : std::filesystem::directory_iterator(current_full_dir, std::filesystem::directory_options::follow_directory_symlink | std::filesystem::directory_options::skip_permission_denied, c))
        {
          if (c)
          {
            c.clear();
            continue;
          }

          std::filesystem::file_status status = dir_entry.status();
          const std::filesystem::file_status entry_status = status;

          if (dir_entry.is_symlink())
            status = dir_entry.symlink_status();

          ret.push_back(
          {
            .entry = dir_entry,
            .status = status,
            .entry_status = entry_status,
          });
        }
        return ret;
      }

      void render_dir_split_entry(const std::string& name, const std::filesystem::path& rel)
      {
        ImVec4 text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        ImGui::TextUnformatted(name.c_str());
        ImVec2 rect_min = ImGui::GetItemRectMin();
        ImVec2 rect_max = ImGui::GetItemRectMax();
        rect_max.y -= (rect_max.y - rect_min.y) * 0.1f;
        rect_min.y = rect_max.y;
        if (ImGui::IsItemClicked())
        {
          cwd = rel;
        }
        if (ImGui::IsItemHovered())
        {
          ImGui::GetWindowDrawList()->AddLine(rect_min, rect_max, ImGui::GetColorU32(text_color), 1.0f);
          ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        ImGui::SameLine();
        ImGui::TextUnformatted(" / ");
        ImGui::SameLine();
      }

      void render_header()
      {
        ImGui::PushFont(hydra::imgui::get_font(default_font | bold));

        if (ImGui::Button("<"))
          cwd = cwd.parent_path();
        ImGui::SameLine();

        std::filesystem::path acc;

        render_dir_split_entry("[root]", acc);

        const std::filesystem::path cwd_cp = cwd;
        for (auto&& it : cwd_cp)
        {
          acc /= it;
          render_dir_split_entry(it, acc);
        }

        ImGui::TextUnformatted(" ");
        ImGui::Separator();
        ImGui::PopFont();
      }

      void render_list()
      {
        if (!ImGui::BeginTable("FolderViewTable", 2 + extra_columns, ImGuiTableFlags_BordersInner | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoSavedSettings))
          return;

        const std::vector<dir_entry_t> entries = get_dir_entries();

        ImGuiListClipper clipper;
        clipper.Begin((int)entries.size());
        while (clipper.Step())
        {
          for (int entry_no = clipper.DisplayStart; entry_no < clipper.DisplayEnd; entry_no++)
          {
            const dir_entry_t& entry = entries[entry_no];

            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            std::string label;
            switch (entry.status.type())
            {
              case std::filesystem::file_type::regular:
                label = "F";
                break;
              case std::filesystem::file_type::directory:
                label = "D";
                break;
              default:
                label = "?";
                break;
            }
            std::string filename = ((std::filesystem::path)entry.entry).filename();
            bool is_clicked = ImGui::Selectable(("##" + filename).c_str(), false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick);
            ImGui::SameLine();
            ImGui::TextUnformatted(label.c_str());
            ImGui::TableNextColumn();

            ImGui::TextUnformatted(filename.c_str());

            if (is_clicked)
            {
              if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && entry.status.type() == std::filesystem::file_type::directory)
              {
                cwd /= ((std::filesystem::path)entry.entry).filename();
              }
              else if (!ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
              {
                on_selected(((std::filesystem::path)entry.entry).lexically_normal());
              }
            }

            if (extra_columns && entry_extra_ui)
            {
              entry_extra_ui(entry.entry);
            }
          }
        }
        clipper.End();

        ImGui::EndTable();
      }

      void render_icons()
      {
        ImGui::Text("Icon mode is not Done Yet!");
      }
  };
}

