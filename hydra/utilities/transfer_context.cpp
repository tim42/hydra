//
// created by : Timothée Feuillet
// date: 2023-9-1
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

#include "transfer_context.hpp"
#include "engine/hydra_context.hpp"

namespace neam::hydra
{
  transfer_context::transfer_context(hydra_context& _hctx) : transfer_context(_hctx, _hctx.tqueue) {}


  void transfer_context::acquire(vk::buffer& buf, vk::queue& src_queue, vk::semaphore* wait_semaphore)
  {
    // no acquisition necessary when the buffer source queue is the transfer queue
    if (src_queue.get_queue_familly_index() == tqueue.get_queue_familly_index() && wait_semaphore == nullptr)
      return;

    std::lock_guard _lg(lock);
    acquisitions.try_emplace(&src_queue).first->second.buffers.push_back
    ({
      .buffer = buf._get_vk_buffer(),
      .semaphore = wait_semaphore ? wait_semaphore->_get_vk_semaphore() : VK_NULL_HANDLE,
    });
  }

  void transfer_context::acquire(vk::image& img, vk::queue& src_queue, VkImageLayout source_layout, vk::semaphore* wait_semaphore)
  {
    // no acquire necessary when the image source queue is the transfer queue and the layout is the correct one
    if (src_queue.get_queue_familly_index() == tqueue.get_queue_familly_index()
        && source_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        && wait_semaphore == nullptr)
      return;

    std::lock_guard _lg(lock);
    acquisitions.try_emplace(&src_queue).first->second.images.push_back
    ({
      .image = img.get_vk_image(),
      .semaphore = wait_semaphore ? wait_semaphore->_get_vk_semaphore() : VK_NULL_HANDLE,
      .layout = source_layout,
      .layout_for_copy = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    });
  }

  void transfer_context::acquire(vk::image& img, VkImageLayout source_layout, vk::semaphore* wait_semaphore)
  {
    // no acquire necessary when the image source queue is the transfer queue and the layout is the correct one
    if (source_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && wait_semaphore == nullptr)
      return;

    std::lock_guard _lg(lock);
    acquisitions.try_emplace(&tqueue).first->second.images.push_back
    ({
      .image = img.get_vk_image(),
      .semaphore = wait_semaphore ? wait_semaphore->_get_vk_semaphore() : VK_NULL_HANDLE,
      .layout = source_layout,
      .layout_for_copy = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    });
  }

  void transfer_context::acquire_custom_layout_transition(vk::image& img, VkImageLayout source_layout, VkImageLayout copy_layout, vk::semaphore* wait_semaphore)
  {
    std::lock_guard _lg(lock);
    acquisitions.try_emplace(&tqueue).first->second.images.push_back
    ({
      .image = img.get_vk_image(),
      .semaphore = wait_semaphore ? wait_semaphore->_get_vk_semaphore() : VK_NULL_HANDLE,
      .layout = source_layout,
      .layout_for_copy = copy_layout,
    });
  }

  void transfer_context::release(vk::buffer& buf, vk::queue& dst_queue, vk::semaphore* signal_semaphore)
  {
    // no release necessary when the buffer source queue is the transfer queue
    if (dst_queue.get_queue_familly_index() == tqueue.get_queue_familly_index() && signal_semaphore == nullptr)
      return;
    std::lock_guard _lg(lock);
    releases.try_emplace(&dst_queue).first->second.buffers.push_back
    ({
      .buffer = buf._get_vk_buffer(),
      .semaphore = signal_semaphore ? signal_semaphore->_get_vk_semaphore() : VK_NULL_HANDLE,
    });
  }

  void transfer_context::release(vk::image& img, vk::queue& dst_queue, VkImageLayout dst_layout, vk::semaphore* signal_semaphore)
  {
    // no release necessary when the image source queue is the transfer queue and the layout is the correct one
    if (dst_queue.get_queue_familly_index() == tqueue.get_queue_familly_index()
        && dst_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        && signal_semaphore == nullptr)
      return;

    std::lock_guard _lg(lock);
    releases.try_emplace(&dst_queue).first->second.images.push_back
    ({
      .image = img.get_vk_image(),
      .semaphore = signal_semaphore ? signal_semaphore->_get_vk_semaphore() : VK_NULL_HANDLE,
      .layout = dst_layout,
      .layout_for_copy = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    });
  }
  void transfer_context::release_custom_layout_transition(vk::image& img, VkImageLayout copy_layout, VkImageLayout dst_layout, vk::semaphore* signal_semaphore)
  {
    std::lock_guard _lg(lock);
    releases.try_emplace(&tqueue).first->second.images.push_back
    ({
      .image = img.get_vk_image(),
      .semaphore = signal_semaphore ? signal_semaphore->_get_vk_semaphore() : VK_NULL_HANDLE,
      .layout = dst_layout,
      .layout_for_copy = copy_layout,
    });
  }

