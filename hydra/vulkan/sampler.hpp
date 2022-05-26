//
// file : sampler.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/sampler.hpp
//
// created by : Timothée Feuillet
// date: Fri Sep 02 2016 21:30:32 GMT+0200 (CEST)
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

#ifndef __N_134929389320483379_2377717497_SAMPLER_HPP__
#define __N_134929389320483379_2377717497_SAMPLER_HPP__

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "../hydra_debug.hpp"
#include "device.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      using sampler_address_mode = glm::tvec3<VkSamplerAddressMode, glm::lowp>;
      class sampler
      {
        public: // advanced
          sampler(device &_dev, VkSampler _vk_sampler) : dev(_dev), vk_sampler(_vk_sampler) {}
          sampler(device &_dev, VkSamplerCreateInfo create_info)
            : dev(_dev)
          {
            create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            create_info.pNext = nullptr;
            create_info.flags = 0;
            check::on_vulkan_error::n_assert_success(dev._vkCreateSampler(&create_info, nullptr, &vk_sampler));
          }

        public:
          /// \brief Most commonly used values
          sampler(device &_dev, VkFilter mag, VkFilter min, VkSamplerMipmapMode mipmap, float miplodbias, float minlod, float maxlod,
                  sampler_address_mode sam = sampler_address_mode(VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT))
            : sampler(_dev, VkSamplerCreateInfo
          {
            VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, nullptr, 0,
            mag, min, mipmap,
            sam.x, sam.y, sam.z,
            miplodbias,
            VK_FALSE, 1.0,
            VK_FALSE, VK_COMPARE_OP_ALWAYS,
            minlod, maxlod,
            VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
            VK_FALSE
          })
          {}
          sampler(device &_dev, VkFilter mag, VkFilter min, VkSamplerMipmapMode mipmap, float miplodbias, float minlod, float maxlod, float max_anisotropy,
                  sampler_address_mode sam = sampler_address_mode(VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT))
            : sampler(_dev, VkSamplerCreateInfo
          {
            VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, nullptr, 0,
            mag, min, mipmap,
            sam.x, sam.y, sam.z,
            miplodbias,
            VK_TRUE, max_anisotropy,
            VK_FALSE, VK_COMPARE_OP_ALWAYS,
            minlod, maxlod,
            VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
            VK_FALSE
          })
          {}

          sampler(sampler &&o) : dev(o.dev), vk_sampler(o.vk_sampler) { o.vk_sampler = nullptr; }
          sampler &operator = (sampler &&o)
          {
            if (&o == this)
              return *this;

            check::on_vulkan_error::n_assert(&o.dev == &dev, "can't assign images with different vulkan devices");

            if (vk_sampler)
              dev._vkDestroySampler(vk_sampler, nullptr);

            vk_sampler = o.vk_sampler;
            o.vk_sampler = nullptr;
            return *this;
          }

          ~sampler()
          {
            if (vk_sampler)
              dev._vkDestroySampler(vk_sampler, nullptr);
          }

        public: // advanced
          /// \brief Return the VkSampler
          VkSampler _get_vk_sampler() const { return vk_sampler; }
          const VkSampler* _get_vk_sampler_ptr() const { return &vk_sampler; }
        private:
          device &dev;
          VkSampler vk_sampler;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_134929389320483379_2377717497_SAMPLER_HPP__

