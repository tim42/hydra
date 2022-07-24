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
#include "memory_allocator.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief Destruct a list of resource when a fence becomes signaled
    /// It can also delete the fence
    /// FIXME: optimize.
    class vk_resource_destructor
    {
      private:
        struct wrapper
        {
          wrapper(const vk::fence *_fence, size_t _queue_familly) : fence(_fence), queue_familly(_queue_familly) {}
          virtual ~wrapper() {}

          const vk::fence *fence;
          size_t queue_familly;
          std::list<wrapper *> sublist;
        };
        template<typename... ResourceTypes>
        struct spec_wrapper : public wrapper
        {
          spec_wrapper(const vk::fence *_fence, size_t _queue_familly, ResourceTypes &&... res)
            : wrapper {_fence, _queue_familly}, resources(std::move(res)...)
          {}

          std::tuple<ResourceTypes...> resources;
        };
        struct frame_allocator_wrapper : public wrapper
        {
          frame_allocator_wrapper(size_t _queue_familly, memory_allocator& _allocator)
            : wrapper {nullptr, _queue_familly}, allocator(_allocator)
          {}

          virtual ~frame_allocator_wrapper()
          {
            allocator.flush_empty_allocations();
          }

          memory_allocator& allocator;
        };

      public:
        vk_resource_destructor() = default;
        vk_resource_destructor(const vk_resource_destructor &) = delete;
//         vk_resource_destructor(vk_resource_destructor &&) = default;
        vk_resource_destructor &operator = (const vk_resource_destructor &) = delete;
//         vk_resource_destructor &operator = (vk_resource_destructor &&) = default;
        ~vk_resource_destructor()
        {
          _force_full_cleanup();
        }

        /// \brief Postpone the frame-end cleanup of the allocator, the fence is the next valid fence supplied
        void postpone_end_frame_cleanup(const vk::queue& queue, memory_allocator& allocator)
        {
          std::lock_guard _l(add_lock);
          to_add.push_back(new frame_allocator_wrapper{queue.get_queue_familly_index(), allocator});
        }

        /// \brief Postpone the destruction of n elements when the fence becomes signaled
        /// \note The fence is the next valid fence
        template<typename... ResourceTypes>
        void postpone_destruction_to_next_fence(const vk::queue& queue, ResourceTypes&& ... resources)
        {
          std::lock_guard _l(add_lock);
          to_add.push_back(new spec_wrapper<ResourceTypes...>{nullptr, queue.get_queue_familly_index(), std::move(resources)...});
        }

        /// \brief Postpone the destruction of n elements when the fence becomes signaled
        /// \note it also destroys the fence
        template<typename... ResourceTypes>
        void postpone_destruction(const vk::queue& queue, vk::fence&& fence, ResourceTypes&& ... resources)
        {
          std::lock_guard _l(lock);
          auto* ptr = new spec_wrapper<vk::fence, ResourceTypes...> {nullptr, queue.get_queue_familly_index(), std::move(fence), std::move(resources)...};
          ptr->fence = &std::get<0>(ptr->resources);

          res_list.push_back(ptr);
        }

        /// \brief Postpone the destruction of n elements when the fence becomes signaled
        /// \note it also destroys the fence
        template<typename... ResourceTypes>
        void postpone_destruction_inclusive(const vk::queue& queue, vk::fence&& fence, ResourceTypes&& ... resources)
        {
          std::lock_guard _l(lock);
          auto* ptr = new spec_wrapper<vk::fence, ResourceTypes...> {nullptr, queue.get_queue_familly_index(), std::move(fence), std::move(resources)...};
          ptr->fence = &std::get<0>(ptr->resources);

          for (auto it = to_add.begin(); it != to_add.end(); ++it)
          {
            if ((*it)->queue_familly == queue.get_queue_familly_index())
            {
              ptr->sublist.push_back(*it);
              it = to_add.erase(it);
            }
          }

          res_list.push_back(ptr);
        }

        /// \brief Postpone the destruction of n elements when the semaphore becomes signaled
        /// \note does not destroy the fence: it must be kept alive until resources are destructed
        template<typename... ResourceTypes>
        void postpone_destruction(const vk::queue& queue, const vk::fence& fence, ResourceTypes&& ... resources)
        {
          std::lock_guard _l(lock);
          auto* ptr = new spec_wrapper<ResourceTypes...>{&fence, queue.get_queue_familly_index(), std::move(resources)...};
          for (auto it = to_add.begin(); it != to_add.end(); ++it)
          {
            if ((*it)->queue_familly == queue.get_queue_familly_index())
            {
              ptr->sublist.push_back(*it);
              it = to_add.erase(it);
            }
          }
          res_list.push_back(ptr);
        }

        /// \brief Perform the check
        void update()
        {
          std::lock_guard _l(lock);
          for (auto it = res_list.begin(); it != res_list.end(); ++it)
          {
            if ((*it)->fence->is_signaled())
            {
              // clean the sublit:
              for (auto* sub_it : (*it)->sublist)
                delete sub_it;

              delete *it;

              it = res_list.erase(it);
            }
          }
        }

        bool has_pending_cleanup() const { return !res_list.empty() || !to_add.empty(); }
        bool has_pending_non_scheduled_cleanup() const { return !to_add.empty(); }

        void _force_full_cleanup()
        {
          while (has_pending_cleanup() || has_pending_non_scheduled_cleanup())
          {
            {
              std::list<wrapper *> temp_list;
              {
                std::lock_guard _l(lock);
                std::swap(temp_list, res_list);
              }
              for (auto it : temp_list)
              {
                // clean the sublit:
                for (auto* sub_it : it->sublist)
                  delete sub_it;
                delete it;
              }
            }

            {
              std::list<wrapper*> temp_list;
              {
                std::lock_guard _l(add_lock);
                std::swap(temp_list, to_add);
              }
              for (auto it : temp_list)
              {
                // clean the sublit:
                for (auto* sub_it : it->sublist)
                  delete sub_it;
                delete it;
              }
            }
          }
        }

      private:
        spinlock lock;
        std::list<wrapper *> res_list;
        spinlock add_lock;
        std::list<wrapper *> to_add;
    };
  } // namespace hydra
} // namespace neam


#endif // __N_154279271610614026_190128118_VK_RESOURCE_DESTRUCTOR_HPP__
