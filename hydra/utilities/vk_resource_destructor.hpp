//
// file : vk_resource_destructor.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/vk_resource_destructor.hpp
//
// created by : Timothée Feuillet
// date: Sun Nov 06 2016 16:08:01 GMT-0500 (EST)
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

#ifndef __N_154279271610614026_190128118_VK_RESOURCE_DESTRUCTOR_HPP__
#define __N_154279271610614026_190128118_VK_RESOURCE_DESTRUCTOR_HPP__

#include "../vulkan/fence.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief Destruct a list of resource when a fence becomes signaled
    /// It can also delete the fence
    class vk_resource_destructor
    {
      private:
        struct wrapper
        {
          wrapper(const vk::fence *_fence) : fence(_fence) {}
          virtual ~wrapper() {}
          const vk::fence *fence;
        };
        template<typename... ResourceTypes>
        struct spec_wrapper : public wrapper
        {
          spec_wrapper(const vk::fence *_fence, ResourceTypes &&... res)
            : wrapper {_fence}, resources(std::move(res)...)
          {}

          std::tuple<ResourceTypes...> resources;
        };

      public:
        vk_resource_destructor() = default;
        vk_resource_destructor(const vk_resource_destructor &) = delete;
        vk_resource_destructor(vk_resource_destructor &&) = default;
        vk_resource_destructor &operator = (const vk_resource_destructor &) = delete;
        vk_resource_destructor &operator = (vk_resource_destructor &&) = default;
        ~vk_resource_destructor()
        {
          for (auto it = res_list.begin(); it != res_list.end(); ++it)
            delete *it;
        }

        /// \brief Postpone the destruction of n elements when the fence becomes signaled
        /// \note it also destroys the fence
        template<typename... ResourceTypes>
        void postpone_destruction(vk::fence &&fence, ResourceTypes &&... resources)
        {
          spec_wrapper<vk::fence, ResourceTypes...> *ptr = new spec_wrapper<vk::fence, ResourceTypes...>{nullptr, std::move(fence), std::move(resources)...};
          ptr->fence = &std::get<0>(ptr->resources);
          res_list.push_back(ptr);
        }

        /// \brief Postpone the destruction of n elements when the semaphore becomes signaled
        /// \note does not destroy the fence: it must be kept alive until resources are destructed
        template<typename... ResourceTypes>
        void postpone_destruction(const vk::fence &fence, ResourceTypes &&... resources)
        {
          res_list.push_back(new spec_wrapper<ResourceTypes...>{&fence, std::move(resources)...});
        }

        /// \brief Perform the check
        void update()
        {
          size_t cnt = 0;
          for (auto it = res_list.begin(); it != res_list.end(); ++it)
          {
            if ((*it)->fence->is_signaled())
            {
              delete *it;
              it = res_list.erase(it);
              ++cnt;
            }
          }
        }

      private:
        std::list<wrapper *> res_list;
    };
  } // namespace hydra
} // namespace neam


#endif // __N_154279271610614026_190128118_VK_RESOURCE_DESTRUCTOR_HPP__
