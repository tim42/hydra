//
// file : submit_info.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/submit_info.hpp
//
// created by : Timothée Feuillet
// date: Fri Apr 14 2024 19:22:33 GMT+0200 (CEST)
//
//
// Copyright (c) 2024 Timothée Feuillet
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

#include "submit_info.hpp"

#include "../utilities/memory_allocation.hpp"

namespace neam::hydra::vk
{
  void submit_info::vk_si_vectors::update(VkSubmitInfo& si)
  {
    si.commandBufferCount = vk_cmd_bufs.size();
    si.pCommandBuffers = vk_cmd_bufs.data();

    si.pWaitDstStageMask = wait_dst_stage_mask.data();

    si.waitSemaphoreCount = vk_wait_semas.size();
    si.pWaitSemaphores = vk_wait_semas.data();
    si.signalSemaphoreCount = vk_sig_semas.size();
    si.pSignalSemaphores = vk_sig_semas.data();
  }

  void submit_info::vk_sbi_vectors::update(VkBindSparseInfo& sbi)
  {
    TRACY_SCOPED_ZONE;

    // build the buffers:
    vk_buffer_binds.resize(buffer_sparse_binds.size());
    vk_image_opaque_binds.resize(image_sparse_opaque_binds.size());
    vk_image_binds.resize(image_sparse_binds.size());

    {
      uint32_t i = 0;
      for (auto& it : buffer_sparse_binds)
      {
        vk_buffer_binds[i].buffer = it.first;
        vk_buffer_binds[i].bindCount = it.second.size();
        vk_buffer_binds[i].pBinds = it.second.data();
        ++i;
      }
    }
    {
      uint32_t i = 0;
      for (auto& it : image_sparse_opaque_binds)
      {
        vk_image_opaque_binds[i].image = it.first;
        vk_image_opaque_binds[i].bindCount = it.second.size();
        vk_image_opaque_binds[i].pBinds = it.second.data();
        ++i;
      }
    }
    {
      uint32_t i = 0;
      for (auto& it : image_sparse_binds)
      {
        vk_image_binds[i].image = it.first;
        vk_image_binds[i].bindCount = it.second.size();
        vk_image_binds[i].pBinds = it.second.data();
        ++i;
      }
    }

    sbi.bufferBindCount = vk_buffer_binds.size();
    sbi.pBufferBinds = vk_buffer_binds.data();
    sbi.imageOpaqueBindCount = vk_image_opaque_binds.size();
    sbi.pImageOpaqueBinds = vk_image_opaque_binds.data();
    sbi.imageBindCount = vk_image_binds.size();
    sbi.pImageBinds = vk_image_binds.data();

    sbi.waitSemaphoreCount = vk_wait_semas.size();
    sbi.pWaitSemaphores = vk_wait_semas.data();
    sbi.signalSemaphoreCount = vk_sig_semas.size();
    sbi.pSignalSemaphores = vk_sig_semas.data();
  }

  void submit_info::vk_si_wrapper::update(uint32_t index)
  {
    // relink debug markers:
    //for (auto&& it : begin_markers)
    //  it.marker.pLabelName = it.str.c_str();
    //for (auto&& it : insert_markers)
    //  it.marker.pLabelName = it.str.c_str();

    if (!sparse_bind)
    {
      if (vk_submit_infos.size() > 0)
      {
        if (index >= vk_submit_infos.size())
          index = vk_submit_infos.size() - 1;
        si_vectors[index].update(vk_submit_infos[index]);
      }
    }
    else
    {
      if (vk_sparse_bind_infos.size() > 0)
      {
        if (index >= vk_sparse_bind_infos.size())
          index = vk_sparse_bind_infos.size() - 1;
        sbi_vectors[index].update(vk_sparse_bind_infos[index]);
      }
    }
  }

