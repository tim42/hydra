//
// created by : Timothée Feuillet
// date: 2022-8-27
//
//
// Copyright (c) 2022 Timothée Feuillet
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

#include "transfer.hpp"
#include "engine/hydra_context.hpp"

namespace neam::hydra
{
  batch_transfers::batch_transfers(hydra_context& _hctx)
    : hctx(_hctx)
  {
  }

  void batch_transfers::acquire(vk::buffer& buf, size_t src_queue_familly, vk::semaphore* wait_semaphore, vk::semaphore* signal_semaphore)
  {
    std::lock_guard<spinlock> _u0(transfer_list_lock);
    for (auto& it : buffer_acq_list)
    {
      if (it.buf == &buf)
      {
        check::debug::n_check(false, "batch_transfers::acquire: duplicate buffer in list");
        return;
      }
    }

    buffer_acq_list.push_back(
    {
      &buf,
      wait_semaphore,
      signal_semaphore,
      ~0u,
      src_queue_familly,
    });
  }

  void batch_transfers::acquire(vk::image& img, size_t src_queue_familly, VkImageLayout source_layout, VkImageLayout dest_layout, VkAccessFlags dest_access, vk::semaphore* wait_semaphore, vk::semaphore* signal_semaphore)
  {
    std::lock_guard<spinlock> _u0(transfer_list_lock);
    for (auto& it : image_acq_list)
    {
      if (it.img == &img)
      {
        check::debug::n_check(false, "batch_transfers::acquire: duplicate image in list");
        return;
      }
    }

    image_acq_list.push_back(
    {
      &img,
      wait_semaphore,
      signal_semaphore,
      ~0u,
      src_queue_familly,
      source_layout,
      dest_layout,
      dest_access
    });
  }

  void batch_transfers::add_transfer(vk::buffer& buf, raw_data&& data, size_t buf_offset)
  {
    std::lock_guard<spinlock> _u0(transfer_list_lock);
    total_size += data.size;

    buffer_transfer_list.push_back(_buffer_data_transfer
    {
      &buf, buf_offset, std::move(data),
    });
  }

  void batch_transfers::add_transfer(vk::image& img, raw_data&& data)
  {
    constexpr uint32_t image_alignment = 32;

    std::lock_guard<spinlock> _u0(transfer_list_lock);
    total_size += image_alignment + data.size;

    image_transfer_list.push_back(_image_data_transfer
    {
      &img,
      glm::ivec3(0, 0, 0), img.get_size(),
      image_alignment,
      std::move(data),
    });
  }

  void batch_transfers::add_transfer(vk::image& img, raw_data&& data, const glm::uvec3 &size, const glm::ivec3 &offset)
  {
    constexpr uint32_t image_alignment = 32;

    std::lock_guard<spinlock> _u0(transfer_list_lock);
    total_size += image_alignment + data.size;

    image_transfer_list.push_back(_image_data_transfer
    {
      &img,
      offset, size,
      image_alignment,
      std::move(data),
    });
  }

