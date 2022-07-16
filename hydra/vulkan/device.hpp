//
// file : device.hpp
// in : file:///home/tim/projects/hydra/hydra/init/device.hpp
//
// created by : Timothée Feuillet
// date: Wed Apr 27 2016 16:54:15 GMT+0200 (CEST)
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

#ifndef __N_95735850311973920_182594734_HYDRA_VULKAN_DEVICE_HPP__
#define __N_95735850311973920_182594734_HYDRA_VULKAN_DEVICE_HPP__

#include <string.h>
#include <string>
#include <map>

#include <vulkan/vulkan.h>

#include "physical_device.hpp"
#include "instance.hpp"
#include "../hydra_debug.hpp"
#include "../hydra_types.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      class command_pool;

      /// \brief Wraps a vulkan logical device
      class device
      {
        public: // advanced
          /// \brief You shouldn't have to call this directly, but instead you should
          /// ask the hydra_device_creator class a new device
          device(instance& _instance, VkDevice _vk_device, const physical_device &_phys_dev, std::map<temp_queue_familly_id_t, std::pair<size_t, size_t>> _id_to_familly_queue)
          : vk_instance(_instance), vk_device(_vk_device), phys_dev(_phys_dev), id_to_familly_queue(_id_to_familly_queue)
          {
            _load_functions();
          }

        public:
          /// \brief Move constructor
          device(device&& o)
            : vk_instance(o.vk_instance),
              vk_device(o.vk_device),
              phys_dev(std::move(o.phys_dev)),
              id_to_familly_queue(std::move(o.id_to_familly_queue))
          {
            memcpy(&_st_offset, &o._st_offset, (uint8_t*)&_end_offset - (uint8_t*)&_st_offset);
            o.vk_device = nullptr;
          }

          ~device()
          {
            if (vk_device)
              vkDestroyDevice(vk_device, nullptr);
            vk_device = nullptr;
          }

          /// \brief Return the physical device from which the device has been created
          const physical_device &get_physical_device() const
          {
            return phys_dev;
          }

          /// \brief Wait for the device to become idle
          void wait_idle() const
          {
            _fn_vkDeviceWaitIdle(vk_device);
          }

        public: // advanced
          /// \brief Return the address of a procedure. No check is performed.
          inline PFN_vkVoidFunction _get_proc_addr_unsafe(const std::string &name)
          {
            return vkGetDeviceProcAddr(vk_device, name.c_str());
          }
          /// \brief Return the address of a procedure. No check is performed.
          inline PFN_vkVoidFunction _get_proc_addr(const std::string &name)
          {
            PFN_vkVoidFunction vulkan_fnc_pointer = vkGetDeviceProcAddr(vk_device, name.c_str());
            check::on_vulkan_error::n_assert(vulkan_fnc_pointer != nullptr, "vkGetDeviceProcAddr failed for {0}", name);
            return vulkan_fnc_pointer;
          }

          /// \brief Convert a temp_queue_familly_id_t into a queue_desc
          std::pair<size_t, size_t> _get_queue_info(temp_queue_familly_id_t temp_id)
          {
            auto it = id_to_familly_queue.find(temp_id);

            check::on_vulkan_error::n_assert(it != id_to_familly_queue.end(), "Unable to find the requested temp_queue_familly_id_t");

            return it->second;
          }

          /// \brief Return the vulkan device
          VkDevice _get_vk_device() const
          {
            return vk_device;
          }

          /// \brief return the vulkan instance that created that device
          instance& _get_instance()
          {
            return vk_instance;
          }
          /// \brief return the vulkan instance that created that device
          const instance& _get_instance() const
          {
            return vk_instance;
          }

        private:
          instance& vk_instance;
          VkDevice vk_device;
          physical_device phys_dev;
          std::map<temp_queue_familly_id_t, std::pair<size_t, size_t>> id_to_familly_queue;

          // /////////////////////////////////////////////////////////////////////
          // /// DIRECT VULKAN WRAPPER /// //
          // ////////////////////////////////
          //
          // NOTE: All vulkan function are prefixed with an underscore:
          //  wrapper for vkGetDeviceQueue is _vkGetDeviceQueue
          // Only vulkan functions that refers to devices are wrapped, and for
          // those functions you should omit the device parameter (the first one)
          //
          // All function pointers are prefixed with _fn_:
          //  function pointer for vkGetDeviceQueue is _fn_vkGetDeviceQueue
          //
          // If you activated the "ultra-debug" mode on hydra, this will quite
          // flood your logs as for each function there's a check to see if the
          // returned value is not a null pointer (but it only occurs one time
          // at initialization)
          //

        public: // andvanced
          /// \brief Load the vulkan funtions for the specific device
          void _load_functions()
          {
#define     HYDRA_LOAD_FNC(fnc)   _fn_##fnc = (PFN_##fnc)_get_proc_addr(#fnc)
            // device
            HYDRA_LOAD_FNC(vkGetDeviceQueue);
            HYDRA_LOAD_FNC(vkDeviceWaitIdle);
            HYDRA_LOAD_FNC(vkAllocateMemory);
            HYDRA_LOAD_FNC(vkFreeMemory);
            HYDRA_LOAD_FNC(vkMapMemory);
            HYDRA_LOAD_FNC(vkUnmapMemory);
            HYDRA_LOAD_FNC(vkFlushMappedMemoryRanges);
            HYDRA_LOAD_FNC(vkInvalidateMappedMemoryRanges);
            HYDRA_LOAD_FNC(vkGetDeviceMemoryCommitment);
            HYDRA_LOAD_FNC(vkBindBufferMemory);
            HYDRA_LOAD_FNC(vkBindImageMemory);
            HYDRA_LOAD_FNC(vkGetBufferMemoryRequirements);
            HYDRA_LOAD_FNC(vkGetImageMemoryRequirements);
            HYDRA_LOAD_FNC(vkGetImageSparseMemoryRequirements);
//             HYDRA_LOAD_FNC(vkGetPhysicalDeviceSparseImageFormatProperties);
            HYDRA_LOAD_FNC(vkCreateFence);
            HYDRA_LOAD_FNC(vkDestroyFence);
            HYDRA_LOAD_FNC(vkResetFences);
            HYDRA_LOAD_FNC(vkGetFenceStatus);
            HYDRA_LOAD_FNC(vkWaitForFences);
            HYDRA_LOAD_FNC(vkCreateSemaphore);
            HYDRA_LOAD_FNC(vkDestroySemaphore);
            HYDRA_LOAD_FNC(vkCreateEvent);
            HYDRA_LOAD_FNC(vkDestroyEvent);
            HYDRA_LOAD_FNC(vkGetEventStatus);
            HYDRA_LOAD_FNC(vkSetEvent);
            HYDRA_LOAD_FNC(vkResetEvent);
            HYDRA_LOAD_FNC(vkCreateQueryPool);
            HYDRA_LOAD_FNC(vkDestroyQueryPool);
            HYDRA_LOAD_FNC(vkGetQueryPoolResults);
            HYDRA_LOAD_FNC(vkCreateBuffer);
            HYDRA_LOAD_FNC(vkDestroyBuffer);
            HYDRA_LOAD_FNC(vkCreateBufferView);   // TODO (buffer views)
            HYDRA_LOAD_FNC(vkDestroyBufferView);
            HYDRA_LOAD_FNC(vkCreateImage);
            HYDRA_LOAD_FNC(vkDestroyImage);
            HYDRA_LOAD_FNC(vkGetImageSubresourceLayout);
            HYDRA_LOAD_FNC(vkCreateImageView);
            HYDRA_LOAD_FNC(vkDestroyImageView);
            HYDRA_LOAD_FNC(vkCreateShaderModule);
            HYDRA_LOAD_FNC(vkDestroyShaderModule);
            HYDRA_LOAD_FNC(vkCreatePipelineCache);
            HYDRA_LOAD_FNC(vkDestroyPipelineCache);
            HYDRA_LOAD_FNC(vkGetPipelineCacheData);
            HYDRA_LOAD_FNC(vkMergePipelineCaches);
            HYDRA_LOAD_FNC(vkCreateGraphicsPipelines);
            HYDRA_LOAD_FNC(vkCreateComputePipelines);
            HYDRA_LOAD_FNC(vkDestroyPipeline);
            HYDRA_LOAD_FNC(vkCreatePipelineLayout);
            HYDRA_LOAD_FNC(vkDestroyPipelineLayout);
            HYDRA_LOAD_FNC(vkCreateSampler);
            HYDRA_LOAD_FNC(vkDestroySampler);
            HYDRA_LOAD_FNC(vkCreateDescriptorSetLayout);
            HYDRA_LOAD_FNC(vkDestroyDescriptorSetLayout);
            HYDRA_LOAD_FNC(vkCreateDescriptorPool);
            HYDRA_LOAD_FNC(vkDestroyDescriptorPool);
            HYDRA_LOAD_FNC(vkResetDescriptorPool);
            HYDRA_LOAD_FNC(vkAllocateDescriptorSets);
            HYDRA_LOAD_FNC(vkFreeDescriptorSets);
            HYDRA_LOAD_FNC(vkUpdateDescriptorSets);
            HYDRA_LOAD_FNC(vkCreateFramebuffer);
            HYDRA_LOAD_FNC(vkDestroyFramebuffer);
            HYDRA_LOAD_FNC(vkCreateRenderPass);
            HYDRA_LOAD_FNC(vkDestroyRenderPass);
            HYDRA_LOAD_FNC(vkGetRenderAreaGranularity);
            HYDRA_LOAD_FNC(vkCreateCommandPool);
            HYDRA_LOAD_FNC(vkDestroyCommandPool);
            HYDRA_LOAD_FNC(vkResetCommandPool);
            HYDRA_LOAD_FNC(vkAllocateCommandBuffers);
            HYDRA_LOAD_FNC(vkFreeCommandBuffers);

            // queue
            HYDRA_LOAD_FNC(vkQueueSubmit);
            HYDRA_LOAD_FNC(vkQueueBindSparse);
            HYDRA_LOAD_FNC(vkQueueWaitIdle);

            // cmd buffer
            HYDRA_LOAD_FNC(vkBeginCommandBuffer);
            HYDRA_LOAD_FNC(vkEndCommandBuffer);
            HYDRA_LOAD_FNC(vkResetCommandBuffer);
            HYDRA_LOAD_FNC(vkCmdBindPipeline);
            HYDRA_LOAD_FNC(vkCmdSetViewport);
            HYDRA_LOAD_FNC(vkCmdSetScissor);
            HYDRA_LOAD_FNC(vkCmdSetLineWidth);
            HYDRA_LOAD_FNC(vkCmdSetDepthBias);
            HYDRA_LOAD_FNC(vkCmdSetBlendConstants);
            HYDRA_LOAD_FNC(vkCmdSetDepthBounds);
            HYDRA_LOAD_FNC(vkCmdSetStencilCompareMask);
            HYDRA_LOAD_FNC(vkCmdSetStencilWriteMask);
            HYDRA_LOAD_FNC(vkCmdSetStencilReference);
            HYDRA_LOAD_FNC(vkCmdBindDescriptorSets);
            HYDRA_LOAD_FNC(vkCmdBindIndexBuffer);
            HYDRA_LOAD_FNC(vkCmdBindVertexBuffers);
            HYDRA_LOAD_FNC(vkCmdDraw);
            HYDRA_LOAD_FNC(vkCmdDrawIndexed);
            HYDRA_LOAD_FNC(vkCmdDrawIndirect);
            HYDRA_LOAD_FNC(vkCmdDrawIndexedIndirect);
            HYDRA_LOAD_FNC(vkCmdDispatch);
            HYDRA_LOAD_FNC(vkCmdDispatchIndirect);
            HYDRA_LOAD_FNC(vkCmdCopyBuffer);
            HYDRA_LOAD_FNC(vkCmdCopyImage);
            HYDRA_LOAD_FNC(vkCmdBlitImage);
            HYDRA_LOAD_FNC(vkCmdCopyBufferToImage);
            HYDRA_LOAD_FNC(vkCmdCopyImageToBuffer);
            HYDRA_LOAD_FNC(vkCmdUpdateBuffer);
            HYDRA_LOAD_FNC(vkCmdFillBuffer);
            HYDRA_LOAD_FNC(vkCmdClearColorImage);
            HYDRA_LOAD_FNC(vkCmdClearDepthStencilImage);
            HYDRA_LOAD_FNC(vkCmdClearAttachments);
            HYDRA_LOAD_FNC(vkCmdResolveImage);
            HYDRA_LOAD_FNC(vkCmdSetEvent);
            HYDRA_LOAD_FNC(vkCmdResetEvent);
            HYDRA_LOAD_FNC(vkCmdWaitEvents);
            HYDRA_LOAD_FNC(vkCmdPipelineBarrier);
            HYDRA_LOAD_FNC(vkCmdBeginQuery);
            HYDRA_LOAD_FNC(vkCmdEndQuery);
            HYDRA_LOAD_FNC(vkCmdResetQueryPool);
            HYDRA_LOAD_FNC(vkCmdWriteTimestamp);
            HYDRA_LOAD_FNC(vkCmdCopyQueryPoolResults);
            HYDRA_LOAD_FNC(vkCmdPushConstants);
            HYDRA_LOAD_FNC(vkCmdBeginRenderPass);
            HYDRA_LOAD_FNC(vkCmdNextSubpass);
            HYDRA_LOAD_FNC(vkCmdEndRenderPass);
            HYDRA_LOAD_FNC(vkCmdExecuteCommands);
#undef      HYDRA_LOAD_FNC
          }

        private:
          PFN_vkVoidFunction _st_offset;
        public:// advanced (vulkan device call wrappers)
          // this is the lazy way to wrap functions, and the big problem with this
          // method will be that the compiler may generate quite a lot of functions...
#define   HYDRA_VK_DEV_FNC_WRAPPER(fnc)     template<typename... FncParams> inline auto _##fnc(FncParams... params) { return _fn_##fnc(vk_device, std::forward<FncParams>(params)...); }
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetDeviceQueue);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDeviceWaitIdle);
          HYDRA_VK_DEV_FNC_WRAPPER(vkAllocateMemory);
          HYDRA_VK_DEV_FNC_WRAPPER(vkFreeMemory);
          HYDRA_VK_DEV_FNC_WRAPPER(vkMapMemory);
          HYDRA_VK_DEV_FNC_WRAPPER(vkUnmapMemory);
          HYDRA_VK_DEV_FNC_WRAPPER(vkFlushMappedMemoryRanges);
          HYDRA_VK_DEV_FNC_WRAPPER(vkInvalidateMappedMemoryRanges);
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetDeviceMemoryCommitment);
          HYDRA_VK_DEV_FNC_WRAPPER(vkBindBufferMemory);
          HYDRA_VK_DEV_FNC_WRAPPER(vkBindImageMemory);
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetBufferMemoryRequirements);
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetImageMemoryRequirements);
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetImageSparseMemoryRequirements);
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetPhysicalDeviceSparseImageFormatProperties);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateFence);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyFence);
          HYDRA_VK_DEV_FNC_WRAPPER(vkResetFences);
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetFenceStatus);
          HYDRA_VK_DEV_FNC_WRAPPER(vkWaitForFences);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateSemaphore);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroySemaphore);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateEvent);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyEvent);
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetEventStatus);
          HYDRA_VK_DEV_FNC_WRAPPER(vkSetEvent);
          HYDRA_VK_DEV_FNC_WRAPPER(vkResetEvent);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateQueryPool);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyQueryPool);
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetQueryPoolResults);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateBuffer);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyBuffer);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateBufferView);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyBufferView);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateImage);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyImage);
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetImageSubresourceLayout);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateImageView);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyImageView);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateShaderModule);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyShaderModule);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreatePipelineCache);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyPipelineCache);
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetPipelineCacheData);
          HYDRA_VK_DEV_FNC_WRAPPER(vkMergePipelineCaches);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateGraphicsPipelines);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateComputePipelines);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyPipeline);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreatePipelineLayout);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyPipelineLayout);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateSampler);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroySampler);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateDescriptorSetLayout);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyDescriptorSetLayout);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateDescriptorPool);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyDescriptorPool);
          HYDRA_VK_DEV_FNC_WRAPPER(vkResetDescriptorPool);
          HYDRA_VK_DEV_FNC_WRAPPER(vkAllocateDescriptorSets);
          HYDRA_VK_DEV_FNC_WRAPPER(vkFreeDescriptorSets);
          HYDRA_VK_DEV_FNC_WRAPPER(vkUpdateDescriptorSets);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateFramebuffer);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyFramebuffer);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateRenderPass);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyRenderPass);
          HYDRA_VK_DEV_FNC_WRAPPER(vkGetRenderAreaGranularity);
          HYDRA_VK_DEV_FNC_WRAPPER(vkCreateCommandPool);
          HYDRA_VK_DEV_FNC_WRAPPER(vkDestroyCommandPool);
          HYDRA_VK_DEV_FNC_WRAPPER(vkResetCommandPool);
          HYDRA_VK_DEV_FNC_WRAPPER(vkAllocateCommandBuffers);
          HYDRA_VK_DEV_FNC_WRAPPER(vkFreeCommandBuffers);
