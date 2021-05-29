//
// file : shader_manager.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/shader_manager.hpp
//
// created by : Timothée Feuillet
// date: Sun Sep 04 2016 22:07:38 GMT+0200 (CEST)
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

#ifndef __N_67232540175720276_19863771_SHADER_MANAGER_HPP__
#define __N_67232540175720276_19863771_SHADER_MANAGER_HPP__

#include <string>
#include <map>

#include "../vulkan/shader_module.hpp"
#include "../vulkan/shader_loaders/spirv_loader.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief A lovely class that handle shaders to avoid the same source being loaded
    /// more than one time
    ///
    /// As shaders are only used at pipeline creation, having a slow [O(log(n))] way
    /// to retrieve shader modules does not really matter
    ///
    /// \note Only the spirv loaded is currently handled
    /// \note It will be a little quirky until std::filesystem (C++17) is here
    class shader_manager
    {
      public:
        shader_manager(vk::device &_dev) : dev(_dev) {}
        shader_manager(shader_manager &&o) : dev(o.dev), module_map(std::move(o.module_map)) {}

        /// \brief Load a spirv shader
        vk::shader_module &load_shader(const std::string &filename)
        {
          // retrieve the module from memory
          auto it = module_map.find(filename);
          if (it != module_map.end())
            return it->second;

          // load the module from disk
          auto ret = module_map.emplace(filename, vk::spirv_shader::load_from_file(dev, filename));
#ifndef HYDRA_NO_MESSAGES
          neam::cr::out.info() << "loaded SPIRV shader '" << filename << "'" << std::endl;
#endif

          return ret.first->second;
        }

        /// \brief Reload all shaders from the disk
        void refresh()
        {
          for (auto &it : module_map)
            it.second = vk::spirv_shader::load_from_file(dev, it.first);
        }

        /// \brief Remove all modules
        void clear()
        {
          module_map.clear();
        }

        size_t get_shader_count() const
        {
          return module_map.size();
        }

      private:
        vk::device &dev;
        std::map<std::string, vk::shader_module> module_map;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_67232540175720276_19863771_SHADER_MANAGER_HPP__