  void transfer_context::transfer(vk::buffer& buf, raw_data&& data, size_t buf_offset)
  {
    std::lock_guard _lg(lock);
    // Allocate the entry in the copy list:
    auto& ref = buffer_copies.emplace_back(buffer_copy_t
    {
      .dst_buffer = buf._get_vk_buffer(),
      .src_buffer = std::make_unique<std::optional<buffer_holder>>(),
      .offset = buf_offset,
      .size = data.size,
    });

    dispatch_copy_task(std::move(data), *ref.src_buffer);
  }

  async::continuation_chain transfer_context::async_transfer(vk::buffer& buf, raw_data&& data, size_t buf_offset)
  {
    async::continuation_chain chain;
    hctx.tm.get_long_duration_task([=, this, data = std::move(data), &buf, state = chain.create_state()] mutable
    {
      // skip the function if canceled
      if (state.is_canceled())
        return;

      TRACY_SCOPED_ZONE_COLOR(0x110FFF);
      std::optional<buffer_holder> temp_holder;
      inner_copy_task(std::move(data), [&]-> std::optional<buffer_holder>& { return temp_holder; });

      // we got canceled after the memcopy and buffer creation. They will be destructed, but we lost time making them :(
      if (state.is_canceled())
        return;

      {
        // Allocate the entry in the copy list:
        std::lock_guard _lg(lock);
        buffer_copies.emplace_back(buffer_copy_t
        {
          .dst_buffer = buf._get_vk_buffer(),
          .src_buffer = std::make_unique<std::optional<buffer_holder>>(std::move(temp_holder)),
          .offset = buf_offset,
          .size = data.size,
          .completion_state = std::move(state),
        });
      }
    });
    return chain;
  }

  void transfer_context::transfer(vk::image& img, raw_data&& data, const glm::uvec3& size, const glm::ivec3& offset, vk::image_subresource_layers isl, VkImageLayout current_layout)
  {
    std::lock_guard _lg(lock);
    // Allocate the entry in the copy list:
    auto& ref = image_copies.emplace_back(image_copy_t
    {
      .dst_image = img.get_vk_image(),
      .src_buffer = std::make_unique<std::optional<buffer_holder>>(),
      .offset = offset,
      .size = size,
      .isl = isl,
      .layout = current_layout,
    });

    dispatch_copy_task(std::move(data), *ref.src_buffer);
  }

  async::continuation_chain transfer_context::async_transfer(vk::image& img, raw_data&& data, const glm::uvec3& size, const glm::ivec3& offset, vk::image_subresource_layers isl, VkImageLayout current_layout)
  {
    async::continuation_chain chain;
    hctx.tm.get_long_duration_task([=, this, data = std::move(data), &img, state = chain.create_state()] mutable
    {
      // skip the function if canceled
      if (state.is_canceled())
        return;

      TRACY_SCOPED_ZONE_COLOR(0x110FFF);
      std::optional<buffer_holder> temp_holder;
      inner_copy_task(std::move(data), [&]-> std::optional<buffer_holder>& { return temp_holder; });

      // we got canceled after the memcopy and buffer creation. They will be destructed, but we lost time making them :(
      if (state.is_canceled())
        return;

      {
        // Allocate the entry in the copy list:
        std::lock_guard _lg(lock);
        image_copies.emplace_back(image_copy_t
        {
          .dst_image = img.get_vk_image(),
          .src_buffer = std::make_unique<std::optional<buffer_holder>>(std::move(temp_holder)),
          .offset = offset,
          .size = size,
          .isl = isl,
          .layout = current_layout,
          .completion_state = std::move(state),
        });
      }
    });
    return chain;
  }

  void transfer_context::dispatch_copy_task(raw_data&& data, std::optional<buffer_holder>& holder)
  {
    // Dispatch a task for copy
    tasks.emplace_back(hctx.tm.get_task([this, &holder, data = std::move(data)] mutable
    {
      TRACY_SCOPED_ZONE_COLOR(0x115FAA);
      inner_copy_task(std::move(data), [&holder] -> std::optional<buffer_holder>& { return holder; });
    }));
  }