  void submit_info::vk_si_wrapper::full_update()
  {
    // relink debug markers:
    //for (auto&& it : begin_markers)
    //  it.marker.pLabelName = it.str.c_str();
    //for (auto&& it : insert_markers)
     // it.marker.pLabelName = it.str.c_str();

    if (!sparse_bind)
    {
      for (uint32_t i = 0; i < (uint32_t)vk_submit_infos.size(); ++i)
      {
        update(i);
      }
    }
    else
    {
      for (uint32_t i = 0; i < (uint32_t)vk_sparse_bind_infos.size(); ++i)
      {
        update(i);
      }
    }
  }

  void submit_info::vk_si_wrapper::add()
  {
    if (!sparse_bind)
    {
      vk_submit_infos.emplace_back();
      si_vectors.emplace_back();

      vk_submit_infos.back().sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      vk_submit_infos.back().pNext = nullptr;
    }
    else
    {
      vk_sparse_bind_infos.emplace_back();
      sbi_vectors.emplace_back();

      vk_sparse_bind_infos.back().sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
      vk_sparse_bind_infos.back().pNext = nullptr;
    }
  }

  void submit_info::vk_si_wrapper::submit(vk_context& vkctx, vk::queue* queue)
  {
    if (vk_submit_infos.empty() && vk_sparse_bind_infos.empty())
      return;

    check::debug::n_assert(queue != nullptr, "submit-info has commands to submit but no queue");

    // FIXME:
    full_update();

    bool fence_only = vk_sparse_bind_infos.size() <= 1 && vk_submit_infos.size() <= 1;
    if (fence_only)
    {
      if (!sparse_bind)
      {
        fence_only = vk_submit_infos.back().commandBufferCount == 0
                  && vk_submit_infos.back().waitSemaphoreCount == 0
                  && vk_submit_infos.back().signalSemaphoreCount == 0;
      }
      else
      {
        fence_only = vk_sparse_bind_infos.back().bufferBindCount == 0
                  && vk_sparse_bind_infos.back().imageOpaqueBindCount == 0
                  && vk_sparse_bind_infos.back().imageBindCount == 0
                  && vk_sparse_bind_infos.back().waitSemaphoreCount == 0
                  && vk_sparse_bind_infos.back().signalSemaphoreCount == 0;
      }
    }

    //for (auto&& it : begin_markers)
     // vkctx.device._vkQueueBeginDebugUtilsLabel(queue->_get_vk_queue(), &it.marker);
    //for (auto&& it : insert_markers)
     // vkctx.device._vkQueueInsertDebugUtilsLabel(queue->_get_vk_queue(), &it.marker);

    if (fence_only)
    {
      if (fence)
      {
#if N_ALLOW_DEBUG
          cr::out().debug(" - -- submit: [queue {}: fence only]", vkctx.get_queue_name(*queue));
#endif
          check::on_vulkan_error::n_assert_success(vkctx.device._vkQueueSubmit(queue->_get_vk_queue(), 0, nullptr, fence));
      }
    }
    else
    {
      if (!sparse_bind)
      {
#if N_ALLOW_DEBUG
        cr::out().debug(" - -- submit: [queue {}: {} entries]", vkctx.get_queue_name(queue), vk_submit_infos.size());
#endif
        check::on_vulkan_error::n_assert_success(vkctx.device._vkQueueSubmit(queue->_get_vk_queue(), vk_submit_infos.size(), vk_submit_infos.data(), fence));
      }
      else
      {
#if N_ALLOW_DEBUG
        cr::out().debug(" - -- sparse-bind: [queue {}: {} entries]", vkctx.get_queue_name(queue), vk_sparse_bind_infos.size());
#endif
        check::on_vulkan_error::n_assert_success(vkctx.device._vkQueueBindSparse(queue->_get_vk_queue(), vk_sparse_bind_infos.size(), vk_sparse_bind_infos.data(), fence));
      }
    }
   // for (uint32_t i = 0; i < end_markers; ++i)
     // vkctx.device._vkQueueEndDebugUtilsLabel(queue->_get_vk_queue());
  }


  void submit_info::clear()
  {
#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [clear]", (void*)this);
#endif
    current = nullptr;
    current_queue = VK_NULL_HANDLE;
    queues.clear();
  }

