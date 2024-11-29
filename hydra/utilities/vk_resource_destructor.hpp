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

#pragma once


#include "../vulkan/fence.hpp"
#include "memory_allocator.hpp"

#include <hydra/vulkan/queue.hpp>
#include <ntools/threading/types.hpp>

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
          wrapper(const vk::fence *_fence, uint32_t _queue_familly) : fence(_fence), queue_familly(_queue_familly) {}
          virtual ~wrapper() {}

          const vk::fence* fence;
          uint32_t queue_familly;
          std::list<wrapper *> sublist;
        };
        template<typename... ResourceTypes>
        struct spec_wrapper : public wrapper
        {
          spec_wrapper(const vk::fence *_fence, uint32_t _queue_familly, ResourceTypes &&... res)
            : wrapper {_fence, _queue_familly}, resources(std::move(res)...)
          {}

          std::tuple<ResourceTypes...> resources;
        };
        struct frame_allocator_wrapper : public wrapper
        {
          frame_allocator_wrapper(uint32_t _queue_familly, memory_allocator& _allocator)
            : wrapper {nullptr, _queue_familly}, allocator(_allocator)
          {}

          virtual ~frame_allocator_wrapper()
          {
            allocator.flush_empty_allocations();
          }

          memory_allocator& allocator;
        };
        struct function_wrapper : public wrapper
        {
          function_wrapper(uint32_t _queue_familly, threading::function_t&& _fnc)
            : wrapper {nullptr, _queue_familly}, fnc(std::move(_fnc))
          {}

          virtual ~function_wrapper()
          {
            fnc();
          }

          threading::function_t fnc;
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

        /// \brief Append the contents of a VRD to the current one
        /// \note The VRD to append must NOT have anything postoned to next fence
        void append(vk_resource_destructor&& o)
        {
#if !N_DISABLE_CHECKS
          if (!o.to_add.empty()) _print_list_summary(o.to_add);
          if (!to_add.empty()) _print_list_summary(to_add);
#endif
          check::debug::n_assert(o.to_add.empty(), "Trying to append an un-complete VRD ({} pending operations)", o.to_add.size());
          check::debug::n_assert(to_add.empty(), "Trying to append on an un-complete VRD ({} pending operations)", to_add.size());
          std::lock_guard _l(lock);
          res_list.splice(res_list.end(), std::move(o.res_list));
        }

        /// \brief Append the contents of a VRD to the current one
        /// \note The VRD to append must NOT have anything postoned to next fence
        /// \warning assumes no other operation are done on the target VRD
        void append(vk_resource_destructor& o)
        {
#if !N_DISABLE_CHECKS
          if (!o.to_add.empty()) _print_list_summary(o.to_add);
          if (!to_add.empty()) _print_list_summary(to_add);
#endif
          check::debug::n_assert(o.to_add.empty(), "Trying to append an un-complete VRD ({} pending operations)", o.to_add.size());
          check::debug::n_assert(to_add.empty(), "Trying to append on an un-complete VRD ({} pending operations)", to_add.size());
          std::lock_guard _l(lock);
          res_list.splice(res_list.end(), std::move(o.res_list));
        }

        /// \brief Append a vrd that has entries postoned to next fence
        /// \warning Should only be done on VRD where a postpone_destruction_inclusive will be called at some point
        void append_incomplete(vk_resource_destructor&& o)
        {
          std::lock_guard _l(lock);
          res_list.splice(res_list.end(), std::move(o.res_list));
          to_add.splice(to_add.end(), std::move(o.to_add));
        }
        /// \brief Append a vrd that has entries postoned to next fence
        /// \warning Should only be done on VRD where a postpone_destruction_inclusive will be called at some point
        void append_incomplete(vk_resource_destructor& o)
        {
          std::lock_guard _l(lock);
          res_list.splice(res_list.end(), std::move(o.res_list));
          to_add.splice(to_add.end(), std::move(o.to_add));
        }

        /// \brief Postpone the frame-end cleanup of the allocator, the fence is the next valid fence supplied
        void postpone_end_frame_cleanup(const vk::queue& queue, memory_allocator& allocator)
        {
          check::debug::n_assert(alllow_fenceless_addition, "Calling postpone_end_frame_cleanup( ... ) without a fence on a VRD that requires a fence");
          std::lock_guard _l(add_lock);
          to_add.push_back(new frame_allocator_wrapper{queue.get_queue_familly_index(), allocator});
        }

        /// \brief Postpone the function call to the next valid fence supplied
        void postpone_to_next_fence(const vk::queue& queue, threading::function_t&& fnc)
        {
          check::debug::n_assert(alllow_fenceless_addition, "Calling postpone_to_next_fence( function ) without a fence on a VRD that requires a fence");
          std::lock_guard _l(add_lock);
          to_add.push_back(new function_wrapper{queue.get_queue_familly_index(), std::move(fnc)});
        }

        /// \brief Postpone the destruction of n elements when the fence becomes signaled
        /// \note The fence is the next valid fence
        template<typename... ResourceTypes>
        void postpone_destruction_to_next_fence(const vk::queue& queue, ResourceTypes&& ... resources)
        {
          check::debug::n_assert(alllow_fenceless_addition, "Calling postpone_destruction_to_next_fence( ... ) without a fence on a VRD that requires a fence");
          std::lock_guard _l(add_lock);
          to_add.push_back(new spec_wrapper<ResourceTypes...>{nullptr, queue.get_queue_familly_index(), std::move(resources)...});
        }

        /// \brief Postpone the destruction of n elements when the fence becomes signaled
        /// \note do not destroy the fence. The fence must be kept alive at the same address as long as it's referenced by the VRD
        template<typename... ResourceTypes>
        void postpone_destruction(const vk::queue& queue, const vk::fence& fence, ResourceTypes&& ... resources)
        {
          if (fence.is_signaled()) return; // let everything be destroyed now, no need to queue anything

          auto* ptr = new spec_wrapper<vk::fence, ResourceTypes...> {&fence, queue.get_queue_familly_index(), std::move(resources)...};

          std::lock_guard _l(lock);
          res_list.push_back(ptr);
        }

        /// \brief Postpone the destruction of n elements when the fence becomes signaled
        /// \note it also destroys the fence
        template<typename... ResourceTypes>
        void postpone_destruction(const vk::queue& queue, vk::fence&& fence, ResourceTypes&& ... resources)
        {
          if (fence.is_signaled()) return; // let everything be destroyed now, no need to queue anything

          auto* ptr = new spec_wrapper<vk::fence, ResourceTypes...> {nullptr, queue.get_queue_familly_index(), std::move(fence), std::move(resources)...};
          ptr->fence = &std::get<0>(ptr->resources);

          std::lock_guard _l(lock);
          res_list.push_back(ptr);
        }

        /// \brief Postpone the destruction of the elements when the fence becomes signaled
        /// \note Also include elements that were postoned to the next fence
        /// \note it also destroys the fence
        template<typename... ResourceTypes>
        void postpone_destruction_inclusive(const vk::queue& queue, vk::fence&& fence, ResourceTypes&& ... resources)
        {
          auto* ptr = new spec_wrapper<vk::fence, ResourceTypes...> {nullptr, queue.get_queue_familly_index(), std::move(fence), std::move(resources)...};
          ptr->fence = &std::get<0>(ptr->resources);

          {
            std::lock_guard _l(add_lock);
            for (auto it = to_add.begin(); it != to_add.end();)
            {
              if ((*it)->queue_familly == queue.get_queue_familly_index())
              {
                ptr->sublist.push_back(*it);
                it = to_add.erase(it);
              }
              else
              {
                ++it;
              }
            }
          }

          std::lock_guard _l(lock);
          res_list.push_back(ptr);
        }

        /// \brief Postpone the destruction of n elements when the semaphore becomes signaled
        /// \note Also include elements that were postoned to the next fence
        /// \note does not destroy the fence: it must be kept alive until resources are destructed
        template<typename... ResourceTypes>
        void postpone_destruction_inclusive(const vk::queue& queue, const vk::fence& fence, ResourceTypes&& ... resources)
        {
          auto* ptr = new spec_wrapper<ResourceTypes...>{&fence, queue.get_queue_familly_index(), std::move(resources)...};
          {
            std::lock_guard _l(add_lock);
            for (auto it = to_add.begin(); it != to_add.end();)
            {
              if ((*it)->queue_familly == queue.get_queue_familly_index())
              {
                ptr->sublist.push_back(*it);
                it = to_add.erase(it);
              }
              else
              {
                ++it;
              }
            }
          }

          std::lock_guard _l(lock);
          res_list.push_back(ptr);
        }

        /// \brief Perform the check
        void update()
        {
          TRACY_SCOPED_ZONE;
          std::lock_guard _l(lock);
          for (auto it = res_list.begin(); it != res_list.end();)
          {
            if ((*it)->fence->is_signaled())
            {
              // clean the sublit:
              for (auto* sub_it : (*it)->sublist)
                delete sub_it;

              delete *it;

              it = res_list.erase(it);
            }
            else
            {
              break;
              ++it;
            }
          }
        }

        bool has_pending_cleanup() const { return !res_list.empty() || !to_add.empty(); }
        bool has_pending_non_scheduled_cleanup() const { return !to_add.empty(); }

        void _force_full_cleanup()
        {
          TRACY_SCOPED_ZONE;
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

      public:
        void assert_on_fenceless_insertions(bool do_assert = true)
        {
          alllow_fenceless_addition = !do_assert;
        }

      private:
#if !N_DISABLE_CHECKS
        static void _print_list_summary(const std::list<wrapper*>& res_list)
        {
          cr::out().log("res-list: {} entries", res_list.size());
          for (auto* it : res_list)
          {
            cr::out().log("  -- queue: {}, sublist-count: {}", it->queue_familly, it->sublist.size());
          }
        }
#endif

      private:
        spinlock lock;
        std::list<wrapper*> res_list;
        spinlock add_lock;
        std::list<wrapper*> to_add;
        bool alllow_fenceless_addition = true;
    };
  } // namespace hydra
} // namespace neam



