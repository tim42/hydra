//
// file : command_buffer_recorder.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/command_buffer_recorder.hpp
//
// created by : Timothée Feuillet
// date: Thu Aug 25 2016 10:57:00 GMT+0200 (CEST)
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

#ifndef __N_1232927689274571835_1298719218_COMMAND_BUFFER_RECORDER_HPP__
#define __N_1232927689274571835_1298719218_COMMAND_BUFFER_RECORDER_HPP__

#include <vulkan/vulkan.h>

#include "../hydra_debug.hpp"

#include "device.hpp"
#include "command_buffer.hpp"
#include "pipeline.hpp"
#include "viewport.hpp"
#include "rect2D.hpp"
#include "image.hpp"
#include "image_subresource_range.hpp"
#include "image_subresource_layers.hpp"
#include "image_copy_area.hpp"
#include "image_blit_area.hpp"
#include "event.hpp"
#include "clear_value.hpp"
#include "buffer.hpp"
#include "memory_barrier.hpp"
#include "buffer_image_copy.hpp"
#include "descriptor_set.hpp"
#include "pipeline_layout.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief The sole purpose of this class is to record commands to a command buffer
      class command_buffer_recorder
      {
        public: // advanced
          command_buffer_recorder(device &_dev, command_buffer &_cmd_buff) : dev(_dev), cmd_buff(_cmd_buff) {}
          command_buffer_recorder(const command_buffer_recorder &o) : dev(o.dev), cmd_buff(o.cmd_buff) {}

        public:
          /// \brief Bind a pipeline object to a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBindPipeline.html">vulkan khr doc</a>
          void bind_pipeline(const pipeline &p, VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS)
          {
            last_bound_pipeline = &p;
            if (!last_bound_pipeline->is_valid())
              return;
            dev._fn_vkCmdBindPipeline(cmd_buff._get_vk_command_buffer(), bind_point, p.get_vk_pipeline());
          }

          /// \brief Return the last bound pipeline (if any)
          const pipeline *get_last_bound_pipeline() const { return last_bound_pipeline; }

          /// \brief Set the viewport on a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetViewport.html">vulkan khr doc</a>
          void set_viewport(const std::vector<viewport> &viewports, size_t offset = 0, size_t count = (size_t)-1)
          {
            if (count == (size_t)-1) count = viewports.size();
            std::vector<VkViewport> vk_viewports;
            vk_viewports.reserve(viewports.size());
            for (const viewport &it : viewports)
              vk_viewports.push_back(it);
            dev._fn_vkCmdSetViewport(cmd_buff._get_vk_command_buffer(), offset, count, vk_viewports.data());
          }

          /// \brief Set the viewport on a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetViewport.html">vulkan khr doc</a>
          void set_viewport(const viewport &viewport)
          {
            dev._fn_vkCmdSetViewport(cmd_buff._get_vk_command_buffer(), 0, 1, &static_cast<const VkViewport &>(viewport));
          }

          /// \brief Set the dynamic scissor rectangles on a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetScissor.html">vulkan khr doc</a>
          void set_scissor(const std::vector<rect2D> &scissors, size_t offset = 0, size_t count = (size_t)-1)
          {
            if (count == (size_t)-1) count = scissors.size();
            std::vector<VkRect2D> vk_scissors;
            vk_scissors.reserve(scissors.size());
            for (const rect2D &it : scissors)
              vk_scissors.push_back(it);
            dev._fn_vkCmdSetScissor(cmd_buff._get_vk_command_buffer(), offset, count, vk_scissors.data());
          }

          /// \brief Set the dynamic scissor rectangles on a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetScissor.html">vulkan khr doc</a>
          void set_scissor(const rect2D &scissor)
          {
            dev._fn_vkCmdSetScissor(cmd_buff._get_vk_command_buffer(), 0, 1, &static_cast<const VkRect2D &>(scissor));
          }

          /// \brief Set the dynamic line width state
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetLineWidth.html">vulkan khr doc</a>
          void set_line_width(float line_width)
          {
            dev._fn_vkCmdSetLineWidth(cmd_buff._get_vk_command_buffer(), line_width);
          }

          /// \brief Set the depth bias dynamic state
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetDepthBias.html">vulkan khr doc</a>
          void set_depth_bias(float constant_factor, float bias_clamp, float slope_factor)
          {
            dev._fn_vkCmdSetDepthBias(cmd_buff._get_vk_command_buffer(), constant_factor, bias_clamp, slope_factor);
          }

          /// \brief Set the values of blend constants
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetBlendConstants.html">vulkan khr doc</a>
          void set_blend_constants(const glm::vec4 &blend_constants)
          {
            float _blend_constants[] = {blend_constants.x, blend_constants.y, blend_constants.z, blend_constants.w};
            dev._fn_vkCmdSetBlendConstants(cmd_buff._get_vk_command_buffer(), _blend_constants);
          }

          /// \brief Set the depth bounds test values for a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetDepthBounds.html">vulkan khr doc</a>
          void set_depth_bounds(float min, float max)
          {
            dev._fn_vkCmdSetDepthBounds(cmd_buff._get_vk_command_buffer(), min, max);
          }

          /// \brief Set the stencil compare mask dynamic state
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetStencilCompareMask.html">vulkan khr doc</a>
          void set_stencil_compare_mask(VkStencilFaceFlags face_mask, uint32_t compare_mask)
          {
            dev._fn_vkCmdSetStencilCompareMask(cmd_buff._get_vk_command_buffer(), face_mask, compare_mask);
          }

          /// \brief Set the stencil write mask dynamic state
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetStencilWriteMask.html">vulkan khr doc</a>
          void set_stencil_write_mask(VkStencilFaceFlags face_mask, uint32_t write_mask)
          {
            dev._fn_vkCmdSetStencilWriteMask(cmd_buff._get_vk_command_buffer(), face_mask, write_mask);
          }

          /// \brief Set the stencil reference dynamic state
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetStencilReference.html">vulkan khr doc</a>
          void set_stencil_reference(VkStencilFaceFlags face_mask, uint32_t reference)
          {
            dev._fn_vkCmdSetStencilReference(cmd_buff._get_vk_command_buffer(), face_mask, reference);
          }

          /// \brief Record a draw command
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDraw.html">vulkan khr doc</a>
          /// \todo maybe something that will encapsulate vertex count and first vertex
          void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
          {
            if (!last_bound_pipeline || !last_bound_pipeline->is_valid())
              return;

            dev._fn_vkCmdDraw(cmd_buff._get_vk_command_buffer(), vertex_count, instance_count, first_vertex, first_instance);
          }

          /// \brief Issue an indirect draw into a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDrawIndirect.html">vulkan khr doc</a>
          /// \todo maybe something that will encapsulate vertex count and first vertex
          void draw_indirect(const buffer &buf, size_t offset, uint32_t draw_count, uint32_t stride)
          {
            if (!last_bound_pipeline || !last_bound_pipeline->is_valid())
              return;

            dev._fn_vkCmdDrawIndirect(cmd_buff._get_vk_command_buffer(), buf._get_vk_buffer(), offset, draw_count, stride);
          }

          /// \brief Issue an indexed draw into a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDrawIndexed.html">vulkan khr doc</a>
          /// \todo maybe something that will encapsulate index count, first index & vertex offset
          void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
          {
            if (!last_bound_pipeline || !last_bound_pipeline->is_valid())
              return;

            dev._fn_vkCmdDrawIndexed(cmd_buff._get_vk_command_buffer(), index_count, instance_count, first_index, vertex_offset, first_instance);
          }

          /// \brief Issue an indirect draw into a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDrawIndexedIndirect.html">vulkan khr doc</a>
          /// \todo maybe something that will encapsulate vertex count and first vertex
          void draw_indexed_indirect(const buffer &buf, size_t offset, uint32_t draw_count, uint32_t stride)
          {
            if (!last_bound_pipeline || !last_bound_pipeline->is_valid())
              return;

            dev._fn_vkCmdDrawIndexedIndirect(cmd_buff._get_vk_command_buffer(), buf._get_vk_buffer(), offset, draw_count, stride);
          }

          /// \brief Dispatch compute work items
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDispatch.html">vulkan khr doc</a>
          void dispatch(uint32_t x, uint32_t y, uint32_t z)
          {
            if (!last_bound_pipeline || !last_bound_pipeline->is_valid())
              return;

            dev._fn_vkCmdDispatch(cmd_buff._get_vk_command_buffer(), x, y, z);
          }

          /// \brief Dispatch compute work items
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDispatch.html">vulkan khr doc</a>
          void dispatch(const glm::uvec3 &work_group_num) { dispatch(work_group_num.x, work_group_num.y, work_group_num.z); }

          /// \brief Dispatch compute work items using indirect parameters
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdDispatchIndirect.html">vulkan khr doc</a>
          void dispatch_indirect(const buffer &buf, size_t offset = 0)
          {
            if (!last_bound_pipeline || !last_bound_pipeline->is_valid())
              return;

            dev._fn_vkCmdDispatchIndirect(cmd_buff._get_vk_command_buffer(), buf._get_vk_buffer(), offset);
          }

          /// \brief Copy data between images
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdCopyImage.html">vulkan khr doc</a>
          void copy_image(const image &source_image, VkImageLayout src_layout, const image &dest_image, VkImageLayout dest_layout, const std::vector<image_copy_area> &cp_vct)
          {
            std::vector<VkImageCopy> vk_cp_vct;
            vk_cp_vct.reserve(cp_vct.size());
            for (const image_copy_area &it : cp_vct)
              vk_cp_vct.push_back(it);
            dev._fn_vkCmdCopyImage(cmd_buff._get_vk_command_buffer(), source_image.get_vk_image(), src_layout, dest_image.get_vk_image(), dest_layout, vk_cp_vct.size(), vk_cp_vct.data());
          }

          /// \brief Copy regions of an image, potentially performing format conversion
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBlitImage.html">vulkan khr doc</a>
          void blit_image(const image &source_image, VkImageLayout src_layout, const image &dest_image, VkImageLayout dest_layout, const std::vector<image_blit_area> &cp_vct,
                          VkFilter filter = VK_FILTER_LINEAR)
          {
            std::vector<VkImageBlit> vk_cp_vct;
            vk_cp_vct.reserve(cp_vct.size());
            for (const image_blit_area &it : cp_vct)
              vk_cp_vct.push_back(it);
            dev._fn_vkCmdBlitImage(cmd_buff._get_vk_command_buffer(), source_image.get_vk_image(), src_layout, dest_image.get_vk_image(), dest_layout, vk_cp_vct.size(), vk_cp_vct.data(), filter);
          }

          /// \brief Clear regions of a color image
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdClearColorImage.html">vulkan khr doc</a>
          void clear_color_image(const image &img, VkImageLayout layout, const glm::vec4 &color, const std::vector<image_subresource_range> &isr_vect)
          {
            VkClearColorValue ccv
            {
              .float32 = {color.r, color.g, color.b, color.a}
            };
            std::vector<VkImageSubresourceRange> vk_isr_vect;
            vk_isr_vect.reserve(isr_vect.size());
            for (const image_subresource_range &it : isr_vect)
              vk_isr_vect.push_back(it);
            dev._fn_vkCmdClearColorImage(cmd_buff._get_vk_command_buffer(), img.get_vk_image(), layout, &ccv, vk_isr_vect.size(), vk_isr_vect.data());
          }

          /// \brief Clear regions of a color image
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdClearColorImage.html">vulkan khr doc</a>
          void clear_color_image(const image &img, VkImageLayout layout, const glm::uvec4 &color, const std::vector<image_subresource_range> &isr_vect)
          {
            VkClearColorValue ccv
            {
              .uint32 = {color.r, color.g, color.b, color.a}
            };
            std::vector<VkImageSubresourceRange> vk_isr_vect;
            vk_isr_vect.reserve(isr_vect.size());
            for (const image_subresource_range &it : isr_vect)
              vk_isr_vect.push_back(it);
            dev._fn_vkCmdClearColorImage(cmd_buff._get_vk_command_buffer(), img.get_vk_image(), layout, &ccv, vk_isr_vect.size(), vk_isr_vect.data());
          }

          /// \brief Clear regions of a color image
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdClearColorImage.html">vulkan khr doc</a>
          void clear_color_image(const image &img, VkImageLayout layout, const glm::ivec4 &color, const std::vector<image_subresource_range> &isr_vect)
          {
            VkClearColorValue ccv
            {
              .int32 = {color.r, color.g, color.b, color.a}
            };
            std::vector<VkImageSubresourceRange> vk_isr_vect;
            vk_isr_vect.reserve(isr_vect.size());
            for (const image_subresource_range &it : isr_vect)
              vk_isr_vect.push_back(it);
            dev._fn_vkCmdClearColorImage(cmd_buff._get_vk_command_buffer(), img.get_vk_image(), layout, &ccv, vk_isr_vect.size(), vk_isr_vect.data());
          }

          /// \brief  Fill regions of a combined depth-stencil image
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdClearDepthStencilImage.html">vulkan khr doc</a>
          void clear_depth_stencil_image(const image &img, VkImageLayout layout, float depth, uint32_t stencil, const std::vector<image_subresource_range> &isr_vect)
          {
            VkClearDepthStencilValue cdsv { depth, stencil };
            std::vector<VkImageSubresourceRange> vk_isr_vect;
            vk_isr_vect.reserve(isr_vect.size());
            for (const image_subresource_range &it : isr_vect)
              vk_isr_vect.push_back(it);
            dev._fn_vkCmdClearDepthStencilImage(cmd_buff._get_vk_command_buffer(), img.get_vk_image(), layout, &cdsv, vk_isr_vect.size(), vk_isr_vect.data());
          }

          /// \brief Set an event object to signaled state
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdSetEvent.html">vulkan khr doc</a>
          void set_event(const event &evt, VkPipelineStageFlags stage_mask)
          {
            dev._fn_vkCmdSetEvent(cmd_buff._get_vk_command_buffer(), evt._get_vk_event(), stage_mask);
          }

          /// \brief  Reset an event object to non-signaled state
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdResetEvent.html">vulkan khr doc</a>
          void reset_event(const event &evt, VkPipelineStageFlags stage_mask)
          {
            dev._fn_vkCmdResetEvent(cmd_buff._get_vk_command_buffer(), evt._get_vk_event(), stage_mask);
          }

          /// \brief Update the values of push constants
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdPushConstants.html">vulkan khr doc</a>
          void push_constants(const pipeline_layout &pl, VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size, const void *values)
          {
            dev._fn_vkCmdPushConstants(cmd_buff._get_vk_command_buffer(), pl._get_vk_pipeline_layout(), stage_flags, offset, size, values);
          }

          /// \brief Update the values of push constants
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdPushConstants.html">vulkan khr doc</a>
          template<typename Type>
          void push_constants(const pipeline_layout &pl, VkShaderStageFlags stage_flags, uint32_t offset, const Type &value)
          {
            dev._fn_vkCmdPushConstants(cmd_buff._get_vk_command_buffer(), pl._get_vk_pipeline_layout(), stage_flags, offset, sizeof(value), (const void *)&value);
          }

          /// \brief Begin a new render pass
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBeginRenderPass.html">vulkan khr doc</a>
          void begin_render_pass(const render_pass &rp, const framebuffer &fb, const rect2D &area, VkSubpassContents sp_contents, const std::vector<clear_value> &cv)
          {
            VkRenderPassBeginInfo vk_rpb;
            vk_rpb.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            vk_rpb.pNext = nullptr;
            vk_rpb.renderPass = rp.get_vk_render_pass();
            vk_rpb.framebuffer = fb.get_vk_framebuffer();
            vk_rpb.renderArea = area;
            vk_rpb.clearValueCount = cv.size();
            std::vector<VkClearValue> vk_cv;
            vk_cv.reserve(cv.size());
            for (const auto &it : cv)
              vk_cv.push_back(it);
            vk_rpb.pClearValues = vk_cv.data();
            dev._fn_vkCmdBeginRenderPass(cmd_buff._get_vk_command_buffer(), &vk_rpb, sp_contents);
          }

          /// \brief Transition to the next subpass of a render pass
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdNextSubpass.html">vulkan khr doc</a>
          void next_subpass(VkSubpassContents sp_contents)
          {
            dev._fn_vkCmdNextSubpass(cmd_buff._get_vk_command_buffer(), sp_contents);
          }

          /// \brief End the current render pass
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdEndRenderPass.html">vulkan khr doc</a>
          void end_render_pass()
          {
            dev._fn_vkCmdEndRenderPass(cmd_buff._get_vk_command_buffer());
          }

          /// \brief Execute a secondary command buffer from a primary command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdExecuteCommands.html">vulkan khr doc</a>
          void execute_commands(const std::vector<const command_buffer *> &cmd_buff_vct)
          {
            std::vector<VkCommandBuffer> vk_cmd_buff;
            vk_cmd_buff.reserve(cmd_buff_vct.size());
            for (const command_buffer *it : cmd_buff_vct)
              vk_cmd_buff.push_back(it->_get_vk_command_buffer());
            dev._fn_vkCmdExecuteCommands(cmd_buff._get_vk_command_buffer(), vk_cmd_buff.size(), vk_cmd_buff.data());
          }

          /// \brief Execute a secondary command buffer from a primary command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdExecuteCommands.html">vulkan khr doc</a>
          void execute_commands(const command_buffer &sec_cmd_buff)
          {
            VkCommandBuffer vk_sec_cmd_buff = sec_cmd_buff._get_vk_command_buffer();
            dev._fn_vkCmdExecuteCommands(cmd_buff._get_vk_command_buffer(), 1, &vk_sec_cmd_buff);
          }

          /// \brief Bind vertex buffers to a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBindVertexBuffers.html">vulkan khr doc</a>
          void bind_vertex_buffers(uint32_t first_binding, const std::vector<const buffer *> &buffers, const std::vector<size_t> &offsets)
          {
            std::vector<VkBuffer> vk_buffers;
            vk_buffers.reserve(buffers.size());
            for (const buffer *it : buffers)
              vk_buffers.push_back(it->_get_vk_buffer());
            dev._fn_vkCmdBindVertexBuffers(cmd_buff._get_vk_command_buffer(), first_binding, vk_buffers.size(), vk_buffers.data(), offsets.data());
          }

          /// \brief Bind vertex buffers to a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBindVertexBuffers.html">vulkan khr doc</a>
          void bind_vertex_buffers(uint32_t first_binding, const std::vector<const buffer *> &buffers)
          {
            std::vector<VkBuffer> vk_buffers;
            vk_buffers.reserve(buffers.size());
            for (const buffer *it : buffers)
              vk_buffers.push_back(it->_get_vk_buffer());

            std::vector<VkDeviceSize> offsets;
            offsets.resize(buffers.size(), 0);

            dev._fn_vkCmdBindVertexBuffers(cmd_buff._get_vk_command_buffer(), first_binding, vk_buffers.size(), vk_buffers.data(), offsets.data());
          }

          /// \brief Bind a vertex buffer to a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBindVertexBuffers.html">vulkan khr doc</a>
          void bind_vertex_buffer(const buffer &buf, uint32_t binding, size_t offset = 0)
          {
            VkBuffer vk_buffer = buf._get_vk_buffer();
            dev._fn_vkCmdBindVertexBuffers(cmd_buff._get_vk_command_buffer(), binding, 1, &vk_buffer, &offset);
          }

          /// \brief Bind an index buffer to a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBindIndexBuffer.html">vulkan khr doc</a>
          void bind_index_buffer(const buffer &buf, VkIndexType index_type, size_t offset = 0)
          {
            dev._fn_vkCmdBindIndexBuffer(cmd_buff._get_vk_command_buffer(), buf._get_vk_buffer(), offset, index_type);
          }

          /// \brief Fill a region of a buffer with a fixed value
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdFillBuffer.html">vulkan khr doc</a>
          void fill_buffer(const buffer &buf, size_t offset, size_t size, uint32_t value)
          {
            dev._fn_vkCmdFillBuffer(cmd_buff._get_vk_command_buffer(), buf._get_vk_buffer(), offset, size, value);
          }

          /// \brief Update a buffer's contents from host memory
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdUpdateBuffer.html">vulkan khr doc</a>
          void update_buffer(const buffer &buf, size_t offset, size_t size, const uint32_t *data) /* NOTE: the vulkan doc states that data should be a 'const void *' */
          {
            dev._fn_vkCmdUpdateBuffer(cmd_buff._get_vk_command_buffer(), buf._get_vk_buffer(), offset, size, data);
          }

          /// \brief Copy data between buffers
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdCopyBuffer.html">vulkan khr doc</a>
          void copy_buffer(const buffer &src, const buffer &dst)
          {
            VkBufferCopy vk_bc
            {
              0,
              0,
              (src.size() < dst.size() ? src.size() : dst.size())
            };
            dev._fn_vkCmdCopyBuffer(cmd_buff._get_vk_command_buffer(), src._get_vk_buffer(), dst._get_vk_buffer(), 1, &vk_bc);
          }

          /// \brief Copy data between buffer regions
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdCopyBuffer.html">vulkan khr doc</a>
          void copy_buffer(const buffer &src, const buffer &dst, const std::vector<VkBufferCopy> &regions)
          {
            dev._fn_vkCmdCopyBuffer(cmd_buff._get_vk_command_buffer(), src._get_vk_buffer(), dst._get_vk_buffer(), regions.size(), regions.data());
          }

          /// \brief Insert a set of execution and memory barriers
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdPipelineBarrier.html">vulkan khr doc</a>
          void pipeline_barrier(VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
                                VkDependencyFlags dep_flags, const std::vector<buffer_memory_barrier> &bmb)
          {
            dev._fn_vkCmdPipelineBarrier(cmd_buff._get_vk_command_buffer(), src_stage_mask, dst_stage_mask, dep_flags,
                                         0, nullptr,
                                         bmb.size(), (VkBufferMemoryBarrier*)bmb.data(),
                                         0, nullptr);
          }

          /// \brief Insert a set of execution and memory barriers
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdPipelineBarrier.html">vulkan khr doc</a>
          void pipeline_barrier(VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
                                VkDependencyFlags dep_flags, const std::vector<image_memory_barrier> &imb)
          {
            dev._fn_vkCmdPipelineBarrier(cmd_buff._get_vk_command_buffer(), src_stage_mask, dst_stage_mask, dep_flags,
                                         0, nullptr,
                                         0, nullptr,
                                         imb.size(), (VkImageMemoryBarrier*)imb.data());
          }

          /// \brief Insert a set of execution and memory barriers
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdPipelineBarrier.html">vulkan khr doc</a>
          void pipeline_barrier(VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
                                VkDependencyFlags dep_flags, const std::vector<memory_barrier> &mb)
          {
            dev._fn_vkCmdPipelineBarrier(cmd_buff._get_vk_command_buffer(), src_stage_mask, dst_stage_mask, dep_flags,
                                         mb.size(), (VkMemoryBarrier*)mb.data(),
                                         0, nullptr,
                                         0, nullptr);
          }

          /// \brief Insert a set of execution and memory barriers
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdPipelineBarrier.html">vulkan khr doc</a>
          void pipeline_barrier(VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
                                VkDependencyFlags dep_flags,
                                const std::vector<memory_barrier> &mb,
                                const std::vector<buffer_memory_barrier> &bmb,
                                const std::vector<image_memory_barrier> &imb)
          {
            dev._fn_vkCmdPipelineBarrier(cmd_buff._get_vk_command_buffer(), src_stage_mask, dst_stage_mask, dep_flags,
                                         mb.size(), (VkMemoryBarrier*)mb.data(),
                                         bmb.size(), (VkBufferMemoryBarrier*)bmb.data(),
                                         imb.size(), (VkImageMemoryBarrier*)imb.data());
          }

          /// \brief Copy data from a buffer into an image
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdCopyBufferToImage.html">vulkan khr doc</a>
          void copy_buffer_to_image(const buffer &src, const image &dst, VkImageLayout dst_layout, const std::vector<buffer_image_copy> &bic_vct)
          {
            dev._fn_vkCmdCopyBufferToImage(cmd_buff._get_vk_command_buffer(), src._get_vk_buffer(), dst.get_vk_image(), dst_layout, bic_vct.size(), (const VkBufferImageCopy*)bic_vct.data());
          }

          /// \brief Copy data from a buffer into an image
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdCopyBufferToImage.html">vulkan khr doc</a>
          void copy_buffer_to_image(const buffer &src, const image &dst, VkImageLayout dst_layout, const buffer_image_copy &bic)
          {
            dev._fn_vkCmdCopyBufferToImage(cmd_buff._get_vk_command_buffer(), src._get_vk_buffer(), dst.get_vk_image(), dst_layout, 1, (const VkBufferImageCopy*)&bic);
          }

          /// \brief Binds descriptor sets to a command buffer
          /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCmdBindDescriptorSets.html">vulkan khr doc</a>
          void bind_descriptor_set(VkPipelineBindPoint point, const pipeline_layout &pl, uint32_t first_set, const std::vector<descriptor_set *> &ds_vct)
          {
            std::vector<VkDescriptorSet> vk_ds_vct;
            vk_ds_vct.reserve(ds_vct.size());
            for (auto it : ds_vct)
              vk_ds_vct.push_back(it->_get_vk_descritpor_set());
            dev._fn_vkCmdBindDescriptorSets(cmd_buff._get_vk_command_buffer(), point, pl._get_vk_pipeline_layout(), first_set, vk_ds_vct.size(), vk_ds_vct.data(), 0, nullptr);
          }

//           /// \brief
//           /// <a href="https://www.khronos.org/registry/vulkan/specs/1.0/man/html/.html">vulkan khr doc</a>
//           void set_()
//           {
//             dev._fn_vkCmd(cmd_buff._get_vk_command_buffer());
//           }
#define TODO_FNC(x)
          // TODO
          TODO_FNC(vkCmdWaitEvents);        // NOTE: This could be done now
          TODO_FNC(vkCmdCopyImageToBuffer); // NOTE: This could be done now
          TODO_FNC(vkCmdClearAttachments);  // NOTE: may be done now (missing: wrapper for VkClearRect)
          TODO_FNC(vkCmdResolveImage);      // NOTE: may be done now (missing: wrapper for VkImageResolve)

          TODO_FNC(vkCmdBeginQuery);
          TODO_FNC(vkCmdEndQuery);
          TODO_FNC(vkCmdResetQueryPool);
          TODO_FNC(vkCmdWriteTimestamp);
          TODO_FNC(vkCmdCopyQueryPoolResults);
#undef TODO_FNC

        private:
          device &dev;
          command_buffer &cmd_buff;

          // state information
          const pipeline *last_bound_pipeline = nullptr;
      };


      // ////////////////////////////////////////////////////
      // // Implementation of the begin_recording thing // //
      // ////////////////////////////////////////////////////

      inline command_buffer_recorder command_buffer::begin_recording(VkCommandBufferUsageFlagBits flags)
      {
        VkCommandBufferBeginInfo vk_cbbi
        {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
          flags, nullptr
        };

        check::on_vulkan_error::n_assert_success(dev._fn_vkBeginCommandBuffer(cmd_buf, &vk_cbbi));

        return command_buffer_recorder(dev, *this);
      }

      inline command_buffer_recorder command_buffer::begin_recording(bool occlusion_query_enable, VkQueryControlFlags query_flags, VkQueryPipelineStatisticFlags stat_flags,
                                                              VkCommandBufferUsageFlagBits flags)
      {
        VkCommandBufferInheritanceInfo vk_cbii
        {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO, nullptr,
          nullptr, 0, /* render pass */
          nullptr,    /* framebuffer */
          occlusion_query_enable, query_flags, stat_flags
        };
        VkCommandBufferBeginInfo vk_cbbi
        {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
          flags, &vk_cbii
        };

        check::on_vulkan_error::n_assert_success(dev._fn_vkBeginCommandBuffer(cmd_buf, &vk_cbbi));

        return command_buffer_recorder(dev, *this);
      }

      inline command_buffer_recorder command_buffer::begin_recording(const framebuffer &fb,
                                              bool occlusion_query_enable, VkQueryControlFlags query_flags, VkQueryPipelineStatisticFlags stat_flags,
                                              VkCommandBufferUsageFlagBits flags)
      {
        VkCommandBufferInheritanceInfo vk_cbii
        {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO, nullptr,
          nullptr, 0, /* render pass */
          fb.get_vk_framebuffer(),    /* framebuffer */
          occlusion_query_enable, query_flags, stat_flags
        };
        VkCommandBufferBeginInfo vk_cbbi
        {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
          flags, &vk_cbii
        };

        check::on_vulkan_error::n_assert_success(dev._fn_vkBeginCommandBuffer(cmd_buf, &vk_cbbi));

        return command_buffer_recorder(dev, *this);
      }

      inline command_buffer_recorder command_buffer::begin_recording(const render_pass &rp, uint32_t subpass,
                                              bool occlusion_query_enable, VkQueryControlFlags query_flags, VkQueryPipelineStatisticFlags stat_flags,
                                              VkCommandBufferUsageFlagBits flags)
      {
        VkCommandBufferInheritanceInfo vk_cbii
        {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO, nullptr,
          rp.get_vk_render_pass(), subpass, /* render pass */
          nullptr,    /* framebuffer */
          occlusion_query_enable, query_flags, stat_flags
        };
        VkCommandBufferBeginInfo vk_cbbi
        {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
          flags, &vk_cbii
        };

        check::on_vulkan_error::n_assert_success(dev._fn_vkBeginCommandBuffer(cmd_buf, &vk_cbbi));

        return command_buffer_recorder(dev, *this);
      }

      inline command_buffer_recorder command_buffer::begin_recording(const framebuffer &fb, const render_pass &rp, uint32_t subpass,
                                              bool occlusion_query_enable, VkQueryControlFlags query_flags, VkQueryPipelineStatisticFlags stat_flags,
                                              VkCommandBufferUsageFlagBits flags)
      {
        VkCommandBufferInheritanceInfo vk_cbii
        {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO, nullptr,
          rp.get_vk_render_pass(), subpass, /* render pass */
          fb.get_vk_framebuffer(),    /* framebuffer */
          occlusion_query_enable, query_flags, stat_flags
        };
        VkCommandBufferBeginInfo vk_cbbi
        {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
          flags, &vk_cbii
        };

        check::on_vulkan_error::n_assert_success(dev._fn_vkBeginCommandBuffer(cmd_buf, &vk_cbbi));

        return command_buffer_recorder(dev, *this);
      }
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_1232927689274571835_1298719218_COMMAND_BUFFER_RECORDER_HPP__

