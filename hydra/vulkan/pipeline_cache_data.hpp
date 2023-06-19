//
// file : pipeline_cache_data.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/pipeline_cache_data.hpp
//
// created by : Timothée Feuillet
// date: Wed Aug 24 2016 19:50:05 GMT+0200 (CEST)
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


#include <cstddef>
#include <new>

#include <vulkan/vulkan.h>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief The header (v1) of the pipeline cache data
      /// assumes we are in little endian
      struct pipeline_cache_header_v1
      {
        uint32_t length;
        uint32_t cache_header_version;
        uint32_t vendorID;
        uint32_t deviceID;
        uint8_t cacheID[VK_UUID_SIZE];
      };

      /// \brief The pipeline cache data
      /// It manages its own memory, so if you want it, you may want to copy it
      class pipeline_cache_data
      {
        public:
          /// \brief Create a cache data.
          /// \note The memory belongs to the instance and may not be freed
          pipeline_cache_data(const void *_cache_data, size_t _data_size) : cache_data(_cache_data), data_size(_data_size) {}

          /// \brief Move constructor
          pipeline_cache_data(pipeline_cache_data &&o) : cache_data(o.cache_data), data_size(o.data_size) { o.cache_data = nullptr; o.data_size = 0; }

          ~pipeline_cache_data()
          {
            if (cache_data)
              operator delete((void *)cache_data, std::nothrow);
          }

          /// \brief Return the size of the data
          size_t size() const { return data_size; }
          /// \brief Return the data
          const void *data() const { return cache_data; }

          /// \brief Return the header of the cache (header v1)
          const pipeline_cache_header_v1 &get_header_v1()
          {
            return *reinterpret_cast<const pipeline_cache_header_v1 *>(cache_data);
          }

        private:
          const void *cache_data;
          size_t data_size;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



