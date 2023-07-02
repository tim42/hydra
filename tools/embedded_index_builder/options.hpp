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
    bool help = false;
    uint32_t thread_count = std::thread::hardware_concurrency() - 4;

    std::string namespace_name = "neam::autogen";
    std::string key;
    std::filesystem::path source_folder = std::filesystem::current_path();
    std::filesystem::path output = std::filesystem::current_path() / "embedded_index.hpp";

    // extra: (from parameters)
    id_t index_key = id_t::none;

    // resources:
    std::vector<std::string_view> parameters;
  };
}
N_METADATA_STRUCT(neam::global_options)
{
  using member_list = neam::ct::type_list
  <
    N_MEMBER_DEF(help, neam::metadata::info{.description = c_string_t<"Print this message and exit.">}),
    N_MEMBER_DEF(verbose, neam::metadata::info{.description = c_string_t<"Show debug messages. May be extremly verbose.">}),
    N_MEMBER_DEF(silent, neam::metadata::info{.description = c_string_t<"Only show warning (and above) messages.">}),
    N_MEMBER_DEF(thread_count, neam::metadata::info{.description = c_string_t<"Number of thread the task manager will launch.">}),

    N_MEMBER_DEF(namespace_name, neam::metadata::info{.description = c_string_t<"Namespace to put the data in.">}),
    N_MEMBER_DEF(key, neam::metadata::info{.description = c_string_t<"Index key. Will be saved along the index in the header.">}),
    N_MEMBER_DEF(source_folder, neam::metadata::info{.description = c_string_t<"Path to the source folder.">}),
    N_MEMBER_DEF(output, neam::metadata::info{.description = c_string_t<"Path to the output file.">})
  >;
};