  template<typename GetHolderFnc>
  void transfer_context::inner_copy_task(raw_data&& data, GetHolderFnc get_holder)
  {
    TRACY_SCOPED_ZONE_COLOR(0x117FFF);

    // create the stating buffer and allocate the memory:
    vk::buffer staging_buffer { hctx.device, data.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT };
    staging_buffer._set_debug_name(fmt::format("transfer_context::staging_buffer|{}", debug_context));
    memory_allocation alloc = hctx.allocator.allocate_memory
    (
      staging_buffer.get_memory_requirements(),
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      allocation_type::short_lived | allocation_type::mapped_memory
    );

    // copy&flush the memory:
    staging_buffer.bind_memory(*alloc.mem(), alloc.offset());
    int8_t* memory = (int8_t*)alloc.mem()->map_memory(alloc.offset());
    memcpy(memory, data.get(), data.size);
    alloc.mem()->flush(memory, staging_buffer.size());

    // Update the ref:
    std::optional<buffer_holder>& holder = get_holder();
    holder.emplace(std::move(alloc), std::move(staging_buffer));
  }

  bool transfer_context::has_any_operation_still_in_progress() const
  {
    std::lock_guard _lg(lock);
    for (auto& it : tasks)
    {
      if (!it.is_completed())
        return true;
    }
    return false;
  }

  void transfer_context::remove_operations_for(vk::buffer& buffer)
  {
    std::lock_guard _lg(lock);
    for (auto& ait : acquisitions)
    {
      for (auto it = ait.second.buffers.begin(); it != ait.second.buffers.end();)
      {
        if (it->buffer == buffer._get_vk_buffer())
        {
          it = ait.second.buffers.erase(it);
          continue;
        }
        ++it;
      }
    }
    for (auto& rit : releases)
    {
      for (auto it = rit.second.buffers.begin(); it != rit.second.buffers.end();)
      {
        if (it->buffer == buffer._get_vk_buffer())
        {
          it = rit.second.buffers.erase(it);
          continue;
        }
        ++it;
      }
    }

    for (auto it = buffer_copies.begin(); it != buffer_copies.end();)
    {
      if (it->dst_buffer == buffer._get_vk_buffer())
      {
        for (auto& it : tasks)
        {
          // NOTE: We cannot actively wait for a task, as we have a lock held and that lock is used in oter tasks
          // while (!it.is_completed()) {}
          hctx.tm.actively_wait_for(std::move(it), threading::task_selection_mode::only_current_task_group);
        }
        tasks.clear();

        it = buffer_copies.erase(it);
        continue;
      }
      ++it;
    }
  }

  async::continuation_chain transfer_context::queue_operation_on_build()
  {
    async::continuation_chain ret;
    async::continuation_chain::state st = ret.create_state();

    {
      std::lock_guard _lg(lock);
      states.push_back(std::move(st));
    }
    return ret;
  }

  void transfer_context::remove_operations_for(vk::image& image)
  {
    std::lock_guard _lg(lock);
    for (auto& ait : acquisitions)
    {
      for (auto it = ait.second.images.begin(); it != ait.second.images.end();)
      {
        if (it->image == image.get_vk_image())
        {
          it = ait.second.images.erase(it);
          continue;
        }
        ++it;
      }
    }
    for (auto& rit : releases)
    {
      for (auto it = rit.second.images.begin(); it != rit.second.images.end();)
      {
        if (it->image == image.get_vk_image())
        {
          it = rit.second.images.erase(it);
          continue;
        }
        ++it;
      }
    }

    for (auto it = image_copies.begin(); it != image_copies.end();)
    {
      if (it->dst_image == image.get_vk_image())
      {
        for (auto& it : tasks)
        {
          // NOTE: We cannot actively wait for a task, as we have a lock held and that lock is used in oter tasks
          // while (!it.is_completed()) {}
          hctx.tm.actively_wait_for(std::move(it), threading::task_selection_mode::only_current_task_group);
        }
        tasks.clear();

        it = image_copies.erase(it);
        continue;
      }
      ++it;
    }
  }