  submit_info& submit_info::on(vk::queue& q)
  {
    switch_to(&q);
    sparse_bind_ops(false);
    return *this;
  }

  submit_info& submit_info::sparse_bind_on(vk::queue& q)
  {
    switch_to(&q);
    sparse_bind_ops(true);
    return *this;
  }

  void submit_info::sync()
  {
    cut();
  }

  submit_info& submit_info::wait(const semaphore &sem, VkPipelineStageFlags wait_flags)
  {
    step(operation_t::wait);
    check::debug::n_assert(!current->queue_submits.back().sparse_bind, "full-wait called on a sparse-bind queue submit");

#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [{}: waiting semaphore]", (void*)this, vkctx.get_queue_name(*current_queue));
#endif
    current->queue_submits.back().si_vectors.back().vk_wait_semas.push_back(sem._get_vk_semaphore());
    current->queue_submits.back().si_vectors.back().wait_dst_stage_mask.push_back(wait_flags);
    return *this;
  }

  submit_info& submit_info::wait(const semaphore &sem)
  {
    step(operation_t::wait);
    check::debug::n_assert(current->queue_submits.back().sparse_bind, "sparse-bind-wait called on a non-sparse-bind queue submit");

#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [{}: waiting semaphore]", (void*)this, vkctx.get_queue_name(*current_queue));
#endif
    current->queue_submits.back().sbi_vectors.back().vk_wait_semas.push_back(sem._get_vk_semaphore());
    return *this;
  }

  submit_info& submit_info::execute(const command_buffer &cmdbuf)
  {
#if !N_DISABLE_CHECKS
    check::debug::n_assert(current_queue->_get_vk_queue() == cmdbuf.queue,
                            "submit-info: execute: command buffer queue != current queue: cmd buf queue: {}, current queue: {}",
                            vkctx.get_queue_name(cmdbuf.queue), vkctx.get_queue_name(*current_queue)
                          );
#endif
    step(operation_t::cmd_buff_or_bind);

    check::debug::n_assert(!current->queue_submits.back().sparse_bind, "execute called on a sparse-bind queue submit");

#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [{}: execute command buffer]", (void*)this, vkctx.get_queue_name(*current_queue));
#endif
    current->queue_submits.back().si_vectors.back().vk_cmd_bufs.push_back(cmdbuf._get_vk_command_buffer());
    return *this;
  }

  submit_info& submit_info::bind(const buffer& buff, memory_allocation& alloc, size_t offset_in_buffer)
  {
    step(operation_t::cmd_buff_or_bind);

#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [{}: bind memory to buffer]", (void*)this, vkctx.get_queue_name(*current_queue));
#endif

    check::debug::n_assert(current->queue_submits.back().sparse_bind, "bind called on a non-sparse-bind queue submit");

    auto[it, ins] = current->queue_submits.back().sbi_vectors.back().buffer_sparse_binds.try_emplace(buff._get_vk_buffer());
    it->second.push_back
    ({
      .resourceOffset = offset_in_buffer,
      .size = alloc.size(),
      .memory = alloc.mem()->_get_vk_device_memory(),
      .memoryOffset = alloc.offset(),
      .flags = 0,
    });

    return *this;
  }

  submit_info& submit_info::bind_mip_tail(const image& img, memory_allocation& alloc, size_t offset_in_opaque)
  {
    step(operation_t::cmd_buff_or_bind);

#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [{}: bind memory to image mip-tail]", (void*)this, vkctx.get_queue_name(*current_queue));
#endif

    check::debug::n_assert(current->queue_submits.back().sparse_bind, "bind called on a non-sparse-bind queue submit");

    auto[it, ins] = current->queue_submits.back().sbi_vectors.back().image_sparse_opaque_binds.try_emplace(img.get_vk_image());
    it->second.push_back
    ({
      .resourceOffset = offset_in_opaque,
      .size = alloc.size(),
      .memory = alloc.mem()->_get_vk_device_memory(),
      .memoryOffset = alloc.offset(),
      .flags = VK_SPARSE_MEMORY_BIND_METADATA_BIT,
    });

    return *this;
  }

