//
// file : pipeline_cache.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/pipeline_cache.hpp
//
// created by : Timothée Feuillet
// date: Wed Aug 24 2016 18:40:12 GMT+0200 (CEST)
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

#ifndef __N_16587258122591023329_1331522798_PIPELINE_CACHE_HPP__
#define __N_16587258122591023329_1331522798_PIPELINE_CACHE_HPP__

#include <initializer_list>
#include <vector>
#include <new>

#include <vulkan/vulkan.h>

#include "../hydra_exception.hpp"
#include "device.hpp"
#include "pipeline_cache_data.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Class that handles pipeline caches
      class pipeline_cache
      {
        public: // advanced
          pipeline_cache(device &_dev, VkPipelineCache _vk_pcache) : dev(_dev), vk_pcache(_vk_pcache) {}
        public:
          /// \brief Create an empty, uninitialized cache
          pipeline_cache(device &_dev)
            : dev(_dev)
          {
            VkPipelineCacheCreateInfo create_info
            {
              VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
              nullptr,
              0,
              0, nullptr
            };

            check::on_vulkan_error::n_throw_exception(dev._vkCreatePipelineCache(&create_info, nullptr, &vk_pcache));
          }

          /// \brief Create a cache with some data
          pipeline_cache(device &_dev, const void *data, size_t data_size)
            : dev(_dev)
          {
            VkPipelineCacheCreateInfo create_info
            {
              VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
              nullptr,
              0,
              data_size, data
            };

            check::on_vulkan_error::n_throw_exception(dev._vkCreatePipelineCache(&create_info, nullptr, &vk_pcache));
          }

          /// \brief Move constructor
          pipeline_cache(pipeline_cache &&o) : dev(o.dev), vk_pcache(o.vk_pcache) { o.vk_pcache = nullptr; }

          ~pipeline_cache()
          {
            if (!vk_pcache)
              dev._vkDestroyPipelineCache(vk_pcache, nullptr);
          }

          /// \brief Merge two caches
          void merge_with(const pipeline_cache &o) { merge_with({&o,}); }

          /// \brief Merge multiple caches
          void merge_with(std::initializer_list<const pipeline_cache *> caches)
          {
            std::vector<VkPipelineCache> tmp_caches;
            tmp_caches.reserve(caches.size());
            for (const pipeline_cache *it : caches)
              tmp_caches.push_back(it->get_vk_pipeline_cache());
            check::on_vulkan_error::n_throw_exception(dev._vkMergePipelineCaches(vk_pcache, tmp_caches.size(), tmp_caches.data()));
          }

          /// \brief Return the pipeline cache data
          pipeline_cache_data get_cache_data() const
          {
            size_t data_size = 0;
            check::on_vulkan_error::n_throw_exception(dev._vkGetPipelineCacheData(vk_pcache, &data_size, nullptr));
            void *data = operator new(data_size, std::nothrow);
            check::on_vulkan_error::n_throw_exception(dev._vkGetPipelineCacheData(vk_pcache, &data_size, data));

            return pipeline_cache_data(data, data_size);
          }

        public: // advanced
          /// \brief Return the underlying vulkan object
          VkPipelineCache get_vk_pipeline_cache() const { return vk_pcache; }
        private:
          device &dev;
          VkPipelineCache vk_pcache;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_16587258122591023329_1331522798_PIPELINE_CACHE_HPP__

