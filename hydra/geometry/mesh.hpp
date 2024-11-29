//
// file : mesh.hpp
// in : file:///home/tim/projects/hydra/hydra/geometry/mesh.hpp
//
// created by : Timothée Feuillet
// date: Sat Aug 27 2016 15:44:40 GMT+0200 (CEST)
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


#include <deque>

#include "../vulkan/vulkan.hpp"
#include "../utilities/memory_allocation.hpp"
#include "../utilities/transfer_context.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief Hold informations about geometry (buffers, vertex type/description)
    ///
    /// A valid usage of this class is
    ///  - step 1: tell him what kind of buffer you want, with what size (add_buffer())
    ///  - step 2: setup vertex binding and geometry type
    ///            (set_topology(), has_primitive_restart(),
    ///            get_vertex_input_state())
    ///  - step 3: allocate the memory on the device with the provided requirements
    ///            and bind the memory to that mesh
    ///            (get_memory_requirements(), bind_memory_area())
    ///  - step 4: upload data to the different buffers of the mesh (preferably
    ///            with a batch_transfers object) (transfer_data())
    class mesh
    {
      public:
        /// \brief Create a mesh without anything
        mesh(vk::device &_dev) : dev(_dev) {}

        mesh(mesh &&o)
          : dev(o.dev), topology(o.topology), primitive_restart(o.primitive_restart),
            buffers(std::move(o.buffers)), first_binding(o.first_binding), index_type(o.index_type),
            buffers_offsets(std::move(o.buffers_offsets)), dev_mem(o.dev_mem), index_buffer_present(o.index_buffer_present),
            pvis(std::move(o.pvis))
        {
        }

        mesh &operator = (mesh &&o)
        {
          if (&o == this)
            return *this;
          check::on_vulkan_error::n_assert(&dev == &o.dev, "can't assign two meshes with different vulkan devices");
          topology = o.topology;
          primitive_restart = o.primitive_restart;

          buffers = std::move(o.buffers);
          first_binding = o.first_binding;
          index_type = o.index_type;

          buffers_offsets = std::move(o.buffers_offsets);
          dev_mem = o.dev_mem;
          index_buffer_present = o.index_buffer_present;

          pvis = std::move(o.pvis);
          return *this;
        }

        ~mesh() { allocation.free(); }

        /// \brief Add a buffer
        void add_buffer(size_t size, VkBufferUsageFlags usage, VkBufferCreateFlags flags = 0)
        {
          if (usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) // TODO: test if the vertex buffer usage flag is also set
          {
            check::on_vulkan_error::n_assert(index_buffer_present == false, "meshes can't have more than one index buffer");

            index_buffer_present = true;
            buffers.emplace_front(dev, size, usage, flags);
          }
          else
          {
// #ifndef HYDRA_NO_MESSAGES
//             if ((usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
//               neam::get_global_logger().info() << "adding a buffer with VK_BUFFER_USAGE_INDEX_BUFFER_BIT as a vertex buffer" << neam::cr::newline
//                                    << "'cause an index buffer is already present and this buffer has another usage bit enabled";
// #endif
            buffers.emplace_back(dev, size, usage, flags);
          }
        }

        /// \brief Set the binding of the first vertex buffer of the mesh.
        /// The second buffer will have a binding point of fb + 1, and so on.
        void set_first_binding(size_t fb)
        {
          first_binding = fb;
        }

        /// \brief Return true if the mesh has an index buffer
        bool has_index_buffer() const { return index_buffer_present; }

        /// \brief Return the topology of the mesh
        VkPrimitiveTopology get_topology() const { return topology; }

        /// \brief Set the topology of the mesh
        void set_topology(VkPrimitiveTopology _topology) { topology = _topology; }

        /// \brief Return if the mesh uses primitive restart
        bool has_primitive_restart() const { return primitive_restart; }

        /// \brief Set if the mesh uses primitive restart
        void has_primitive_restart(bool restart) { primitive_restart = restart; }

        /// \brief A nice way to describe binding & attributes of the mesh
        vk::pipeline_vertex_input_state &vertex_input_state() { return pvis; }

        /// \brief Clear the buffers (destroy them, but keep the same state)
        void clear_buffers() { buffers.clear(); index_buffer_present = false; }

        /// \brief Return the memory needed for all the buffers of the mesh
        /// \note If you want a number to allocate memory, use get_memory_requirements()
        /// \see get_memory_requirements()
        size_t get_mesh_memory_consumption() const
        {
          size_t ret = 0;
          for (const vk::buffer &it : buffers)
            ret += it.size();
          return ret;
        }

        /// \brief Return the qty of memory needed to have a contiguous chunk of memory holding
        /// All the buffers
        /// \note Size may vary depending on the offset the said memory
        size_t get_mesh_aligned_memory_consumption() const
        {
          size_t ret = 0;
          for (const vk::buffer &it : buffers)
          {
            VkMemoryRequirements reqs = it.get_memory_requirements();
            if ((ret % reqs.alignment) != 0)
              ret += (reqs.alignment - (ret % reqs.alignment));
            ret += reqs.size;
          }
          return ret;
        }

        /// \brief Return the memory requirements of the mesh
        VkMemoryRequirements get_memory_requirements() const
        {
          VkMemoryRequirements ret;
          ret.size = 0;
          ret.memoryTypeBits = ~0;
          if (buffers.size())
            ret.alignment = buffers[0].get_memory_requirements().alignment;
          else
            ret.alignment = 1;
          for (const vk::buffer &it : buffers)
          {
            VkMemoryRequirements reqs = it.get_memory_requirements();
            if ((ret.size % reqs.alignment) != 0)
              ret.size += (reqs.alignment - (ret.size % reqs.alignment));
            ret.size += reqs.size;
            ret.memoryTypeBits &= reqs.memoryTypeBits;
          }
          return ret;
        }

        /// \brief Allocate and bind some memory for the batch transfer
        /// The memory is then freed when the transfer object is destructed
        void allocate_memory(memory_allocator &mem_alloc, VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocation_type at = allocation_type::persistent)
        {
          _bind_memory_area(mem_alloc.allocate_memory(get_memory_requirements(), flags, at));
        }

        /// \brief Bind a memory area to every single buffers
        /// (it does alignement & everything else).
        /// \see get_memory_requirements()
        /// \note The memory is freed after the end of the mesh's life
        void _bind_memory_area(memory_allocation &&ma)
        {
          _bind_memory_area(*ma.mem(), ma.offset());
          allocation.free();
          allocation = std::move(ma);
        }

        /// \brief Bind a memory area to every single buffers
        /// (it does alignement & everything else).
        /// \see get_memory_requirements()
        void _bind_memory_area(const vk::device_memory &dm, size_t offset, const std::string& debug_name = "mesh")
        {
          buffers_offsets.clear();
          for (vk::buffer &it : buffers)
          {
            auto reqs = it.get_memory_requirements();
            if ((offset % reqs.alignment) != 0)
              offset += (reqs.alignment - (offset % reqs.alignment));

            buffers_offsets.push_back(offset);
            it.bind_memory(dm, offset);
            it._set_debug_name(debug_name);

            offset += reqs.size;
          }
          dev_mem = &dm;
        }

        /// \brief Return the associated device memory (if any)
        const vk::device_memory *_get_device_memory() { return dev_mem; }

        /// \brief Return the offset of a buffer (may throw if no memory)
        size_t _get_buffer_offset(size_t buffer_index) const { return buffers_offsets.at(buffer_index); }

        /// \brief Map the buffer to host memory
        /// \note The bound memory MUST be host visible (and allocated with memory type: mapped_memory)
        void *_map_buffer(size_t buffer_index) const { return dev_mem->map_memory(buffers_offsets.at(buffer_index)); }


        /// \brief Transfer some data to a buffer (using a batch transfer utility)
        /// \param signal_semaphore If specified, a semaphore that will become signaled when the transfer as been completed
        void transfer_data(transfer_context& txctx, size_t buffer_index, raw_data&& data, vk::queue& q, vk::semaphore* signal_semaphore = nullptr)
        {
          transfer_data(txctx, buffer_index, 0, std::move(data), q, signal_semaphore);
        }

        /// \brief Transfer some data to a buffer (using a batch transfer utility)
        /// \param signal_semaphore If specified, a semaphore that will become signaled when the transfer as been completed
        void transfer_data(transfer_context& txctx, size_t buffer_index, size_t offset, raw_data&& data, vk::queue& q, vk::semaphore* signal_semaphore = nullptr)
        {
          txctx.acquire(buffers.at(buffer_index), q);
          txctx.transfer(buffers.at(buffer_index), std::move(data), offset);
          txctx.release(buffers.at(buffer_index), q, signal_semaphore);
        }

        /// \brief Call this to setup the vertex description
        /// \see get_vertex_input_state()
        void setup_vertex_description(vk::graphics_pipeline_creator &pc)
        {
          pc.get_vertex_input_state() = pvis;
          pc.get_input_assembly_state().set_topology(topology).enable_primitive_restart(primitive_restart);
        }

        id_t compute_vertex_description_hash() const
        {
          id_t hash = pvis.compute_hash();
          hash = (id_t)ct::hash::fnv1a_continue<64>((uint64_t)hash, reinterpret_cast<const uint8_t*>(&topology), sizeof(topology));
          hash = (id_t)ct::hash::fnv1a_continue<64>((uint64_t)hash, reinterpret_cast<const uint8_t*>(&primitive_restart), sizeof(primitive_restart));

          return hash;
        }

        /// \brief Bind the buffers to a command buffer
        void bind(vk::command_buffer_recorder &cbr) const
        {
          std::vector<const vk::buffer *> vct_buf;
          vct_buf.reserve(buffers.size());
          if (index_buffer_present)
            cbr.bind_index_buffer(buffers[0], index_type);
          for (size_t i = (index_buffer_present ? 1 : 0); i < buffers.size(); ++i)
            vct_buf.push_back(&buffers[i]);
          cbr.bind_vertex_buffers(first_binding, vct_buf);
        }

      public:
        /// \brief Return the buffers. If there's an index buffer, it has index 0
        /// \note marked as advanced
        std::deque<vk::buffer> &_get_buffers() { return buffers; }

      protected:
        vk::device &dev;

        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        bool primitive_restart = false;

        std::deque<vk::buffer> buffers; // If there's an index buffer, it's the first one

        size_t first_binding = 0;
        VkIndexType index_type = VK_INDEX_TYPE_UINT16;

        std::deque<size_t> buffers_offsets;
        memory_allocation allocation;
        const vk::device_memory *dev_mem = nullptr;

        bool index_buffer_present = false;

        vk::pipeline_vertex_input_state pvis;
    };
  } // namespace hydra
} // namespace neam



