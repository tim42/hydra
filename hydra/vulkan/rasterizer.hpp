//
// file : rasterizer.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/rasterizer.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 13 2016 11:30:30 GMT+0200 (CEST)
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

#ifndef __N_10440156862933621972_117153490_RASTERIZER_HPP__
#define __N_10440156862933621972_117153490_RASTERIZER_HPP__

#include <vulkan/vulkan.h>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wraps a VkPipelineRasterizationStateCreateInfo
      /// It controls & fine tune how the rasterizer will work for a given pipeline
      class rasterizer
      {
        public:
          /// \brief Create a rasterizer with some default values. (you may never need to touch this)
          rasterizer()
           : vk_rasterizer_nfo
          {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            nullptr,
            0,
            VK_FALSE, // depth clamp enable
            VK_FALSE, // discard sample
            VK_POLYGON_MODE_FILL, // poly mode
            VK_CULL_MODE_BACK_BIT,// cull mode
            VK_FRONT_FACE_CLOCKWISE, // front face
            VK_FALSE, // depth bias enable
            0.f, 0.f, 0.f, // depth bias {constant factor, clamp, slope factor}
            1.f, // line width
          }
          {
          }

          /// \brief Create a rasterizer with some default values
          static rasterizer create_default_rasterizer() { return rasterizer(); }
          /// \brief Create a rasterizer that discard every fragment
          static rasterizer create_null_rasterizer()
          {
            rasterizer ras;
            ras.set_discard_samples(true);
            return ras;
          }
          /// \brief Create a rasterizer with a polygon mode / line width != from the default
          static rasterizer create_rasterizer(VkPolygonMode poly, float line_width = 1.f)
          {
            rasterizer ras;
            ras.set_poligon_mode(poly);
            ras.set_line_width(line_width);
            return ras;
          }
          /// \brief Create a rasterizer with a cull mode / front face / line width != from the default
          static rasterizer create_rasterizer(VkCullModeFlags cull_mode, VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE, float line_width = 1.f)
          {
            rasterizer ras;
            ras.set_cull_mode(cull_mode);
            ras.set_front_face(front_face);
            ras.set_line_width(line_width);
            return ras;
          }

          /// \brief Copy constr.
          rasterizer(const rasterizer &o) : vk_rasterizer_nfo(o.vk_rasterizer_nfo) {}
          /// \brief Copy constr.
          rasterizer(const VkPipelineRasterizationStateCreateInfo &o) : vk_rasterizer_nfo(o)
          {
            vk_rasterizer_nfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            vk_rasterizer_nfo.pNext = nullptr;
            vk_rasterizer_nfo.flags = 0;
          }
          /// \brief Affectation operator
          rasterizer &operator = (const rasterizer &o) { vk_rasterizer_nfo = o.vk_rasterizer_nfo; return *this; }
          /// \brief Affectation operator
          rasterizer &operator = (const VkPipelineRasterizationStateCreateInfo  &o)
          {
            vk_rasterizer_nfo = o;
            vk_rasterizer_nfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            vk_rasterizer_nfo.pNext = nullptr;
            vk_rasterizer_nfo.flags = 0;
            return *this;
          }

          /// \brief Return the polygon mode
          VkPolygonMode get_poligon_mode() const { return vk_rasterizer_nfo.polygonMode; }
          /// \brief Return the cull mode
          VkCullModeFlags get_cull_mode() const { return vk_rasterizer_nfo.cullMode; }
          /// \brief Return how the front face is computed
          VkFrontFace get_front_face() const { return vk_rasterizer_nfo.frontFace; }
          /// \brief Return true if the rasterizer discard pixels before the pixel shader stage
          bool discard_samples() const { return vk_rasterizer_nfo.rasterizerDiscardEnable == VK_TRUE; }
          /// \brief Return true if the rasterizer clamp fragment depth values
          bool depth_clamp_enabled() const { return vk_rasterizer_nfo.depthClampEnable == VK_TRUE; }
          /// \brief Return if the depth bias is enabled
          bool depth_bias_enabled() const { return vk_rasterizer_nfo.depthBiasEnable == VK_TRUE; }

          float get_depth_bias_constant_factor() const { return vk_rasterizer_nfo.depthBiasConstantFactor; }
          float get_depth_bias_clamp() const { return vk_rasterizer_nfo.depthBiasClamp; }
          float get_depth_bias_slope_factor() const { return vk_rasterizer_nfo.depthBiasSlopeFactor; }

          /// \brief Return the line width
          float get_line_width() const { return vk_rasterizer_nfo.lineWidth; }

          /// \brief Set the polygon mode
          void set_poligon_mode(VkPolygonMode pmode) { vk_rasterizer_nfo.polygonMode = pmode; }
          /// \brief Set the cull mode
          void set_cull_mode(VkCullModeFlags cull_mode) { vk_rasterizer_nfo.cullMode = cull_mode; }
          /// \brief Set how the front face is computed
          void set_front_face(VkFrontFace ff) { vk_rasterizer_nfo.frontFace = ff; }
          /// \brief true if the rasterizer discard pixels before the pixel shader stage
          void set_discard_samples(bool discard) { vk_rasterizer_nfo.rasterizerDiscardEnable = discard ? VK_TRUE : VK_FALSE; }
          /// \brief true if the rasterizer clamp fragment depth values
          void enabled_depth_clamp(bool enabled) { vk_rasterizer_nfo.depthClampEnable = enabled ? VK_TRUE : VK_FALSE; }
          /// \brief Set if the depth bias is enabled
          void enable_depth_bias(bool enabled) { vk_rasterizer_nfo.depthBiasEnable = enabled ? VK_TRUE : VK_FALSE; }

          void set_depth_bias_constant_factor(float factor) { vk_rasterizer_nfo.depthBiasConstantFactor = factor; }
          void set_depth_bias_clamp(float clmp) { vk_rasterizer_nfo.depthBiasClamp = clmp; }
          void set_depth_bias_slope_factor(float factor) { vk_rasterizer_nfo.depthBiasSlopeFactor = factor; }

          /// \brief Set the line width
          void set_line_width(float width = 1.f) { vk_rasterizer_nfo.lineWidth = width; }

        public: // advanced
          /// \brief Yield a VkPipelineRasterizationStateCreateInfo
          operator const VkPipelineRasterizationStateCreateInfo &() const { return vk_rasterizer_nfo; }

        private:
          VkPipelineRasterizationStateCreateInfo vk_rasterizer_nfo;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_10440156862933621972_117153490_RASTERIZER_HPP__