  submit_info& submit_info::bind(const image& img, memory_allocation& alloc, size_t offset_in_opaque)
  {
    step(operation_t::cmd_buff_or_bind);

#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [{}: opaquely bind memory to image]", (void*)this, vkctx.get_queue_name(*current_queue));
#endif

    check::debug::n_assert(current->queue_submits.back().sparse_bind, "bind called on a non-sparse-bind queue submit");

    auto[it, ins] = current->queue_submits.back().sbi_vectors.back().image_sparse_opaque_binds.try_emplace(img.get_vk_image());
    it->second.push_back
    ({
      .resourceOffset = offset_in_opaque,
      .size = alloc.size(),
      .memory = alloc.mem()->_get_vk_device_memory(),
      .memoryOffset = alloc.offset(),
      .flags = 0,
    });

    return *this;
  }

  submit_info& submit_info::bind(const image& img, memory_allocation& alloc, glm::uvec3 offset, glm::uvec3 extent, const image_subresource& subres)
  {
    step(operation_t::cmd_buff_or_bind);

#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [{}: bind memory to image]", (void*)this, vkctx.get_queue_name(*current_queue));
#endif

    check::debug::n_assert(current->queue_submits.back().sparse_bind, "bind called on a non-sparse-bind queue submit");

    auto[it, ins] = current->queue_submits.back().sbi_vectors.back().image_sparse_binds.try_emplace(img.get_vk_image());
    it->second.push_back
    ({
      .subresource = subres,
      .offset = { (int32_t)offset.x, (int32_t)offset.y, (int32_t)offset.z },
      .extent = { extent.x, extent.y, extent.z },
      .memory = alloc.mem()->_get_vk_device_memory(),
      .memoryOffset = alloc.offset(),
      .flags = 0,
    });

    return *this;
  }

  submit_info& submit_info::signal(const semaphore &sem)
  {
    step(operation_t::signal_sema);

#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [{}: signaling semaphore]", (void*)this, vkctx.get_queue_name(*current_queue));
#endif

    if (!current->queue_submits.back().sparse_bind)
      current->queue_submits.back().si_vectors.back().vk_sig_semas.push_back(sem._get_vk_semaphore());
    else
      current->queue_submits.back().sbi_vectors.back().vk_sig_semas.push_back(sem._get_vk_semaphore());
    return *this;
  }

  submit_info& submit_info::signal(const fence &fnc)
  {
    step(operation_t::signal_fence);
#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [{}: signaling fence]", (void*)this, vkctx.get_queue_name(*current_queue));
#endif

    current->queue_submits.back().fence = fnc._get_vk_fence();
    return *this;
  }

  submit_info& submit_info::append(const submit_info& info)
  {
#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [append {}]", (void*)this, (void*)&info);
#endif

    if (info.queues.empty())
      return *this;

    TRACY_SCOPED_ZONE;
    vk::queue* last_queue = current_queue;
    current_queue = nullptr;
    current = nullptr;
    const uint32_t first_index = (uint32_t)queues.size();

    queues.insert(queues.end(), info.queues.begin(), info.queues.end());

    // relink everything:
    for (uint32_t i = first_index; i < (uint32_t)queues.size(); ++i)
    {
      for (auto& it : queues[i])
      {
        for (auto& si_it : it.second)
        {
          for (auto& qs_it : si_it.queue_submits)
          {
            qs_it.full_update();
          }
        }
      }
    }

    switch_to(last_queue);
    return *this;
  }

  void submit_info::deferred_submit()
  {
    if (queues.empty()) return;

    std::lock_guard _lg(vkctx.dqe.lock);

    deferred_submit_unlocked();
  }

