//
// file : instance_extension.hpp
// in : file:///home/tim/projects/hydra/hydra/init/instance_extension.hpp
//
// created by : Timothée Feuillet
// date: Mon Apr 25 2016 22:39:18 GMT+0200 (CEST)
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

#ifndef __N_1979217171365015759_13397729_INSTANCE_EXTENSION_HPP__
#define __N_1979217171365015759_13397729_INSTANCE_EXTENSION_HPP__

#include <string>
#include <vulkan/vulkan.h>

namespace neam
{
  namespace hydra
  {
    class vk_instance_extension
    {
      private:
        vk_instance_extension(const VkExtensionProperties &_properties) : properties(_properties) {}

      public:
        /// \brief Return the name of the extension
        std::string get_name() const
        {
          return properties.extensionName;
        }

        /// \brief Return the version (revision) of the extension
        uint32_t get_revision() const
        {
          return properties.specVersion;
        }

      private:
        VkExtensionProperties properties;
        friend class hydra_instance_creator;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_1979217171365015759_13397729_INSTANCE_EXTENSION_HPP__

