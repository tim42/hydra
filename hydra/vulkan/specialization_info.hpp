//
// file : specialization_info.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/specialization_info.hpp
//
// created by : Timothée Feuillet
// date: Thu Aug 11 2016 14:11:04 GMT+0200 (CEST)
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


#include <vector>
#include <vulkan/vulkan.h>

#include <ntools/id/string_id.hpp>
#include <ntools/raw_data.hpp>
#include <ntools/memory_allocator.hpp> // used to get a fast-growing yet contiguous memory area
#include "../hydra_debug.hpp" // for check::

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief A map of parameters that can be used to provide specialization info for pipelines
      ///        Is to be used with the shader module constant id map to generate what can be consumed by vulkan
      class specialization
      {
        public:
          class parameter
          {
            public:
              parameter(parameter&& o)
              {
                size = o.size;
                if (size <= k_embedded_size)
                  value = o.value;
                else
                  new (&ext_value) raw_data { std::move(o.ext_value) };
              }

              template<typename T>
              parameter(T&& o)
              {
                using type = std::remove_cvref_t<T>;
                static_assert(!std::is_same_v<type, parameter>);

                size = sizeof(type);
                if constexpr (sizeof(type) <= k_embedded_size)
                {
                  memcpy(&value, &o, sizeof(type));
                }
                else
                {
                  new (&ext_value) raw_data { raw_data::allocate(sizeof(type)) };
                  memcpy(ext_value, &o, sizeof(type));
                }
              }

              parameter& operator = (parameter&& o)
              {
                if (this == &o) return *this;
                if (size > k_embedded_size)
                  ext_value.~raw_data();
                size = o.size;
                if (size <= k_embedded_size)
                  value = o.value;
                else
                  new (&ext_value) raw_data { std::move(o.ext_value) };
                return *this;
              }

              template<typename T>
              parameter& operator = (T&& o)
              {
                using type = std::remove_cvref_t<T>;
                static_assert(!std::is_same_v<type, parameter>);

                if (size > k_embedded_size)
                  ext_value.~raw_data();
                size = sizeof(type);
                if constexpr (sizeof(type) <= k_embedded_size)
                {
                  memcpy(value, &o, sizeof(type));
                }
                else
                {
                  new (&ext_value) raw_data { raw_data::allocate(sizeof(type)) };
                  memcpy(ext_value.data.get(), &o, sizeof(type));
                }
                return *this;
              }

              ~parameter()
              {
                if (size > k_embedded_size)
                  ext_value.~raw_data();
              }

              uint32_t get_size() const { return size; }
              const void* get_data() const
              {
                if (size <= k_embedded_size)
                  return &value;
                return ext_value.data.get();
              }

              id_t hash() const { return string_id::_runtime_build_from_string((const char*)get_data(), get_size()); }

            private:
              // size above which a dyn allocation is necessary
              static constexpr uint32_t k_embedded_size = sizeof(uint64_t) * 3;

              union
              {
                std::array<uint64_t, k_embedded_size / sizeof(uint64_t)> value = {0};
                raw_data ext_value;
              };
              uint32_t size = 0;
          };

        public:
          specialization() = default;
          specialization(specialization&&) = default;
          specialization& operator=(specialization&&) = default;
          ~specialization() = default;

//           specialization(std::map<id_t, parameter>&& p) : parameters(std::move(p)) {}
          template<size_t N>
          specialization(std::pair<id_t, parameter> (&&p)[N])
          {
            for (auto& it : p)
              parameters.emplace(it.first, std::move(it.second));
          }

          const parameter* get(id_t id) const
          {
            if (auto it = parameters.find(id); it != parameters.end())
              return &it->second;
            return nullptr;
          }

          template<typename T>
          void set(id_t id, T&& value)
          {
            parameters.insert_or_assign(id, parameter{std::forward<T>(value)});
          }

          template<typename T>
          T& add(id_t id)
          {
            return *(T*)parameters.insert_or_assign(id, parameter{T{}}).first->second.get_data();
          }

          size_t entry_count() const { return parameters.size(); }

          id_t hash() const
          {
            id_t h = id_t::none;
            for (const auto& it : parameters) { h = combine(h, it.second.hash()); }
            return h;
          }
        private:
          std::map<id_t, parameter> parameters;
      };

      /// \brief Wraps operations around VkSpecializationInfo & VkSpecializationMapEntry
      class specialization_info
      {
        public:
          specialization_info() = default;

          specialization_info(const specialization& s, const std::map<id_t, uint32_t>& constant_id_map)
          {
            update(s, constant_id_map);
          }

          /// \note a move invalidate the VkSpecializationInfo reference
          specialization_info(specialization_info &&o)
            : vk_specialization_info(o.vk_specialization_info), sme(std::move(o.sme)),
              data(std::move(o.data))
          {
            o.vk_specialization_info.pData = nullptr;
            o.vk_specialization_info.pMapEntries = nullptr;
          }

          /// \note a move invalidate the VkSpecializationInfo reference
          specialization_info &operator = (specialization_info &&o)
          {
            if (&o == this)
              return *this;
            vk_specialization_info = o.vk_specialization_info;
            sme = std::move(o.sme);
            data = std::move(o.data);

            o.vk_specialization_info.pData = nullptr;
            o.vk_specialization_info.pMapEntries = nullptr;
          }

          ~specialization_info() {}

          /// \brief Yield a reference to VkSpecializationInfo when needed
          operator VkSpecializationInfo *() { return &vk_specialization_info; }
          operator const VkSpecializationInfo *() const { return &vk_specialization_info; }

          void update(const specialization& s, const std::map<id_t, uint32_t>& constant_id_map)
          {
            sme.clear();
            data.reset();

            std::vector<const specialization::parameter*> parameters;
            parameters.reserve(s.entry_count());
            uint32_t total_size = 0;
            {
              for (const auto& it : constant_id_map)
              {
                if (const auto* p = s.get(it.first); p != nullptr)
                {
                  parameters.emplace_back(p);
                  sme.push_back(
                  {
                    .constantID = it.second,
                    .offset = total_size,
                    .size = p->get_size(),
                  });
                  total_size += p->get_size();
                }
              }
            }
            data = raw_data::allocate(total_size);
            uint32_t offset = 0;
            for (const auto* it : parameters)
            {
              memcpy((uint8_t*)data.get() + offset, it->get_data(), it->get_size());
              offset += it->get_size();
            }

            vk_specialization_info.dataSize = data.size;
            vk_specialization_info.pData = data;
            vk_specialization_info.mapEntryCount = sme.size();
            vk_specialization_info.pMapEntries = sme.data();
          }

        private:
          VkSpecializationInfo vk_specialization_info;

          std::vector<VkSpecializationMapEntry> sme;
          raw_data data;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



