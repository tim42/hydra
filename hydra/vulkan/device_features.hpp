//
// file : device_features.hpp
// in : file:///home/tim/projects/hydra/hydra/init/device_features.hpp
//
// created by : Timothée Feuillet
// date: Tue Apr 26 2016 22:55:50 GMT+0200 (CEST)
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


#include <map>
#include <vulkan/vulkan.h>
#include <ntools/ct_list.hpp>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      namespace internal
      {
        template<typename VkFeatureStruct, VkStructureType Type>
        struct feature_info_impl_t
        {
          using type = VkFeatureStruct;

          static constexpr VkStructureType struct_type = Type;

          static constexpr uint32_t data_size = (sizeof(VkFeatureStruct) - sizeof(VkBaseInStructure));
          // If the number of entries is not even, the padding will be treated as a valid entry
          static constexpr uint32_t entry_count = data_size / sizeof(VkBool32);
        };

        template<VkStructureType Type>
        struct feature_info_from_name_t
        {
          // bad struct
        };

        template<typename Type>
        struct feature_info_from_type_t
        {
          // bad struct
        };


#define FEATURE_INFO(X) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR, VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR, VkPhysicalDeviceAccelerationStructureFeaturesKHR) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR, VkPhysicalDeviceRayTracingPipelineFeaturesKHR) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT, VkPhysicalDeviceMeshShaderFeaturesEXT) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT, VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR, VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT, VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT, VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT, VkPhysicalDeviceShaderAtomicFloatFeaturesEXT) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT, VkPhysicalDeviceDescriptorBufferFeaturesEXT) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT, VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT, VkPhysicalDeviceRobustness2FeaturesEXT) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, VkPhysicalDeviceVulkan13Features) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, VkPhysicalDeviceVulkan12Features) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, VkPhysicalDeviceVulkan11Features) \
        X(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, VkPhysicalDeviceFeatures2) /* NOTE: MUST be last in this list */\

#define DECLARE_FEATURE_INFO(STRUCT_TYPE, STRUCT)       template<> struct feature_info_from_name_t<STRUCT_TYPE> : public feature_info_impl_t<STRUCT, STRUCT_TYPE> {}; \
                                                        template<> struct feature_info_from_type_t<STRUCT> : public feature_info_impl_t<STRUCT, STRUCT_TYPE> {};
      FEATURE_INFO(DECLARE_FEATURE_INFO)
#undef DECLARE_FEATURE_INFO

#define FEATURE_INFO_LIST_ENTRY(STRUCT_TYPE, STRUCT)    STRUCT,
      using feature_struct_list = ct::list::remove_type
      <
        ct::type_list
        <
        FEATURE_INFO(FEATURE_INFO_LIST_ENTRY)
        void // extra padding, to account for the trailing ','
        >,
        // remove the extra type:
        void
      >;
#undef  FEATURE_INFO_LIST_ENTRY
#undef FEATURE_INFO
      }

      /// \brief Represent a vulkan device feature with some nice utilities
      /// \warning The size of this struct is quite high, please beware of this.
      class device_features
      {
        public:
          /// \brief Convert a vulkan device feature list to a hydra one
          device_features(const VkPhysicalDeviceFeatures2& vk_features2) : device_features()
          {
            // this is a lazy way to do this, but this code is not made to be called outside init, and should only ever be called once
            std::map<VkStructureType, const void*> init_entry_list;

            // fill the map with the different structures from the list:
            const VkBaseInStructure* it = (const VkBaseInStructure*)&vk_features2.pNext;
            while (it != nullptr)
            {
              init_entry_list.emplace(it->sType, it);
              it = it->pNext;
            }

            // copy the entries in the feature_list:
            for_each_entries(feature_list, [&init_entry_list](auto& entry)
            {
              using info_t = internal::feature_info_from_type_t<std::remove_cvref_t<decltype(entry)>>;
              if (auto it = init_entry_list.find(info_t::struct_type); it != init_entry_list.end())
                memcpy(to_data(entry), to_data(it->second), info_t::data_size);
              else
                memset(to_data(entry), 0, info_t::data_size);
            });
          }

          device_features() { clear(); }
          device_features(const device_features &o) = default;
          device_features &operator = (const device_features &o) = default;


          /// \brief Allow ORing device feature lists
          device_features &operator |= (const device_features &o)
          {
            for_each_entries(feature_list, o.feature_list, [](auto& entry_a, const auto& entry_b)
            {
              struct_binary_or(entry_a, entry_b);
            });
            return *this;
          }

          /// \brief Allow AND operations on device feature lists
          device_features &operator &= (const device_features &o)
          {
            for_each_entries(feature_list, o.feature_list, [](auto& entry_a, const auto& entry_b)
            {
              struct_binary_and(entry_a, entry_b);
            });
            return *this;
          }

          /// \brief Negate the feature list
          device_features operator ~() const
          {
            device_features ret = *this;
            for_each_entries(ret.feature_list, [](auto& entry)
            {
              struct_binary_negate(entry);
            });

            return ret;
          }

          /// \brief Allow comparisons on device feature lists
          bool operator == (const device_features &o) const
          {
            bool ret = true;
            for_each_entries(feature_list, o.feature_list, [&ret](const auto& entry_a, const auto& entry_b)
            {
              ret = ret && struct_equality(entry_a, entry_b);
            });
            return ret;
          }

          /// \brief Allow comparisons on device feature lists
          /// \note This isn't strict difference (where every value needs to be different)
          ///       but the operator returns true if at least one value is different
          bool operator != (const device_features &o) const
          {
            return !(*this == o);
          }

          /// \brief Check the current device feature list against another, returning true
          /// if any of the features activated in this list is also activated in the other,
          /// false if there's an activated feature in this list that isn't in the other
          bool check_against(const device_features &mask) const
          {
            const device_features zero;
            device_features tmp = *this;
            tmp &= ~mask;
            return tmp == zero;
          }

          /// \brief Deactivate every feature in the list
          void clear()
          {
            void* prev = nullptr;
            for_each_entries(feature_list, [&prev](auto& entry)
            {
              struct_init(entry, prev);
              prev = &entry;
            });
          }

          /// \brief Remove unused structs from the vulkan linked-list
          void simplify()
          {
            void* prev = nullptr;
            for_each_entries(feature_list, [&prev](auto& entry)
            {
              if (!struct_is_default(entry))
              {
                entry.pNext = prev;
                prev = &entry;
              }
            });
          }

          // getters: both get<type> and get<VK_STRUCT_TYPE> are accepted

          template<typename Type> Type& get() { return std::get<Type>(feature_list); }
          template<typename Type> const Type& get() const { return std::get<Type>(feature_list); }

          template<VkStructureType StructType>
          auto& get()
          {
            using type_t = typename internal::feature_info_from_name_t<StructType>::type;
            return std::get<type_t>(feature_list);
          }
          template<VkStructureType StructType>
          const auto& get() const
          {
            using type_t = typename internal::feature_info_from_name_t<StructType>::type;
            return std::get<type_t>(feature_list);
          }

        public:
          VkPhysicalDeviceFeatures& get_device_features()
          {
            return get<VkPhysicalDeviceFeatures2>().features;
          }
          const VkPhysicalDeviceFeatures& get_device_features() const
          {
            return get<VkPhysicalDeviceFeatures2>().features;
          }
          const void* _get_VkDeviceCreateInfo_pNext() const
          {
            return get<VkPhysicalDeviceFeatures2>().pNext;
          }

        private:
          template<typename Function, typename... Types>
          static void for_each_entries(std::tuple<Types...>& a, Function&& fnc)
          {
            (fnc(std::get<Types>(a)), ...);
          }
          template<typename Function, typename... Types>
          static void for_each_entries(std::tuple<Types...>& a, const std::tuple<Types...>& b, Function&& fnc)
          {
            (fnc(std::get<Types>(a), std::get<Types>(b)), ...);
          }
          template<typename Function, typename... Types>
          static void for_each_entries(const std::tuple<Types...>& a, const std::tuple<Types...>& b, Function&& fnc)
          {
            (fnc(std::get<Types>(a), std::get<Types>(b)), ...);
          }

          template<typename Data>
          static uint32_t* to_data(Data& a)
          {
            return reinterpret_cast<uint32_t*>(&a) + (sizeof(VkBaseInStructure) / sizeof(uint32_t));
          }
          template<typename Data>
          static const uint32_t* to_data(const Data& a)
          {
            return reinterpret_cast<const uint32_t*>(&a) + (sizeof(VkBaseInStructure) / sizeof(uint32_t));
          }

          template<typename Type>
          static void struct_binary_or(Type& a, const Type& b)
          {
            using info_t = internal::feature_info_from_type_t<Type>;
            uint32_t* a_data = to_data(a);
            const uint32_t* b_data = to_data(b);

            for (uint32_t i = 0; i < info_t::entry_count; ++i)
              a_data[i] = a_data[i] | b_data[i];
          }
          template<typename Type>
          static void struct_binary_and(Type& a, const Type& b)
          {
            using info_t = internal::feature_info_from_type_t<Type>;
            uint32_t* a_data = to_data(a);
            const uint32_t* b_data = to_data(b);

            for (uint32_t i = 0; i < info_t::entry_count; ++i)
              a_data[i] = (a_data[i] != 0) && (b_data[i] != 0);
          }
          template<typename Type>
          static void struct_binary_negate(Type& a)
          {
            using info_t = internal::feature_info_from_type_t<Type>;
            uint32_t* a_data = to_data(a);

            for (uint32_t i = 0; i < info_t::entry_count; ++i)
              a_data[i] = !a_data[i];
          }
          template<typename Type>
          static bool struct_equality(const Type& a, const Type& b)
          {
            using info_t = internal::feature_info_from_type_t<Type>;
            const uint32_t* a_data = to_data(a);
            const uint32_t* b_data = to_data(b);

            for (uint32_t i = 0; i < info_t::entry_count; ++i)
            {
              if ((a_data[i] != 0) != (b_data[i] != 0))
                return false;
            }
            return true;
          }
          template<typename Type>
          static bool struct_is_default(Type& a)
          {
            using info_t = internal::feature_info_from_type_t<Type>;
            const uint32_t* a_data = to_data(a);

            for (uint32_t i = 0; i < info_t::entry_count; ++i)
            {
              if ((a_data[i] != 0))
                return false;
            }
            return true;
          }
          template<typename Type>
          static void struct_init(Type& a, void* prev = nullptr)
          {
            using info_t = internal::feature_info_from_type_t<Type>;
            a.sType = info_t::struct_type;
            a.pNext = prev;

            // set the fields to 0:
            memset(to_data(a), 0, info_t::data_size);
          }

        private:
          // Is a tuple of all the different feature structs:
          ct::list::extract<internal::feature_struct_list>::as<std::tuple> feature_list;
      };

      /// \brief | operator overload
      inline device_features operator | (const device_features &d1, const device_features &d2)
      {
        device_features r = d1;
        r |= d2;
        return r;
      }

      /// \brief & operator overload
      inline device_features operator & (const device_features &d1, const device_features &d2)
      {
        device_features r = d1;
        r &= d2;
        return r;
      }
    } // namespace vk
  } // namespace hydra
} // namespace neam