  bool batch_transfers::transfer(memory_allocator& mem_alloc, neam::hydra::vk::submit_info& si)
  {
    if (buffer_transfer_list.empty() && image_transfer_list.empty())
      return false;

    TRACY_SCOPED_ZONE_COLOR(0x117FFF);

    // FIXME: remove/fix the issue
    vk::validation::state_scope _vsc0(vk::internal::validation_state_t::simple_notice);

    // create the stating buffer:
    vk::buffer staging_buffer = vk::buffer(hctx.device, total_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    staging_buffer._set_debug_name("batch_transfers::staging_buffer");
    memory_allocation alloc = hctx.allocator.allocate_memory(staging_buffer.get_memory_requirements(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, allocation_type::short_lived | allocation_type::mapped_memory);

    staging_buffer.bind_memory(*alloc.mem(), alloc.offset());

    // fill the memory, +create the command buffer
    vk::command_buffer cmd_buf(hctx.tcpm.get_pool().create_command_buffer());
    cmd_buf._set_debug_name("batch_transfers::command_buffer");
    {
      int8_t* memory = (int8_t*)alloc.mem()->map_memory(alloc.offset());
      vk::command_buffer_recorder cbr = cmd_buf.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
      size_t current_offset = 0;

      std::lock_guard<spinlock> _u0(transfer_list_lock);

      // acquisitions: //
      {
        std::vector<vk::buffer_memory_barrier> bmb;
        for (auto& buffer_acq : buffer_acq_list)
        {
          if (buffer_acq.src_queue_familly != ~0u && buffer_acq.src_queue_familly != hctx.tqueue.get_queue_familly_index())
          {
            bmb.push_back(vk::buffer_memory_barrier::queue_transfer(*buffer_acq.buf, buffer_acq.src_queue_familly, hctx.tqueue.get_queue_familly_index()));
          }
        }

        std::vector<vk::image_memory_barrier> imb;
        for (auto& image_acq : image_acq_list)
        {
          imb.push_back(vk::image_memory_barrier::queue_transfer(*image_acq.img, image_acq.src_queue_familly, hctx.tqueue.get_queue_familly_index(), image_acq.source_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
        }

        // perform a single barrier call:
        cbr.pipeline_barrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, {}, bmb, imb);
      }

      // layout changes write barrier: //
      {
        std::vector<vk::image_memory_barrier> imb;
        for (auto& image_acq : image_acq_list)
        {
          vk::image_memory_barrier b
          {
            *image_acq.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
          };

          imb.push_back(b);
        }
        cbr.pipeline_barrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, imb);

      }

      // transfer buffers //
      for (auto& buffer_transfer : buffer_transfer_list)
      {
        TRACY_SCOPED_ZONE;
        const size_t cp_sz = (buffer_transfer.data.size);

        memcpy(memory + current_offset, buffer_transfer.data.get(), cp_sz);

        cbr.copy_buffer(staging_buffer, *buffer_transfer.buf, {{ current_offset, buffer_transfer.buf_offset, cp_sz }});

        total_size -= cp_sz;
        current_offset += cp_sz;
      }

      // transfer images //
      for (auto& image_transfer : image_transfer_list)
      {
        TRACY_SCOPED_ZONE;

        // cbr.pipeline_barrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        // {
        //   vk::image_memory_barrier
        //   {
        //     *image_transfer.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //     VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
        //   }
        // });

        if (current_offset % image_transfer.image_alignment)
          current_offset += image_transfer.image_alignment - current_offset % image_transfer.image_alignment;
        const size_t cp_sz = (image_transfer.data.size);
        memcpy(memory + current_offset, image_transfer.data, cp_sz);

        cbr.copy_buffer_to_image(staging_buffer, *image_transfer.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  { current_offset, image_transfer.offset, image_transfer.size });

        // cbr.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
        // {
        //   vk::image_memory_barrier
        //   {
        //     *image_transfer.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //     VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT
        //   }
        // });

        total_size -= cp_sz;
        current_offset += cp_sz;
      }

      // releases: //
      {
        std::vector<vk::buffer_memory_barrier> bmb;
        std::vector<vk::image_memory_barrier> imb;
        for (auto& buffer_acq : buffer_acq_list)
        {
          buffer_rel_list.push_back(buffer_acq);

          if (buffer_acq.dst_queue_familly != ~0u && buffer_acq.dst_queue_familly != hctx.tqueue.get_queue_familly_index())
          {
            bmb.push_back(vk::buffer_memory_barrier::queue_transfer(*buffer_acq.buf, hctx.tqueue.get_queue_familly_index(), buffer_acq.dst_queue_familly));
          }
        }

        for (auto& image_acq : image_acq_list)
        {
          image_rel_list.push_back(image_acq);

          vk::image_memory_barrier b
          {
            *image_acq.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image_acq.dest_layout,
                                                 VK_ACCESS_TRANSFER_WRITE_BIT, 0
          };
          if (image_acq.dst_queue_familly != ~0u && image_acq.dst_queue_familly != hctx.tqueue.get_queue_familly_index())
          {
            b.set_queue_transfer(hctx.tqueue.get_queue_familly_index(), image_acq.dst_queue_familly);
          //   imb.push_back(vk::image_memory_barrier::queue_transfer(*image_acq.img,  tqueue.get_queue_familly_index(), image_acq.dst_queue_familly,
          //                                                          image_acq.dest_layout, image_acq.dest_layout));
          }
          imb.push_back(b);
        }

        // perform a single barrier call:
        cbr.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, {}, bmb, imb);
      }

      cmd_buf.end_recording();

      // build the submit info:
      si.sync();
      si.on(hctx.tqueue);
      for (const auto& transfer : buffer_acq_list)
      {
        if (transfer.wait_semaphore != nullptr)
          si.wait(*transfer.wait_semaphore, VK_PIPELINE_STAGE_TRANSFER_BIT);
      }
      for (const auto& transfer : image_acq_list)
      {
        if (transfer.wait_semaphore != nullptr)
          si.wait(*transfer.wait_semaphore, VK_PIPELINE_STAGE_TRANSFER_BIT);
      }

      si.execute(cmd_buf);

      for (const auto& transfer : buffer_acq_list)
      {
        if (transfer.signal_semaphore != nullptr)
          si.signal(*transfer.signal_semaphore);
      }
      for (const auto& transfer : image_acq_list)
      {
        if (transfer.signal_semaphore != nullptr)
          si.signal(*transfer.signal_semaphore);
      }

      // add fences:
//       for (const auto& transfer : buffer_transfer_list)
//       {
//         if (transfer.signal_fence != nullptr)
//           si >> *transfer.signal_fence;
//       }
//       for (const auto& transfer : image_transfer_list)
//       {
//         if (transfer.signal_fence != nullptr)
//           si >> *transfer.signal_fence;
//       }

      // if (fence != nullptr)
      //   si >> *fence;
      si.sync();

      // start the transfer / flush
      {
        TRACY_SCOPED_ZONE_COLOR(0xFF0000);
        alloc.mem()->flush(memory, staging_buffer.size(), true);
      }

      // tqueue.submit(si);

      // clear the state of everything:
      image_transfer_list.clear();
      image_acq_list.clear();
      buffer_transfer_list.clear();
      buffer_acq_list.clear();

      hctx.dfe.defer_destruction(hctx.dfe.queue_mask(hctx.tqueue), std::move(cmd_buf), std::move(staging_buffer), std::move(alloc));
    }

    return true;
  }

  void batch_transfers::release_resources(vk::command_buffer_recorder& cbr, size_t dst_queue_familly)
  {
    if (dst_queue_familly == hctx.tqueue.get_queue_familly_index())
      return;
    {
      std::vector<vk::buffer_memory_barrier> bmb;
      std::vector<vk::image_memory_barrier> imb;
      for (auto& buffer_acq : buffer_rel_list)
      {
        if (buffer_acq.dst_queue_familly == dst_queue_familly)
        {
          bmb.push_back(vk::buffer_memory_barrier::queue_transfer(*buffer_acq.buf, hctx.tqueue.get_queue_familly_index(), buffer_acq.dst_queue_familly));
        }
      }

      for (auto& image_acq : image_rel_list)
      {
        if (image_acq.dst_queue_familly == dst_queue_familly)
        {
          imb.push_back(vk::image_memory_barrier::queue_transfer(*image_acq.img, hctx.tqueue.get_queue_familly_index(), image_acq.dst_queue_familly,
                                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image_acq.dest_layout));
        }
      }

      // perform a single barrier call:
      cbr.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, {}, bmb, imb);
    }
  }

  void batch_transfers::acquire_resources(vk::command_buffer_recorder& cbr, size_t src_queue_familly)
  {
    if (src_queue_familly == hctx.tqueue.get_queue_familly_index())
      return;
    {
      std::vector<vk::buffer_memory_barrier> bmb;
      std::vector<vk::image_memory_barrier> imb;
      for (auto& buffer_acq : buffer_rel_list)
      {
        if (buffer_acq.dst_queue_familly == src_queue_familly)
        {
          bmb.push_back(vk::buffer_memory_barrier::queue_transfer(*buffer_acq.buf, buffer_acq.dst_queue_familly, hctx.tqueue.get_queue_familly_index()));
        }
      }

      for (auto& image_acq : image_rel_list)
      {
        if (image_acq.dst_queue_familly == src_queue_familly)
        {
          imb.push_back(vk::image_memory_barrier::queue_transfer(*image_acq.img, image_acq.dst_queue_familly, hctx.tqueue.get_queue_familly_index(),
                                                                 image_acq.source_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
        }
      }

      // perform a single barrier call:
      cbr.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, {}, bmb, imb);
    }
  }
}

