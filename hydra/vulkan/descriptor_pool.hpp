//
// file : descriptor_pool.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/descriptor_pool.hpp
//
// created by : Timothée Feuillet
// date: Tue Aug 30 2016 12:37:52 GMT+0200 (CEST)
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

#include "../hydra_debug.hpp"
#include "device.hpp"

#include "descriptor_set_layout.hpp"
#include "descriptor_set.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkDescriptorPool
      class descriptor_pool
      {
        public: // advanced
          descriptor_pool(device &_dev, const VkDescriptorPoolCreateInfo &create_info)
            : dev(_dev)
          {
            check::on_vulkan_error::n_assert_success(dev._vkCreateDescriptorPool(&create_info, nullptr, &vk_dpool));
          }

          descriptor_pool(device &_dev, VkDescriptorPool _vk_dpool) : dev(_dev), vk_dpool(_vk_dpool) {}

        public:
          /// \brief Construct the descriptor pool
          /// \link https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkDescriptorPoolCreateInfo.html
          descriptor_pool(device &_dev, uint32_t max_set, const std::vector<VkDescriptorPoolSize> &pool_size_vct, VkDescriptorPoolCreateFlags flags = 0)
            : descriptor_pool(_dev, VkDescriptorPoolCreateInfo
            {
              VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr, flags,
              max_set, (uint32_t)pool_size_vct.size(), pool_size_vct.data()
            })
          {}

          descriptor_pool(descriptor_pool &&o) : dev(o.dev), vk_dpool(o.vk_dpool) { o.vk_dpool = nullptr; }
          descriptor_pool& operator = (descriptor_pool &&o)
          {
            if (&o == this)
              return *this;

            check::on_vulkan_error::n_assert(&dev == &o.dev, "Cannot move-assign a descriptor pool to a pool from another device");
            if (vk_dpool)
              dev._vkDestroyDescriptorPool(vk_dpool, nullptr);
            vk_dpool = o.vk_dpool;
            o.vk_dpool = nullptr;
            return *this;
          }

          ~descriptor_pool()
          {
            if (vk_dpool)
              dev._vkDestroyDescriptorPool(vk_dpool, nullptr);
          }

          /// \brief Reset the descriptor pool
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkResetDescriptorPool.html">vulkan khr doc</a>
          void reset()
          {
            std::lock_guard _l(pool_lock);
            check::on_vulkan_error::n_assert_success(dev._vkResetDescriptorPool(vk_dpool, 0));
          }

          /// \brief Allocate some new descriptors from to the pool
          std::vector<descriptor_set> allocate_descriptor_sets(const std::vector<descriptor_set_layout *> &dsl_vct, bool allow_free = false)
          {
            std::vector<VkDescriptorSetLayout> vk_dsl_vct;
            vk_dsl_vct.reserve(dsl_vct.size());
            for (descriptor_set_layout *it : dsl_vct)
              vk_dsl_vct.push_back(it->_get_vk_descriptor_set_layout());
            VkDescriptorSetAllocateInfo ds_allocate
            {
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, vk_dpool,
              (uint32_t)vk_dsl_vct.size(), vk_dsl_vct.data()
            };
            std::vector<VkDescriptorSet> ds_sets;
            ds_sets.resize(dsl_vct.size());
            {
              std::lock_guard _l(pool_lock);
              check::on_vulkan_error::n_assert_success(dev._vkAllocateDescriptorSets(&ds_allocate, ds_sets.data()));
            }

            std::vector<descriptor_set> ret;
            ret.reserve(ds_sets.size());

            for (VkDescriptorSet it : ds_sets)
            {
              if (allow_free)
                ret.push_back(descriptor_set(dev, this, it));
              else
                ret.push_back(descriptor_set(dev, it));
            }
            return ret;
          }

          /// \brief Allocate a new descriptor from to the pool
          descriptor_set allocate_descriptor_set(const descriptor_set_layout &dsl, bool allow_free = false)
          {
            VkDescriptorSetLayout vk_dsl = dsl._get_vk_descriptor_set_layout();
            VkDescriptorSetAllocateInfo ds_allocate
            {
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, vk_dpool,
              1, &vk_dsl
            };
            VkDescriptorSet rset;
            {
              std::lock_guard _l(pool_lock);
              check::on_vulkan_error::n_assert_success(dev._vkAllocateDescriptorSets(&ds_allocate, &rset));
            }

            if (allow_free)
              return descriptor_set(dev, this, rset);
            else
              return descriptor_set(dev, rset);
          }

          /// \brief Return a descriptor set to the pool
          void free_descriptor_set(descriptor_set &dset)
          {
            VkDescriptorSet vk_dset = dset._get_vk_descritpor_set();

            std::lock_guard _l(pool_lock);
            check::on_vulkan_error::n_assert_success(dev._vkFreeDescriptorSets(vk_dpool, 1, &vk_dset));
          }

          /// \brief Return multiple descriptor set to the pool
          void free_descriptor_set(const std::vector<descriptor_set *> &dset)
          {
            std::vector<VkDescriptorSet> vk_dset;
            vk_dset.reserve(dset.size());
            for (descriptor_set *it : dset)
              vk_dset.push_back(it->_get_vk_descritpor_set());

            std::lock_guard _l(pool_lock);
            check::on_vulkan_error::n_assert_success(dev._vkFreeDescriptorSets(vk_dpool, vk_dset.size(), vk_dset.data()));
          }

        public: // advanced
          /// \brief Return the vulkan descriptor pool
          VkDescriptorPool _get_vk_descriptor_pool() const { return vk_dpool; }
        private:
          device &dev;
          VkDescriptorPool vk_dpool;
          mutable spinlock pool_lock;
      };


      // //

      inline descriptor_set::~descriptor_set()
      {
        if (dpool)
          dpool->free_descriptor_set(*this);
      }

    } // namespace vk
  } // namespace hydra
} // namespace neam




