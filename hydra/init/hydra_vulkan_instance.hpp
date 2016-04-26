//
// file : hydra_vulkan_instance.hpp
// in : file:///home/tim/projects/hydra/hydra/init/hydra_vulkan_instance.hpp
//
// created by : Timothée Feuillet
// date: Tue Apr 26 2016 11:53:06 GMT+0200 (CEST)
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

#ifndef __N_692459433198810776_77086004_HYDRA_VULKAN_INSTANCE_HPP__
#define __N_692459433198810776_77086004_HYDRA_VULKAN_INSTANCE_HPP__

#include <string>
#include <vulkan/vulkan.h>

namespace neam
{
  namespace hydra
  {
    /// \brief Wrap a vulkan instance
    class hydra_vulkan_instance
    {
      public:
        /// \brief The only reason this constructor is public is to allow
        /// compatibility with other means of creating a vulkan instance.
        /// Instead of calling this directly, you should use the class
        /// hydra_instance_creator that handle the creation for you.
        hydra_vulkan_instance(VkInstance _vulkan_instance, const std::string &_app_name = "")
          : vulkan_instance(_vulkan_instance), app_name(_app_name)
        {
        }

        /// \brief Move constructor.
        /// Allow move semantics
        hydra_vulkan_instance(hydra_vulkan_instance &&o)
         : vulkan_instance(o.vulkan_instance), app_name(o.app_name)
        {
          o.vulkan_instance = nullptr;
        }

        /// \brief Destructor
        ~hydra_vulkan_instance()
        {
          if (vulkan_instance != nullptr)
            vkDestroyInstance(vulkan_instance, nullptr);
        }

        /// \brief Return the vulkan instance object
        /// \note Marked as advanced
        VkInstance _get_vulkan_instance()
        {
          return vulkan_instance;
        }

      private:
        VkInstance vulkan_instance;
        std::string app_name;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_692459433198810776_77086004_HYDRA_VULKAN_INSTANCE_HPP__
