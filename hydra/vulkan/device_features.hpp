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

#ifndef __N_258283952455712780_248821193_DEVICE_FEATURES_HPP__
#define __N_258283952455712780_248821193_DEVICE_FEATURES_HPP__

#include <vulkan/vulkan.h>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Represent a vulkan device feature with some nice utilities
      class device_features
      {
        public:
          /// \brief Convert a vulkan device feature list to a hydra one
          device_features(const VkPhysicalDeviceFeatures &vk_features)
          {
            // This is just a safeguard, as the header may change and this will abort the compilation if the structure contains more/less members
            static_assert(sizeof(VkPhysicalDeviceFeatures) / 4 == feature_count, "it looks like you have an unsupported version of the vulkan header");

            // this is a lazy way to do this...
            // this is full of suppositions, but I believe most compilers will align 32bit integers on 32bit offsets,
            // and 8bit integers to 8bit offset, so we should be OK
            // I also suppose that khronos will not replace/move features, simply add (or remove) them
            const VkBool32 *orig_ptr = reinterpret_cast<const VkBool32 *>(&vk_features);
            uint8_t *dest_ptr = &robust_buffer_access;
            for (size_t i = 0; i < feature_count; ++i)
              dest_ptr[i] = (orig_ptr[i] == VK_TRUE ? 1 : 0);
          }

          device_features() { clear(); }
          device_features(const device_features &o) = default;
          device_features &operator = (const device_features &o) = default;


          /// \brief Allow ORing device feature lists
          device_features &operator |= (const device_features &o)
          {
            uint8_t *this_iter_ptr = &robust_buffer_access;
            const uint8_t *o_iter_ptr = &o.robust_buffer_access;
            for (size_t i = 0; i < feature_count; ++i)
              this_iter_ptr[i] |= o_iter_ptr[i];
            return *this;
          }

          /// \brief Allow AND operations on device feature lists
          device_features &operator &= (const device_features &o)
          {
            uint8_t *this_iter_ptr = &robust_buffer_access;
            const uint8_t *o_iter_ptr = &o.robust_buffer_access;
            for (size_t i = 0; i < feature_count; ++i)
              this_iter_ptr[i] = (this_iter_ptr[i] != 0) && (o_iter_ptr[i] != 0);
            return *this;
          }

          /// \brief Negate the feature list
          device_features operator ~() const
          {
            device_features ret;
            const uint8_t *this_iter_ptr = &robust_buffer_access;
            uint8_t *o_iter_ptr = &ret.robust_buffer_access;
            for (size_t i = 0; i < feature_count; ++i)
              o_iter_ptr[i] = (this_iter_ptr[i] == 0);
            return ret;
          }

          /// \brief Allow comparisons on device feature lists
          bool operator == (const device_features &o) const
          {
            bool ret = true;
            const uint8_t *this_iter_ptr = &robust_buffer_access;
            const uint8_t *o_iter_ptr = &o.robust_buffer_access;
            for (size_t i = 0; i < feature_count; ++i)
              ret &= (this_iter_ptr[i] != 0) == (o_iter_ptr[i] != 0);
            return ret;
          }

          /// \brief Allow comparisons on device feature lists
          /// \note This isn't strict difference (where every value needs to be different)
          ///       but the operator returns true if at least one value is different
          bool operator != (const device_features &o) const
          {
            bool ret = false;
            const uint8_t *this_iter_ptr = &robust_buffer_access;
            const uint8_t *o_iter_ptr = &o.robust_buffer_access;
            for (size_t i = 0; i < feature_count; ++i)
              ret |= (this_iter_ptr[i] != 0) != (o_iter_ptr[i] != 0);
            return ret;
          }

          /// \brief Check the current device feature list against another, returning true
          /// if any of the features activated in this list is also activated in the other,
          /// false if there's an activated feature in this list that isn't in the other
          bool check_against(const device_features &mask) const
          {
            bool is_bad = false;
            const uint8_t *this_iter_ptr = &robust_buffer_access;
            const uint8_t *mask_iter_ptr = &mask.robust_buffer_access;
            for (size_t i = 0; i < feature_count; ++i)
              is_bad |= (this_iter_ptr[i] != 0 && mask_iter_ptr[i] == 0);
            return !is_bad;
          }

          /// \brief Deactivate every feature in the list
          void clear()
          {
            uint8_t *this_iter_ptr = &robust_buffer_access;
            for (size_t i = 0; i < feature_count; ++i)
              this_iter_ptr[i] = 0;
          }

        public: // advanced
          /// \brief Convert a hydra device feature list back to a vulkan one
          VkPhysicalDeviceFeatures _to_vulkan() const
          {
            VkPhysicalDeviceFeatures ret;

            VkBool32 *dest_ptr = reinterpret_cast<VkBool32 *>(&ret);
            const uint8_t *orig_ptr = &robust_buffer_access;
            for (size_t i = 0; i < feature_count; ++i)
              dest_ptr[i] = (orig_ptr[i] != 0 ? VK_TRUE : VK_FALSE);

            return ret;
          }

        public: // I don't have getters/setters, that would be horrible
          uint8_t    robust_buffer_access;
          uint8_t    full_draw_index_uint32;
          uint8_t    image_cube_array;
          uint8_t    independent_blend;
          uint8_t    geometry_shader;
          uint8_t    tessellation_shader;
          uint8_t    sample_rate_shading;
          uint8_t    dual_src_blend;
          uint8_t    logic_op;
          uint8_t    multi_draw_indirect;
          uint8_t    draw_indirect_first_instance;
          uint8_t    depth_clamp;
          uint8_t    depth_bias_clamp;
          uint8_t    fill_mode_non_solid;
          uint8_t    depth_bounds;
          uint8_t    wide_lines;
          uint8_t    large_points;
          uint8_t    alpha_to_one;
          uint8_t    multi_viewport;
          uint8_t    sampler_anisotropy;
          uint8_t    texture_compression_etc2;
          uint8_t    texture_compression_astc_ldr;
          uint8_t    texture_compression_bc;
          uint8_t    occlusion_query_precise;
          uint8_t    pipeline_statistics_query;
          uint8_t    vertex_pipeline_stores_and_atomics;
          uint8_t    fragment_stores_and_atomics;
          uint8_t    shader_tessellation_and_geometry_point_size;
          uint8_t    shader_image_gather_extended;
          uint8_t    shader_storage_image_extended_formats;
          uint8_t    shader_storage_image_multisample;
          uint8_t    shader_storage_image_read_without_format;
          uint8_t    shader_storage_image_write_without_format;
          uint8_t    shader_uniform_buffer_array_dynamic_indexing;
          uint8_t    shader_sampled_image_array_dynamic_indexing;
          uint8_t    shader_storage_buffer_array_dynamic_indexing;
          uint8_t    shader_storage_image_array_dynamic_indexing;
          uint8_t    shader_clip_distance;
          uint8_t    shader_cull_distance;
          uint8_t    shader_float64;
          uint8_t    shader_int64;
          uint8_t    shader_int16;
          uint8_t    shader_resource_residency;
          uint8_t    shader_resource_min_lod;
          uint8_t    sparse_binding;
          uint8_t    sparse_residency_buffer;
          uint8_t    sparse_residency_image2_d;
          uint8_t    sparse_residency_image3_d;
          uint8_t    sparse_residency2_samples;
          uint8_t    sparse_residency4_samples;
          uint8_t    sparse_residency8_samples;
          uint8_t    sparse_residency16_samples;
          uint8_t    sparse_residency_aliased;
          uint8_t    variable_multisample_rate;
          uint8_t    inherited_queries;

        private:
          static constexpr size_t feature_count = 55;
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

#endif // __N_258283952455712780_248821193_DEVICE_FEATURES_HPP__

