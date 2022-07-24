//
// file : hydra_vulkan_instance.hpp
// in : file:///home/tim/projects/hydra/hydra/init/hydra_vulkan_instance.hpp
//
// created by : Timothée Feuillet
// date: Tue Apr 26 2016 11:53:06 GMT+0200 (CEST)
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

#ifndef __N_692459433198810776_77086004_HYDRA_VULKAN_INSTANCE_HPP__
#define __N_692459433198810776_77086004_HYDRA_VULKAN_INSTANCE_HPP__

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include <ntools/backtrace.hpp>

#include "../hydra_debug.hpp"
#include "physical_device.hpp"

namespace neam
{
  namespace hydra
  {
    class hydra_device_creator;
    class physical_device;
    namespace vk
    {
      /// \brief Wrap a vulkan instance
      class instance
      {
        public:
          /// \brief The only reason this constructor is public is to allow
          /// compatibility with other means of creating a vulkan instance.
          /// Instead of calling this directly, you should use the class
          /// hydra_instance_creator that handle the creation for you.
          instance(VkInstance _vulkan_instance, const std::string &_app_name = "")
            : vulkan_instance(_vulkan_instance), app_name(_app_name)
          {
            enumerate_devices();
          }

          /// \brief Move constructor.
          /// Allow move semantics
          instance(instance &&o)
          : vulkan_instance(o.vulkan_instance), app_name(o.app_name),
              gpu_list(std::move(o.gpu_list)),
              default_debug_callback(o.default_debug_callback)
          {
            o.vulkan_instance = nullptr;
            o.default_debug_callback = nullptr;
          }

          /// \brief Destructor
          ~instance()
          {
            if (vulkan_instance != nullptr)
            {
              remove_default_debug_callback();
              vkDestroyInstance(vulkan_instance, nullptr);
            }
          }

          /// \brief Return the number of GPU the system has
          size_t get_device_count() const
          {
            return gpu_list.size();
          }

          /// \brief Return a wrapper around a vulkan GPU
          const physical_device &get_device(size_t index) const
          {
            return gpu_list.at(index);
          }

          /// \brief Brief return a new device creator
          /// A device creator allows the user to request a logical device to vulkan
          /// in a easier fashion than the vulkan way
          /// \note the device creator should not live past the destruction of the
          ///       instance that created it
          ///
          /// implementation is in hydra_device_creator.hpp
          hydra_device_creator get_device_creator();

          /// \brief Setup/install a debug callback that will print validation layers messages to stdout/stderr
          /// \note For this to work, you'll need the VK_EXT_DEBUG_REPORT_EXTENSION_NAME extension
          /// \note the callback is automatically removed at destruction
          void install_default_debug_callback(VkDebugReportFlagsEXT report_flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
          {
            auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(vulkan_instance, "vkCreateDebugReportCallbackEXT");
            check::on_vulkan_error::n_check(vkCreateDebugReportCallbackEXT != nullptr, "vk::instance : extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME not activated");
            if (vkCreateDebugReportCallbackEXT == nullptr) return;

            VkDebugReportCallbackCreateInfoEXT rcci
            {
              VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
              nullptr,
              report_flags,
              (PFN_vkDebugReportCallbackEXT)(&instance::debug_callback),
              nullptr
            };
            check::on_vulkan_error::n_check_success(vkCreateDebugReportCallbackEXT(vulkan_instance, &rcci, nullptr, &default_debug_callback));
          }

          /// \brief Remove the default debug callback, if installed
          void remove_default_debug_callback()
          {
            if (default_debug_callback == nullptr)
              return;
            auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(vulkan_instance, "vkDestroyDebugReportCallbackEXT");
            check::on_vulkan_error::n_check(vkDestroyDebugReportCallbackEXT != nullptr, "vk::instance : extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME not activated");
            if (vkDestroyDebugReportCallbackEXT == nullptr) return;

            vkDestroyDebugReportCallbackEXT(vulkan_instance, default_debug_callback, nullptr);
          }

        public: // advanced
          /// \brief Return the vulkan instance object
          /// \note "Marked" as advanced ('cause that's not meant to be used by the end user)
          VkInstance _get_vk_instance() const
          {
            return vulkan_instance;
          }

        private:
          void enumerate_devices()
          {
            uint32_t gpu_count = 0;
            check::on_vulkan_error::n_assert_success(vkEnumeratePhysicalDevices(vulkan_instance, &gpu_count, nullptr));
            check::on_vulkan_error::n_assert(gpu_count > 0, "no compatible GPU found");

            std::vector<VkPhysicalDevice> vk_gpu_list;

            vk_gpu_list.resize(gpu_count);
            check::on_vulkan_error::n_assert_success(vkEnumeratePhysicalDevices(vulkan_instance, &gpu_count, vk_gpu_list.data()));

            for (const VkPhysicalDevice &it : vk_gpu_list)
              gpu_list.emplace_back(physical_device(it));
          }

          static VkBool32 debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
          {
            (void)userData;

            neam::cr::logger::severity s;
            switch (flags)
            {
              case VK_DEBUG_REPORT_INFORMATION_BIT_EXT: s = neam::cr::logger::severity::message;
                break;
              case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
              case VK_DEBUG_REPORT_WARNING_BIT_EXT: s = neam::cr::logger::severity::warning;
                break;
              case VK_DEBUG_REPORT_ERROR_BIT_EXT: s = neam::cr::logger::severity::error;
                break;
              case VK_DEBUG_REPORT_DEBUG_BIT_EXT: s = neam::cr::logger::severity::debug;
                break;
              default: s = neam::cr::logger::severity::message;
                break;
            }
            const char* object_type = nullptr;
#define _HYDRA_CASE(cs)     case cs: object_type = #cs; break;
            switch (objType)
            {
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT)
//                 _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_RANGE_SIZE_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT)
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT)

//               case VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT:
//                 _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_END_RANGE_EXT)

//               case VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT:
                _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT)
              default:break;
            }
#undef _HYDRA_CASE

            // finally print the message:
            neam::cr::out().log_fmt(s, std::source_location::current(),
                                  "VULKAN VALIDATION LAYER MESSAGE: {0}:\n"
                                  "[{1} / {2} / {3} ]: {4}:\n"
                                  "{5}",
                                  object_type,
                                  obj, location, code, layerPrefix,
                                  msg);

            neam::cr::print_callstack();
            return VK_FALSE;
          }

        private:
          VkInstance vulkan_instance;
          std::string app_name;
          std::vector<physical_device> gpu_list;

          VkDebugReportCallbackEXT default_debug_callback = nullptr;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_692459433198810776_77086004_HYDRA_VULKAN_INSTANCE_HPP__
