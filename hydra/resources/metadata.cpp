//
// created by : Timothée Feuillet
// date: 2023-3-11
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

#include "metadata.hpp"

namespace neam::resources
{
  std::unordered_map<id_t, metadata_type_registration_t>& get_metadata_type_map()
  {
    static std::unordered_map<id_t, metadata_type_registration_t> map;
    return map;
  }


  void register_metadata_type(metadata_type_registration_t&& type)
  {
    auto& map = get_metadata_type_map();
    map.insert_or_assign(type.entry_name_id, std::move(type));
  }

  void unregister_metadata_type(id_t entry_name_id)
  {
    get_metadata_type_map().erase(entry_name_id);
  }
}

