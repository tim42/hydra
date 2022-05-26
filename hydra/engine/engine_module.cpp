//
// created by : Timothée Feuillet
// date: 2022-5-22
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


#include "engine_module.hpp"
#include <hydra/hydra_debug.hpp>

namespace neam::hydra::module_manager
{
  namespace
  {
    struct module_t
    {
      create_func_t create;
      filter_func_t filter;
      std::string name;
    };

    struct module_data
    {
      std::vector<module_t> module_list;
    };

    static module_data& get_module_data()
    {
      static module_data mod;
      return mod;
    }
  }

  void register_module(create_func_t create, filter_func_t filter, const std::string& name)
  {
    module_data& data = get_module_data();
    data.module_list.push_back({create, filter, name});
  }

  void unregister_module(const std::string& name)
  {
    module_data& data = get_module_data();

    data.module_list.erase(std::find_if(data.module_list.begin(), data.module_list.end(), [&name](auto& it) { return it.name == name; }));
  }

  std::vector<filtered_module_t> filter_modules(runtime_mode mode)
  {
    module_data& data = get_module_data();
    std::vector<filtered_module_t> ret;

    for (auto& it : data.module_list)
    {
      if (it.filter(mode))
      {
        ret.push_back({it.create, it.name});
      }
    }
    return ret;
  }
}