  void submit_info::deferred_submit_unlocked()
  {
#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [deferred_submit]", (void*)this);
#endif
    if (queues.empty()) return;

    TRACY_SCOPED_ZONE;
    for (uint32_t i = 0; i < (uint32_t)queues.size(); ++i) // syncs
    {
      // FIXME: handle the case where sync() is called first (currently, we skip this)
      if (i != 0)
        vkctx.dqe.defer_sync_unlocked();

      for (auto& it : queues[i]) // map of queues
      {
        for (auto& si_it : it.second) // vector queue ops
        {
          for (auto& qs_it : si_it.queue_submits)
          {
            qs_it.update();
            vkctx.dqe.defer_execution_unlocked(it.first->queue_id, [&vkctx = vkctx, queue = it.first, qs_it = std::move(qs_it)] mutable
            {
              TRACY_SCOPED_ZONE;
              std::lock_guard _l(queue->queue_lock);
              qs_it.submit(vkctx, queue);
            });
          }
        }
      }
    }
  }

  bool submit_info::has_any_entries_for(vk::queue& q) const
  {
    for (auto &it : queues)
    {
      if (it.contains(&q))
        return true;
    }
    return false;
  }

  void submit_info::sparse_bind_ops(bool do_sparse_bind)
  {
    if (!current->queue_submits.size() || current->queue_submits.back().sparse_bind != do_sparse_bind)
    {
      current->queue_submits.emplace_back(do_sparse_bind);
      current->last_op = operation_t::initial;
    }
  }
  void submit_info::begin_marker(std::string_view name, glm::vec4 color)
  {
    return;
    // step(operation_t::_cut);
    //
    // if (!vkctx.device._has_vkQueueBeginDebugUtilsLabel()) return;
    //
    // current->queue_submits.back().begin_markers.emplace_back(std::string(name),
    // VkDebugUtilsLabelEXT
    // {
    //   .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
    //   .pNext = nullptr,
    //   .pLabelName = name.data(),
    //   .color = {color.r, color.g, color.b, color.a},
    // });
  }
  void submit_info::end_marker()
  {
    return;
    // step(operation_t::end_marker);
    //
    // if (vkctx.device._has_vkQueueEndDebugUtilsLabel())
    // {
    //   current->queue_submits.back().end_markers += 1;
    // }
  }

  void submit_info::insert_marker(std::string_view name, glm::vec4 color)
  {
    return;
    // step(operation_t::_cut);
    // if (!vkctx.device._has_vkQueueInsertDebugUtilsLabel()) return;
    //
    // current->queue_submits.back().insert_markers.emplace_back(std::string(name),
    // VkDebugUtilsLabelEXT
    // {
    //   .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
    //   .pNext = nullptr,
    //   .pLabelName = name.data(),
    //   .color = {color.r, color.g, color.b, color.a},
    // });
  }

  void submit_info::step(operation_t current_op)
  {
    const bool is_regression = std::to_underlying(current->last_op) > std::to_underlying(current_op);
    if (!current->queue_submits.size())
      current->queue_submits.emplace_back();
    else if ((is_regression || current_op == operation_t::signal_fence) && current->queue_submits.back().fence != VK_NULL_HANDLE)
      current->queue_submits.emplace_back(current->queue_submits.back().sparse_bind);
    else if (is_regression)
      current->queue_submits.back().add();
    current->last_op = (current_op == operation_t::_cut ? operation_t::initial : current_op);
  }

  void submit_info::switch_to(vk::queue* q)
  {
    if (current_queue == q) return;
    if (queues.empty()) queues.emplace_back();

    current_queue = q;
#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [switching to queue {}]", (void*)this, vkctx.get_queue_name(*current_queue));
#endif
    if (auto it = queues.back().find(q); it != queues.back().end())
    {
      current = &it->second.back();
      return;
    }
    auto[it, ins] = queues.back().emplace(q, std::mtc_deque<queue_operations_t>{});
    it->second.emplace_back();
    current = &it->second.back();
    return;
  }

  void submit_info::cut()
  {
    if (!queues.empty() && queues.back().empty())
      return;
#if N_ALLOW_DEBUG
    cr::out().debug(" - si {}: [cutting]", (void*)this);
#endif
    queues.emplace_back();

    current_queue = nullptr;
    current = nullptr;
  }
}
