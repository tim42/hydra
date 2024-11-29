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

#include <hydra/engine/hydra_context.hpp>

namespace neam::hydra::shaders::internal
{
  struct runtime_ds_map_entry_t
  {
    generate_ds_layout_function_t generate_layout;
  };
  struct runtime_ds_map_t
  {
    std::mtc_map<id_t, runtime_ds_map_entry_t> map;
  };
  static runtime_ds_map_t& get_runtime_descriptor_set_map()
  {
    static runtime_ds_map_t ret;
    return ret;
  }

  void register_runtime_descriptor_set(string_id cpp_name, generate_ds_layout_function_t generate)
  {
    get_runtime_descriptor_set_map().map.emplace(cpp_name, runtime_ds_map_entry_t
    {
      .generate_layout = generate,
    });
  }

  void unregister_runtime_descriptor_set(string_id cpp_name)
  {
    get_runtime_descriptor_set_map().map.erase(cpp_name);
  }

  bool is_runtime_descriptor_set_registered(id_t cpp_name)
  {
    const auto& map = get_runtime_descriptor_set_map().map;
    return map.contains(cpp_name);
  }

  vk::descriptor_set_layout generate_descriptor_set_layout(id_t cpp_name, vk::device& dev, VkDescriptorSetLayoutCreateFlags flags)
  {
    auto& map = get_runtime_descriptor_set_map().map;
    if (auto it = map.find(cpp_name); it != map.end())
    {
      return it->second.generate_layout(dev, flags);
    }
    return {dev, nullptr};
  }


  vk::descriptor_set descriptor_set_struct_internal::allocate_descriptor_set(hydra_context& hctx, vk::descriptor_set_layout& ds_layout, uint32_t variable_descriptor_count)
  {
    return hctx.da.allocate_set(ds_layout, variable_descriptor_count);
  }

  void descriptor_set_struct_internal::deallocate_descriptor_set(hydra_context& hctx, vk::descriptor_set&& set)
  {
    hctx.dfe.defer_destruction(std::move(set));
  }

  vk::queue& descriptor_set_struct_internal::get_graphic_queue(hydra_context& hctx)
  {
    return hctx.gqueue;
  }

  vk::device& descriptor_set_struct_internal::get_device(hydra_context& hctx)
  {
    return hctx.device;
  }
}