  void transfer_context::build(vk::submit_info& si)
  {
    if (buffer_copies.empty() && image_copies.empty() && acquisitions.empty() && releases.empty()) return;

    decltype(states) states_tmp;

    TRACY_SCOPED_ZONE;
    {
      std::lock_guard _lg(lock);

      states_tmp = std::move(states);

      constexpr VkAccessFlags access = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

      // go over all acquisitions:
      std::vector<vk::semaphore> wait_semas;
      if (!acquisitions.empty())
      {
        for (auto& it : acquisitions)
        {
          const bool ignore_queue = (it.first->get_queue_familly_index() == tqueue.get_queue_familly_index());
          if (ignore_queue)
            continue; // skip tqueue stuff
          std::vector<vk::buffer_memory_barrier> bmb;
          for (auto& bit : it.second.buffers)
          {
            bmb.push_back(vk::buffer_memory_barrier::queue_transfer
            (
              bit.buffer,
              it.first->get_queue_familly_index(),
              tqueue.get_queue_familly_index(),
              /*bit.*/access,
              VK_ACCESS_TRANSFER_WRITE_BIT
            ));
          }

          std::vector<vk::image_memory_barrier> imb;
          // also add images here (share the same command buffer):
          for (auto& iit : it.second.images)
          {
            if (it.first == &tqueue) continue;
            imb.push_back(vk::image_memory_barrier::queue_transfer
            (
              iit.image,
              it.first->get_queue_familly_index(),
              tqueue.get_queue_familly_index(),
              iit.layout,
              iit.layout_for_copy,
              /*iit.*/access, VK_ACCESS_TRANSFER_WRITE_BIT
            ));
          }

          vk::command_buffer cb = hctx.get_cpm(*it.first).get_pool().create_command_buffer();
          cb._set_debug_name(fmt::format("transfer_context::build|{}: resource release barriers (original queue -> tqueue)", debug_context));
          {
            vk::command_buffer_recorder cbr = cb.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            cbr.pipeline_barrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, {}, bmb, imb);
            cb.end_recording();
          }

          vk::semaphore sem { hctx.device };
          si.on(*it.first).execute(cb).signal(sem);
          wait_semas.emplace_back(std::move(sem));

          hctx.dfe.defer_destruction(hctx.dfe.queue_mask(*it.first), std::move(cb));
        }
      }

      if (wait_sema != nullptr)
        si.on(tqueue).wait(*wait_sema, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

      if (!acquisitions.empty() || wait_sema != nullptr)
        si.sync();

      std::map<vk::queue*, vk::semaphore> semas;
      // work on tqueue:
      {
        vk::command_buffer cb = hctx.get_cpm(tqueue).get_pool().create_command_buffer();
        cb._set_debug_name(fmt::format("transfer_context::build: tqueue work|{}", debug_context));
        vk::command_buffer_recorder cbr = cb.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        for (auto& it : wait_semas)
          si.on(tqueue).wait(it, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        hctx.dfe.defer_destruction(hctx.dfe.queue_mask(tqueue), std::move(wait_semas));

        // acquire everything on tqueue
        if (!acquisitions.empty())
        {
          neam::hydra::vk::cbr_debug_marker _dm(cbr, "acquisitions");
          std::vector<vk::buffer_memory_barrier> bmb;
          std::vector<vk::image_memory_barrier> imb;
          for (auto& it : acquisitions)
          {
            const bool ignore_queue = (it.first->get_queue_familly_index() == tqueue.get_queue_familly_index());
            for (auto& bit : it.second.buffers)
            {
              bmb.push_back(vk::buffer_memory_barrier::queue_transfer
              (
                bit.buffer,
                ignore_queue ? VK_QUEUE_FAMILY_IGNORED : it.first->get_queue_familly_index(),
                ignore_queue ? VK_QUEUE_FAMILY_IGNORED : tqueue.get_queue_familly_index(),
                /*bit.*/access,
                VK_ACCESS_TRANSFER_WRITE_BIT
              ));
            }

            // also add images here (share the same command buffer):
            for (auto& iit : it.second.images)
            {
              imb.push_back(vk::image_memory_barrier::queue_transfer
              (
                iit.image,
                ignore_queue ? VK_QUEUE_FAMILY_IGNORED : it.first->get_queue_familly_index(),
                ignore_queue ? VK_QUEUE_FAMILY_IGNORED : tqueue.get_queue_familly_index(),
                iit.layout,
                iit.layout_for_copy,
                /*iit.*/access, VK_ACCESS_TRANSFER_WRITE_BIT
              ));
            }
          }
          cbr.pipeline_barrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, {}, bmb, imb);
        }

        // wait for tasks
        {
          TRACY_SCOPED_ZONE_COLOR(0x117FFF);
          for (auto& it : tasks)
          {
            // NOTE: We cannot actively wait for a task, as we have a lock held and that lock is used in oter tasks
            // FIXME: this may be incorrect?
            // also FIXME: we should probably only wait for those tasks at submit (which is next frame, and thus would give more time)
            // but this would require to think a bit more about the transfers
            // while (!it.is_completed()) {}
            hctx.tm.actively_wait_for(std::move(it), threading::task_selection_mode::only_current_task_group);
          }
          tasks.clear();
        }

        // perform copies
        {
          neam::hydra::vk::cbr_debug_marker _dm(cbr, "copies");
          for (auto& it : buffer_copies)
          {
            if (!it.completion_state || !it.completion_state.is_canceled())
            {
              cbr.copy_buffer((**it.src_buffer).buffer._get_vk_buffer(), it.dst_buffer, {{ 0, it.offset, it.size }});
            }

            hctx.dfe.defer_destruction(hctx.dfe.queue_mask(tqueue), std::move(it.src_buffer));

            if (it.completion_state && !it.completion_state.is_canceled())
            {
              hctx.dfe.defer(hctx.dfe.queue_mask(tqueue), [state = std::move(it.completion_state)] mutable
              {
                state.complete();
              });
            }
          }

          for (auto& it : image_copies)
          {
            if (!it.completion_state || !it.completion_state.is_canceled())
            {
              cbr.copy_buffer_to_image((**it.src_buffer).buffer._get_vk_buffer(), it.dst_image, it.layout, { 0, it.offset, it.size, it.isl });
            }

            hctx.dfe.defer_destruction(hctx.dfe.queue_mask(tqueue), std::move(it.src_buffer));

            if (it.completion_state && !it.completion_state.is_canceled())
            {
              hctx.dfe.defer(hctx.dfe.queue_mask(tqueue), [state = std::move(it.completion_state), mip=it.isl.get_mipmap_level()] mutable
              {
                state.complete();
              });
            }
          }
        }

        // release everything from tqueue
        if (!releases.empty())
        {
          neam::hydra::vk::cbr_debug_marker _dm(cbr, "releases");
          std::vector<vk::buffer_memory_barrier> bmb;
          std::vector<vk::image_memory_barrier> imb;
          for (auto& it : releases)
          {
            const bool ignore_queue = (it.first->get_queue_familly_index() == tqueue.get_queue_familly_index());
            for (auto& bit : it.second.buffers)
            {
              bmb.push_back(vk::buffer_memory_barrier::queue_transfer
              (
                bit.buffer,
                ignore_queue ? VK_QUEUE_FAMILY_IGNORED : tqueue.get_queue_familly_index(),
                ignore_queue ? VK_QUEUE_FAMILY_IGNORED : it.first->get_queue_familly_index(),
                VK_ACCESS_TRANSFER_WRITE_BIT, /*bit.*/access
              ));
            }

            // also add images here (share the same command buffer):
            for (auto& iit : it.second.images)
            {
              imb.push_back(vk::image_memory_barrier::queue_transfer
              (
                iit.image,
                ignore_queue ? VK_QUEUE_FAMILY_IGNORED : tqueue.get_queue_familly_index(),
                ignore_queue ? VK_QUEUE_FAMILY_IGNORED : it.first->get_queue_familly_index(),
                iit.layout_for_copy,
                iit.layout,
                VK_ACCESS_TRANSFER_WRITE_BIT, /*iit.*/access
              ));
            }

            if (!ignore_queue)
            {
              // Create the semaphore here (this avoids a submission):
              vk::semaphore sem { hctx.device };
              semas.emplace(it.first, std::move(sem));
            }
          }
          cbr.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, {}, bmb, imb);
        }

        cb.end_recording();
        si.on(tqueue).execute(cb);
        for (auto& semit : semas)
          si.signal(semit.second);
        if (sig_fence != nullptr)
          si.signal(*sig_fence);
        hctx.dfe.defer_destruction(hctx.dfe.queue_mask(tqueue), std::move(cb));
      }

      if (!releases.empty())
        si.sync();

      // release everything (acquire them in their destination queues)
      if (!releases.empty())
      {
        for (auto& it : releases)
        {
          const bool ignore_queue = (it.first->get_queue_familly_index() == tqueue.get_queue_familly_index());
          if (ignore_queue)
            continue; // skip tqueue stuff
          std::vector<vk::buffer_memory_barrier> bmb;
          for (auto& bit : it.second.buffers)
          {
            bmb.push_back(vk::buffer_memory_barrier::queue_transfer
            (
              bit.buffer,
              tqueue.get_queue_familly_index(),
              it.first->get_queue_familly_index(),
              VK_ACCESS_TRANSFER_WRITE_BIT,
              /*bit.*/access
            ));
          }

          std::vector<vk::image_memory_barrier> imb;
          // also add images here (share the same command buffer):
          for (auto& iit : it.second.images)
          {
            imb.push_back(vk::image_memory_barrier::queue_transfer
            (
              iit.image,
              tqueue.get_queue_familly_index(),
              it.first->get_queue_familly_index(),
              iit.layout_for_copy,
              iit.layout,
              VK_ACCESS_TRANSFER_WRITE_BIT, /*iit.*/access
            ));
          }

          vk::command_buffer cb = hctx.get_cpm(*it.first).get_pool().create_command_buffer();
          cb._set_debug_name(fmt::format("transfer_context::build: resource release barriers (tqueue -> destination queue)|{}", debug_context));
          {
            vk::command_buffer_recorder cbr = cb.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            cbr.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, {}, bmb, imb);
            cb.end_recording();
          }
          vk::semaphore sem = std::move(semas.at(it.first));

          si.on(*it.first).wait(sem, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
          si.execute(cb);
          hctx.dfe.defer_destruction(hctx.dfe.queue_mask(*it.first), std::move(cb), std::move(sem));
        }
      }

      // clear everything, but with the lock still held
      // (so that if there's async tasks waiting to push data, it's not being erased)
      tasks.clear();

      acquisitions.clear();
      releases.clear();

      buffer_copies.clear();
      image_copies.clear();

      wait_sema = nullptr;
      sig_fence = nullptr;
    }

