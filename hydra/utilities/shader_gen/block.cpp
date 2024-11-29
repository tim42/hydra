//
// created by : Timothée Feuillet
// date: 2024-2-11
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

#include "block.hpp"
#include <ntools/mt_check/vector.hpp>
#include <ntools/mt_check/map.hpp>

namespace neam::hydra::shaders::internal
{
  struct block_struct_map_entry_t
  {
    std::string glsl_name;
    id_t hash;
    std::vector<id_t> dependencies;
    generate_function_t generate_member_code;
  };
  struct block_struct_map_t
  {
    std::mtc_map<id_t, block_struct_map_entry_t> map;
  };
  static block_struct_map_t& get_block_struct_map()
  {
    static block_struct_map_t ret;
    return ret;
  }

  void register_block_struct(string_id cpp_name, std::string glsl_name, id_t hash, generate_function_t generate_member_code, std::vector<id_t>&& deps)
  {
    get_block_struct_map().map.emplace(cpp_name, block_struct_map_entry_t
    {
      .glsl_name = glsl_name,
      .hash = hash,
      .dependencies = std::move(deps),
      .generate_member_code = generate_member_code
    });
  }

  void unregister_block_struct(string_id cpp_name)
  {
    get_block_struct_map().map.erase(cpp_name);
  }

  bool is_struct_registered(id_t cpp_name)
  {
    const auto& map = get_block_struct_map().map;
    return map.contains(cpp_name);
  }

  std::string generate_all_structs()
  {
    std::string ret;
    for (const auto& it : get_block_struct_map().map)
    {
      ret = fmt::format("{}// cpp struct: {}\nstruct {} {{ {} }};\n\n", ret, string_id::_from_id_t(it.first), it.second.glsl_name, it.second.generate_member_code());
    }
    return ret;
  }

  std::string generate_struct_body(id_t cpp_name)
  {
    auto& map = get_block_struct_map().map;
    if (auto it = map.find(cpp_name); it != map.end())
    {
      return it->second.generate_member_code();
    }
    return {};
  }

  std::string generate_structs(const std::vector<id_t>& ids)
  {
    std::string ret;
    auto& map = get_block_struct_map().map;
    for (id_t id : ids)
    {
      if (auto it = map.find(id); it != map.end())
      {
        ret = fmt::format("{}\n// cpp struct: {}\nstruct {} {{ {} }};\n", ret,
                          string_id::_from_id_t(id), it->second.glsl_name, it->second.generate_member_code());
      }
    }
    ret = fmt::format("\n// begin/generated structs:\n{}\n// end/generated structs\n\n", ret);
    return ret;
  }

  void get_all_dependencies(id_t cpp_name, std::vector<id_t>& deps, bool insert_self)
  {
    const auto has_entry = [&](id_t id) { return std::find(deps.begin(), deps.end(), id) != deps.end(); };
    // no need to do anything if we already have the current entry
    if (has_entry(cpp_name))
      return;
    auto& map = get_block_struct_map().map;
    if (auto it = map.find(cpp_name); it != map.end())
    {
      for (id_t id : it->second.dependencies)
      {
        get_all_dependencies(id, deps, true);
      }
      if (insert_self)
        deps.push_back(cpp_name);
    }
    // will not do anything if the entry is not present (might generate a shader compilation error)
  }
}