#undef    HYDRA_VK_DEV_FNC_WRAPPER
        public:
#define   HYDRA_DECLARE_VK_FNC(fnc)   PFN_##fnc _fn_##fnc
          // device
          HYDRA_DECLARE_VK_FNC(vkGetDeviceQueue);
          HYDRA_DECLARE_VK_FNC(vkDeviceWaitIdle);
          HYDRA_DECLARE_VK_FNC(vkAllocateMemory);
          HYDRA_DECLARE_VK_FNC(vkFreeMemory);
          HYDRA_DECLARE_VK_FNC(vkMapMemory);
          HYDRA_DECLARE_VK_FNC(vkUnmapMemory);
          HYDRA_DECLARE_VK_FNC(vkFlushMappedMemoryRanges);
          HYDRA_DECLARE_VK_FNC(vkInvalidateMappedMemoryRanges);
          HYDRA_DECLARE_VK_FNC(vkGetDeviceMemoryCommitment);
          HYDRA_DECLARE_VK_FNC(vkBindBufferMemory);
          HYDRA_DECLARE_VK_FNC(vkBindImageMemory);
          HYDRA_DECLARE_VK_FNC(vkGetBufferMemoryRequirements);
          HYDRA_DECLARE_VK_FNC(vkGetImageMemoryRequirements);
          HYDRA_DECLARE_VK_FNC(vkGetImageSparseMemoryRequirements);
          HYDRA_DECLARE_VK_FNC(vkGetPhysicalDeviceSparseImageFormatProperties);
          HYDRA_DECLARE_VK_FNC(vkCreateFence);
          HYDRA_DECLARE_VK_FNC(vkDestroyFence);
          HYDRA_DECLARE_VK_FNC(vkResetFences);
          HYDRA_DECLARE_VK_FNC(vkGetFenceStatus);
          HYDRA_DECLARE_VK_FNC(vkWaitForFences);
          HYDRA_DECLARE_VK_FNC(vkCreateSemaphore);
          HYDRA_DECLARE_VK_FNC(vkDestroySemaphore);
          HYDRA_DECLARE_VK_FNC(vkCreateEvent);
          HYDRA_DECLARE_VK_FNC(vkDestroyEvent);
          HYDRA_DECLARE_VK_FNC(vkGetEventStatus);
          HYDRA_DECLARE_VK_FNC(vkSetEvent);
          HYDRA_DECLARE_VK_FNC(vkResetEvent);
          HYDRA_DECLARE_VK_FNC(vkCreateQueryPool);
          HYDRA_DECLARE_VK_FNC(vkDestroyQueryPool);
          HYDRA_DECLARE_VK_FNC(vkGetQueryPoolResults);
          HYDRA_DECLARE_VK_FNC(vkCreateBuffer);
          HYDRA_DECLARE_VK_FNC(vkDestroyBuffer);
          HYDRA_DECLARE_VK_FNC(vkCreateBufferView);
          HYDRA_DECLARE_VK_FNC(vkDestroyBufferView);
          HYDRA_DECLARE_VK_FNC(vkCreateImage);
          HYDRA_DECLARE_VK_FNC(vkDestroyImage);
          HYDRA_DECLARE_VK_FNC(vkGetImageSubresourceLayout);
          HYDRA_DECLARE_VK_FNC(vkCreateImageView);
          HYDRA_DECLARE_VK_FNC(vkDestroyImageView);
          HYDRA_DECLARE_VK_FNC(vkCreateShaderModule);
          HYDRA_DECLARE_VK_FNC(vkDestroyShaderModule);
          HYDRA_DECLARE_VK_FNC(vkCreatePipelineCache);
          HYDRA_DECLARE_VK_FNC(vkDestroyPipelineCache);
          HYDRA_DECLARE_VK_FNC(vkGetPipelineCacheData);
          HYDRA_DECLARE_VK_FNC(vkMergePipelineCaches);
          HYDRA_DECLARE_VK_FNC(vkCreateGraphicsPipelines);
          HYDRA_DECLARE_VK_FNC(vkCreateComputePipelines);
          HYDRA_DECLARE_VK_FNC(vkDestroyPipeline);
          HYDRA_DECLARE_VK_FNC(vkCreatePipelineLayout);
          HYDRA_DECLARE_VK_FNC(vkDestroyPipelineLayout);
          HYDRA_DECLARE_VK_FNC(vkCreateSampler);
          HYDRA_DECLARE_VK_FNC(vkDestroySampler);
          HYDRA_DECLARE_VK_FNC(vkCreateDescriptorSetLayout);
          HYDRA_DECLARE_VK_FNC(vkDestroyDescriptorSetLayout);
          HYDRA_DECLARE_VK_FNC(vkCreateDescriptorPool);
          HYDRA_DECLARE_VK_FNC(vkDestroyDescriptorPool);
          HYDRA_DECLARE_VK_FNC(vkResetDescriptorPool);
          HYDRA_DECLARE_VK_FNC(vkAllocateDescriptorSets);
          HYDRA_DECLARE_VK_FNC(vkFreeDescriptorSets);
          HYDRA_DECLARE_VK_FNC(vkUpdateDescriptorSets);
          HYDRA_DECLARE_VK_FNC(vkCreateFramebuffer);
          HYDRA_DECLARE_VK_FNC(vkDestroyFramebuffer);
          HYDRA_DECLARE_VK_FNC(vkCreateRenderPass);
          HYDRA_DECLARE_VK_FNC(vkDestroyRenderPass);
          HYDRA_DECLARE_VK_FNC(vkGetRenderAreaGranularity);
          HYDRA_DECLARE_VK_FNC(vkCreateCommandPool);
          HYDRA_DECLARE_VK_FNC(vkDestroyCommandPool);
          HYDRA_DECLARE_VK_FNC(vkResetCommandPool);
          HYDRA_DECLARE_VK_FNC(vkAllocateCommandBuffers);
          HYDRA_DECLARE_VK_FNC(vkFreeCommandBuffers);

          // queue
          HYDRA_DECLARE_VK_FNC(vkQueueSubmit);
          HYDRA_DECLARE_VK_FNC(vkQueueBindSparse);
          HYDRA_DECLARE_VK_FNC(vkQueueWaitIdle);

          // cmd buffer
          HYDRA_DECLARE_VK_FNC(vkBeginCommandBuffer);
          HYDRA_DECLARE_VK_FNC(vkEndCommandBuffer);
          HYDRA_DECLARE_VK_FNC(vkResetCommandBuffer);
          HYDRA_DECLARE_VK_FNC(vkCmdBindPipeline);
          HYDRA_DECLARE_VK_FNC(vkCmdSetViewport);
          HYDRA_DECLARE_VK_FNC(vkCmdSetScissor);
          HYDRA_DECLARE_VK_FNC(vkCmdSetLineWidth);
          HYDRA_DECLARE_VK_FNC(vkCmdSetDepthBias);
          HYDRA_DECLARE_VK_FNC(vkCmdSetBlendConstants);
          HYDRA_DECLARE_VK_FNC(vkCmdSetDepthBounds);
          HYDRA_DECLARE_VK_FNC(vkCmdSetStencilCompareMask);
          HYDRA_DECLARE_VK_FNC(vkCmdSetStencilWriteMask);
          HYDRA_DECLARE_VK_FNC(vkCmdSetStencilReference);
          HYDRA_DECLARE_VK_FNC(vkCmdBindDescriptorSets);
          HYDRA_DECLARE_VK_FNC(vkCmdBindIndexBuffer);
          HYDRA_DECLARE_VK_FNC(vkCmdBindVertexBuffers);
          HYDRA_DECLARE_VK_FNC(vkCmdDraw);
          HYDRA_DECLARE_VK_FNC(vkCmdDrawIndexed);
          HYDRA_DECLARE_VK_FNC(vkCmdDrawIndirect);
          HYDRA_DECLARE_VK_FNC(vkCmdDrawIndexedIndirect);
          HYDRA_DECLARE_VK_FNC(vkCmdDispatch);
          HYDRA_DECLARE_VK_FNC(vkCmdDispatchIndirect);
          HYDRA_DECLARE_VK_FNC(vkCmdCopyBuffer);
          HYDRA_DECLARE_VK_FNC(vkCmdCopyImage);
          HYDRA_DECLARE_VK_FNC(vkCmdBlitImage);
          HYDRA_DECLARE_VK_FNC(vkCmdCopyBufferToImage);
          HYDRA_DECLARE_VK_FNC(vkCmdCopyImageToBuffer);
          HYDRA_DECLARE_VK_FNC(vkCmdUpdateBuffer);
          HYDRA_DECLARE_VK_FNC(vkCmdFillBuffer);
          HYDRA_DECLARE_VK_FNC(vkCmdClearColorImage);
          HYDRA_DECLARE_VK_FNC(vkCmdClearDepthStencilImage);
          HYDRA_DECLARE_VK_FNC(vkCmdClearAttachments);
          HYDRA_DECLARE_VK_FNC(vkCmdResolveImage);
          HYDRA_DECLARE_VK_FNC(vkCmdSetEvent);
          HYDRA_DECLARE_VK_FNC(vkCmdResetEvent);
          HYDRA_DECLARE_VK_FNC(vkCmdWaitEvents);
          HYDRA_DECLARE_VK_FNC(vkCmdPipelineBarrier);
          HYDRA_DECLARE_VK_FNC(vkCmdBeginQuery);
          HYDRA_DECLARE_VK_FNC(vkCmdEndQuery);
          HYDRA_DECLARE_VK_FNC(vkCmdResetQueryPool);
          HYDRA_DECLARE_VK_FNC(vkCmdWriteTimestamp);
          HYDRA_DECLARE_VK_FNC(vkCmdCopyQueryPoolResults);
          HYDRA_DECLARE_VK_FNC(vkCmdPushConstants);
          HYDRA_DECLARE_VK_FNC(vkCmdBeginRenderPass);
          HYDRA_DECLARE_VK_FNC(vkCmdNextSubpass);
          HYDRA_DECLARE_VK_FNC(vkCmdEndRenderPass);
          HYDRA_DECLARE_VK_FNC(vkCmdExecuteCommands);
#undef    HYDRA_DECLARE_VK_FNC
        private:
          PFN_vkVoidFunction _end_offset;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_95735850311973920_182594734_HYDRA_VULKAN_DEVICE_HPP__

