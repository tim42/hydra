//
// created by : Timothée Feuillet
// date: 2023-3-25
//
//
// Copyright (c) 2023 Timothée Feuillet
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

#include "instance.hpp"
#include "device.hpp"

namespace neam::hydra::vk
{
  namespace internal
  {
    static validation_state_t& get_validation_state_tls()
    {
      thread_local validation_state_t validation_state = validation_state_t::verbose;
      return validation_state;
    }
    void set_thread_validation_state(validation_state_t state)
    {
      get_validation_state_tls() = state;
    }

    validation_state_t get_thread_validation_state()
    {
      return get_validation_state_tls();
    }
  }

  VkBool32 instance::debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
  {
    (void)userData;

    const internal::validation_state_t validation_state = internal::get_thread_validation_state();

    // silent: do nothing.
    if (validation_state == internal::validation_state_t::silent)
      return VK_FALSE;

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

        _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT)
        _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT)
        _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT)
        _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT)
        _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT)

        _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT)

        _HYDRA_CASE(VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT)
      default:break;
    }
#undef _HYDRA_CASE

    if (validation_state == internal::validation_state_t::simple_notice)
    {
      neam::cr::out().debug("suppressed validation message for: {0} ({1})", object_type, std::string_view(strchr(msg, '['), strchr(msg, ']') - strchr(msg, '[')));
    }

    if (validation_state == internal::validation_state_t::summary || validation_state == internal::validation_state_t::verbose)
    {
      // finally print the message:
      neam::cr::out().log_fmt(s, std::source_location::current(),
                            "VULKAN VALIDATION LAYER MESSAGE: {0}:\n"
                            // "[{1} / {2} / {3} ]: {4}:\n"
                            "{5}",
                            object_type,
                            obj, location, code, layerPrefix,
                            msg);
#if N_HYDRA_DEBUG_VK_LOCATION
      neam::cr::out().log_fmt(s, std::source_location::current(), "vulkan call: {}",
                            device::_get_current_vk_call_str());
#endif
    }


    if (validation_state == internal::validation_state_t::verbose)
    {
      neam::cr::print_callstack();
    }
    return VK_FALSE;
  }
}

