//
// created by : Timothée Feuillet
// date: 2022-7-22
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

#include <filesystem>
#include <thread>
#include <vector>

#include <ntools/id/id.hpp>
#include <ntools/struct_metadata/struct_metadata.hpp>

namespace neam
{
  struct global_options
  {
    // options
    bool verbose = false;
    bool silent = false;
    bool force = false;
    bool watch = true;
    bool help = false;
    bool ui = true;
    bool print_source_name = false;
    uint32_t watch_delay = 2;
    uint32_t thread_count = std::thread::hardware_concurrency() - 4;

    std::vector<std::string_view> parameters;

    // extra: (from parameters)
    id_t index_key = id_t::none;
    std::filesystem::path data_folder = std::filesystem::current_path();

    std::filesystem::path source_folder;
    std::filesystem::path build_folder;
    std::filesystem::path index;
    std::filesystem::path ts_file;
  };
}
N_METADATA_STRUCT(neam::global_options)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(help, neam::metadata::info{.description = c_string_t<"Print this message and exit.">}),
    N_MEMBER_DEF(verbose, neam::metadata::info{.description = c_string_t<"Show debug messages. May be extremly verbose.">}),
    N_MEMBER_DEF(silent, neam::metadata::info{.description = c_string_t<"Only show warning (and above) messages.">}),
    N_MEMBER_DEF(force, neam::metadata::info{.description = c_string_t<"Force rebuild the index and repack all the resources.">}),
    N_MEMBER_DEF(watch, neam::metadata::info{.description = c_string_t<"Watch for filesystem changes and repack those resources.\nIf false, will exit as soon as there's no more operations left to do.">}),
    N_MEMBER_DEF(ui, neam::metadata::info{.description = c_string_t<"Launch in graphical mode.\nWill only open the window after imgui shaders are successfuly packed.">}),
    N_MEMBER_DEF(print_source_name, neam::metadata::info{.description = c_string_t<"Will print file names that are being imported.">}),
    N_MEMBER_DEF(watch_delay, neam::metadata::info{.description = c_string_t<"Sleep duration when no changes are detected.">}),
    N_MEMBER_DEF(thread_count, neam::metadata::info{.description = c_string_t<"Number of thread the task manager will launch.">})
  >;
};

