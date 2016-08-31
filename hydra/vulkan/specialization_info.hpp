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

#ifndef __N_2119615343320201058_297491590_SPECIALIZATION_INFO_HPP__
#define __N_2119615343320201058_297491590_SPECIALIZATION_INFO_HPP__

#include <vector>
#include <vulkan/vulkan.h>

#include "../tools/memory_allocator.hpp" // used to get a fast-growing yet contiguous memory area
#include "../hydra_exception.hpp" // for check::

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps operations around VkSpecializationInfo & VkSpecializationMapEntry
      /// \note You have to keep that object alive if it is used in a hydra::vk::pipeline_shader_stage
      class specialization_info
      {
        public:
          specialization_info() { clear(); }

          /// \note a move invalidate the VkSpecializationInfo reference
          specialization_info(specialization_info &&o)
            : vk_specialization_info(o.vk_specialization_info), sme(std::move(o.sme)),
              data(std::move(o.data)), dirty(o.dirty), data_is_copy(o.data_is_copy)
          {
            o.vk_specialization_info.pData = nullptr;
            o.data_is_copy = false;
            o.clear();
          }

          /// \note a move invalidate the VkSpecializationInfo reference
          specialization_info &operator = (specialization_info &&o)
          {
            if (&o == this)
              return *this;
            clear();
            vk_specialization_info = o.vk_specialization_info;
            sme = std::move(o.sme);
            data = std::move(o.data);
            dirty = o.dirty;
            data_is_copy = o.data_is_copy;

            o.data_is_copy = false;
            o.vk_specialization_info.pData = nullptr;
            o.clear();
          }

          ~specialization_info() {clear();}

          /// \brief Add a new constant
          /// \note Type is copy constructed
          template<typename Type>
          Type &add_constant(uint32_t ID, Type value)
          {
            void *ret = _add_constant(ID, sizeof(Type));

            new (ret) Type(value); // copy construct the type

            return *reinterpret_cast<Type *>(ret);
          }

          /// \brief Add a new constant
          /// \note Type is default constructed
          template<typename Type>
          Type &add_constant(uint32_t ID)
          {
            void *ret = _add_constant(ID, sizeof(Type));

            new (ret) Type(); // default construct the type

            return *reinterpret_cast<Type *>(ret);
          }

          /// \brief Add a new constant, but the memory allocated in uninitialized and returned
          /// \note Type is default constructed
          void *_add_constant(uint32_t ID, size_t memory_size)
          {
            dirty = true;
            sme.push_back({
              ID,
              (uint32_t)data.size(),
              memory_size
            });

            void *ret = data.allocate(memory_size);
            check::on_vulkan_error::n_assert(data.has_failed() == false, "can't allocate more memory");

            return ret;
          }

          /// \brief Yield a reference to VkSpecializationInfo when needed
          operator VkSpecializationInfo &() { update(); return vk_specialization_info; }

          /// \brief Clear the specialization info
          void clear()
          {
            data.clear();
            sme.clear();

            vk_specialization_info.dataSize = 0;
            vk_specialization_info.mapEntryCount = 0;
            if (data_is_copy)
              operator delete((void *)vk_specialization_info.pData, std::nothrow);
            vk_specialization_info.pData = nullptr;
            vk_specialization_info.pMapEntries = nullptr;

            data_is_copy = false;
            dirty = false;
          }

          /// \brief Update the VkSpecializationInfo
          /// \note This will also update every reference to it
          void update()
          {
            if (!dirty)
              return;
            vk_specialization_info.dataSize = data.size();
            vk_specialization_info.mapEntryCount = sme.size();
            vk_specialization_info.pMapEntries = sme.data();

            if (data_is_copy)
              operator delete((void *)vk_specialization_info.pData, std::nothrow);

            if (data.is_data_contiguous())
            {
              data_is_copy = false; // no need to free anything
              vk_specialization_info.pData = data.get_contiguous_data();
            }
            else
            {
              data_is_copy = true; // will create a temporary copy
              vk_specialization_info.pData = data.get_contiguous_data_copy();
            }
          }

        private:
          VkSpecializationInfo vk_specialization_info;

          std::vector<VkSpecializationMapEntry> sme;
          cr::memory_allocator data;
          bool dirty = false;
          bool data_is_copy = false;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_2119615343320201058_297491590_SPECIALIZATION_INFO_HPP__

