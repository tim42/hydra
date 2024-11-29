//
// file : descriptor_set.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/descriptor_set.hpp
//
// created by : Timothée Feuillet
// date: Tue Aug 30 2016 15:22:16 GMT+0200 (CEST)
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

#pragma once


#include <vulkan/vulkan.h>
#include <ntools/mt_check/mt_check_base.hpp>

#include "buffer.hpp"
#include "sampler.hpp"
#include "image_view.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      class descriptor_pool;

      struct buffer_info
      {
        const buffer &buff;
        size_t offset;
        size_t range_size;
      };

      struct image_info
      {
        const image_view &imgv;
        VkImageLayout layout;
      };

      struct image_sampler_info
      {
        const sampler &splr;
        const image_view &imgv;
        VkImageLayout layout;
      };

      /// \brief Wraps a VkDescriptorSet
      class descriptor_set : public cr::mt_checked<descriptor_set>
      {
        public: // advanced
          descriptor_set(device &_dev, descriptor_pool *_dpool, VkDescriptorSet _vk_ds) : dev(_dev), dpool(_dpool), vk_ds(_vk_ds) {}
          descriptor_set(device &_dev, VkDescriptorSet _vk_ds) : dev(_dev), vk_ds(_vk_ds) {}

        public:
          descriptor_set(descriptor_set &&o) : dev(o.dev), dpool(o.dpool), vk_ds(o.vk_ds) { o.dpool = nullptr; o.vk_ds = nullptr; }
          descriptor_set &operator = (descriptor_set &&o)
          {
            N_MTC_WRITER_SCOPE_OBJ(o);
            N_MTC_WRITER_SCOPE;
            if (this == &o)
              return *this;
            check::on_vulkan_error::n_assert(&dev == &o.dev, "can't assign descriptor set that have two different vulkan devices");
            vk_ds = o.vk_ds;
            o.vk_ds = nullptr;
            dpool = o.dpool;
            o.dpool = nullptr;
            return *this;
          }

          // implemented in descriptor_pool.hpp
          ~descriptor_set();

          /// \brief  Update the contents of a descriptor set object.
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkUpdateDescriptorSets.html">vulkan khr doc</a>
          void write_descriptor_set(uint32_t dst_binding, uint32_t dst_array, VkDescriptorType dtype, const std::vector<buffer_info> &buff_info)
          {
            if (!vk_ds) return;

            std::vector<VkDescriptorBufferInfo> dbi;
            dbi.reserve(buff_info.size());
            for (const buffer_info &it : buff_info)
            {
              dbi.push_back(VkDescriptorBufferInfo
              {
                it.buff._get_vk_buffer(),
                it.offset,
                it.range_size
              });
            }
            N_MTC_WRITER_SCOPE;
            VkWriteDescriptorSet vk_wds
            {
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, vk_ds,
              dst_binding, dst_array, (uint32_t)dbi.size(), dtype,
              nullptr, dbi.data(), nullptr
            };
            dev._vkUpdateDescriptorSets(1, &vk_wds, 0, nullptr);
          }

          /// \brief  Update the contents of a descriptor set object.
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkUpdateDescriptorSets.html">vulkan khr doc</a>
          void write_descriptor_set(uint32_t dst_binding, uint32_t dst_array, const std::vector<image_sampler_info> &img_info)
          {
            if (!vk_ds) return;

            std::vector<VkDescriptorImageInfo> dii;
            dii.reserve(img_info.size());
            for (const auto &it : img_info)
            {
              dii.push_back(VkDescriptorImageInfo
              {
                it.splr._get_vk_sampler(),
                it.imgv.get_vk_image_view(),
                it.layout
              });
            }
            N_MTC_WRITER_SCOPE;
            VkWriteDescriptorSet vk_wds
            {
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, vk_ds,
              dst_binding, dst_array, (uint32_t)dii.size(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              dii.data(), nullptr, nullptr
            };
            dev._vkUpdateDescriptorSets(1, &vk_wds, 0, nullptr);
          }
          
          /// \brief  Update the contents of a descriptor set object.
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkUpdateDescriptorSets.html">vulkan khr doc</a>
          void write_descriptor_set(uint32_t dst_binding, uint32_t dst_array, VkDescriptorType dtype, const std::vector<image_info> &img_info)
          {
            if (!vk_ds) return;

            std::vector<VkDescriptorImageInfo> dii;
            dii.reserve(img_info.size());
            for (const auto &it : img_info)
            {
              dii.push_back(VkDescriptorImageInfo
              {
                nullptr,
                it.imgv.get_vk_image_view(),
                it.layout
              });
            }
            N_MTC_WRITER_SCOPE;
            VkWriteDescriptorSet vk_wds
            {
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, vk_ds,
              dst_binding, dst_array, (uint32_t)dii.size(), dtype,
              dii.data(), nullptr, nullptr
            };
            dev._vkUpdateDescriptorSets(1, &vk_wds, 0, nullptr);
          }

          // TODO: the other things (update, copy*, write*, ...)

        public: // advanced
          VkDescriptorSet _get_vk_descritpor_set() const { return vk_ds; }

          void _set_debug_name(const std::string& name)
          {
            N_MTC_WRITER_SCOPE;
            dev._set_object_debug_name((uint64_t)vk_ds, VK_OBJECT_TYPE_DESCRIPTOR_SET, name);
          }

        private:
          device &dev;
          descriptor_pool *dpool = nullptr;
          VkDescriptorSet vk_ds;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam




