//
// created by : Timothée Feuillet
// date: 2024-2-27
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

#include "descriptor_sets.hpp"
#include <ntools/mt_check/map.hpp>

namespace neam::hydra::shaders::internal
{
  struct ds_map_entry_t
  {
    std::vector<id_t> dependencies;
    generate_ds_function_t generate_code;
  };
  struct ds_map_t
  {
    std::mtc_map<id_t, ds_map_entry_t> map;
  };
  static ds_map_t& get_descriptor_set_map()
  {
    static ds_map_t* ret = nullptr;
    if (ret == nullptr)
      ret = new ds_map_t{};
    return *ret;
  }

  void register_descriptor_set(string_id cpp_name, generate_ds_function_t generate, std::vector<id_t>&& deps)
  {
    get_descriptor_set_map().map.emplace(cpp_name, ds_map_entry_t
    {
      .dependencies = std::move(deps),
      .generate_code = generate,
    });
  }

  void unregister_descriptor_set(string_id cpp_name)
  {
    get_descriptor_set_map().map.erase(cpp_name);
  }
  bool is_descriptor_set_registered(id_t cpp_name)
  {
    const auto& map = get_descriptor_set_map().map;
    return map.contains(cpp_name);
  }

  std::string generate_descriptor_set(id_t cpp_name, uint32_t set_index)
  {
    auto& map = get_descriptor_set_map().map;
    if (auto it = map.find(cpp_name); it != map.end())
    {
      return it->second.generate_code(set_index);
    }
    return {};
  }

  void get_descriptor_set_dependencies(id_t cpp_name, std::vector<id_t>& deps)
  {
    auto& map = get_descriptor_set_map().map;
    if (auto it = map.find(cpp_name); it != map.end())
    {
      for (id_t id : it->second.dependencies)
      {
        get_all_dependencies(id, deps, true);
      }
    }
    // will not do anything if the entry is not present (might generate a shader compilation error)
  }
}

