//
// file : spirv_loader.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/shader_loaders/spirv_loader.hpp
//
// created by : Timothée Feuillet
// date: Wed Aug 10 2016 14:01:10 GMT+0200 (CEST)
//
//
// Copyright (c) 2016 Timothée Feuillet
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

#ifndef __N_280839898788326663_2575717169_SPIRV_LOADER_HPP__
#define __N_280839898788326663_2575717169_SPIRV_LOADER_HPP__

#include <string>
#include <fstream>

#include "../shader_module.hpp"
#include "../../hydra_debug.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief create a shader module from a spirv assembly
      class spirv_shader
      {
        public:
          /// \brief Create a shader module from a SIPRV file
          static shader_module load_from_file(device &dev, const std::string &filename)
          {
#ifndef HYDRA_NO_MESSAGES
            neam::cr::out().log("loading SPIRV shader '{}'...", filename);
#endif
            std::ifstream file(filename, std::ios::binary | std::ios::ate);

            if (!check::on_vulkan_error::n_check(file.is_open() && file, "can't load spirv file '{}'", filename))
            {
              return shader_module(dev, nullptr, "main");
            }


            size_t sz = file.tellg();
            file.seekg(0);

            std::vector<uint32_t> buffer(sz / sizeof(uint32_t) + 1);
            file.read((char *)buffer.data(), sz);

            return shader_module(dev, buffer.data(), sz, "main");
          }

        private:
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_280839898788326663_2575717169_SPIRV_LOADER_HPP__

