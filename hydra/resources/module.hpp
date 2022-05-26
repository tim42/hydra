//
// created by : Timothée Feuillet
// date: 2021-11-26
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

#include <resources/asset.hpp>

namespace neam::resources
{
  /// \brief represent a chunk of code (a .so object) with a resource_id.
  ///        Modules can be packed with the other resources.
  ///        Resource accessor is : [module:/path/to/module](target)
  ///        (`target` being the target (like `linux` for a linux target))
  class exe_module : public asset<"module", exe_module>
  {
    public: // api:
      void load();
      void unload();

    public: // resource stuff:
      static exe_module from_raw_data(const raw_data& data, status& st);

      /// \brief Saves the asset to packed data
      static raw_data to_raw_data(const exe_module& data, status& st);

      /// \brief Pack a resource to the raw data
      static exe_module from_resource_data(raw_data&& data, status& st)
      {
        // we don't have a different resource from raw / packed
        return from_raw_data(data, st);
      }

    private:
  };
}