    // complete the states with no lock held
    {
      for (auto& it : states_tmp)
      {
        it.complete();
      }
    }
  }

  void transfer_context::clear()
  {
    std::lock_guard _lg(lock);
    for (auto& it : tasks)
      hctx.tm.actively_wait_for(std::move(it), threading::task_selection_mode::only_current_task_group);
    tasks.clear();

    acquisitions.clear();
    releases.clear();

    buffer_copies.clear();
    image_copies.clear();

    wait_sema = nullptr;
    sig_fence = nullptr;
  }

  void transfer_context::append(transfer_context& other)
  {
    // avoids a deadlock
    if (&other == this) return;

    {
      std::lock_guard _olg(other.lock);
      std::lock_guard _lg(lock);
      if (other.tasks.empty())
        return;

      for (auto& it : other.acquisitions)
      {
        acqrel_t& lst = acquisitions.try_emplace(it.first).first->second;
        lst.buffers.insert(lst.buffers.end(), std::make_move_iterator(it.second.buffers.begin()), std::make_move_iterator(it.second.buffers.end()));
        lst.images.insert(lst.images.end(), std::make_move_iterator(it.second.images.begin()), std::make_move_iterator(it.second.images.end()));
      }
      for (auto& it : other.releases)
      {
        acqrel_t& lst = releases.try_emplace(it.first).first->second;
        lst.buffers.insert(lst.buffers.end(), std::make_move_iterator(it.second.buffers.begin()), std::make_move_iterator(it.second.buffers.end()));
        lst.images.insert(lst.images.end(), std::make_move_iterator(it.second.images.begin()), std::make_move_iterator(it.second.images.end()));
      }

      buffer_copies.insert(buffer_copies.end(), std::make_move_iterator(other.buffer_copies.begin()), std::make_move_iterator(other.buffer_copies.end()));
      image_copies.insert(image_copies.end(), std::make_move_iterator(other.image_copies.begin()), std::make_move_iterator(other.image_copies.end()));

      tasks.insert(tasks.end(), std::make_move_iterator(other.tasks.begin()), std::make_move_iterator(other.tasks.end()));

      // finish by clearing all the containsers
      other.acquisitions.clear();
      other.releases.clear();
      other.buffer_copies.clear();
      other.image_copies.clear();
      other.tasks.clear();
    }
  }
}

