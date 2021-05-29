//
// file : font_manager.hpp
// in : file:///home/tim/projects/hydra/hydra/renderer/gui/font_manager.hpp
//
// created by : Timothée Feuillet
// date: Sun Sep 04 2016 16:49:04 GMT+0200 (CEST)
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

#ifndef __N_687369083166416517_2966612692_FONT_MANAGER_HPP__
#define __N_687369083166416517_2966612692_FONT_MANAGER_HPP__

#include <deque>
#include <string>

#include "../../hydra_exception.hpp"

#include "../../vulkan/device.hpp"
#include "../../vulkan/buffer.hpp"

#include "../../utilities/transfer.hpp"
#include "../../utilities/memory_allocator.hpp"

#include "font_face.hpp"

namespace neam
{
  namespace hydra
  {
    namespace gui
    {
      /// \brief Load and hold fonts
      /// \note You cannot unload fonts.
      class font_manager
      {
        public:
          // The size of the buffers. (can hold ~100 bfont 0.1 per buffer (less than 1Mo of mem))
          static constexpr size_t buffer_size = (sizeof(internal::character_nfo) * 256) * 100;

        public:
          font_manager(vk::device &_dev, batch_transfers &_btransfers, memory_allocator &_mem_alloc)
            : dev(_dev), btransfers(_btransfers), mem_alloc(_mem_alloc)
          {}

          ~font_manager()
          {
            // free the allocations
            for (memory_allocation &it : buffer_allocations)
              it.free();
          }

          /// \brief Load a font face from a file using a custom image loader
          /// \todo find a more standard way to set an image loader
          template<typename ImageLoader>
          void load_font_face(const std::string &name, const std::string &file)
          {
            ImageLoader loader;
            raw_font_face rff(file, &loader);

            if (current_buffer_offset + sizeof(rff.table) > buffer_size)
            {
              current_buffer_offset = 0;
              buffers.emplace_back(dev, buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
              buffer_allocations.emplace_back(mem_alloc.allocate_memory(buffers.back().get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

              buffers.back().bind_memory(*buffer_allocations.back().mem(), buffer_allocations.back().offset());

            }
            // setup contents of the vertex buffer:
            btransfers.add_transfer(buffers.back(), current_buffer_offset, sizeof(rff.table), rff.table);

            // create the image
            vk::image font_image = vk::image::create_image_arg(dev,
              neam::hydra::vk::image_2d(rff.image_size, VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
            );
            memory_allocation image_alloc = mem_alloc.allocate_memory(font_image.get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocation_type::optimal_image);
            font_image.bind_memory(*image_alloc.mem(), image_alloc.offset());

            // setup the image contents (and get the image layout transitioned to SHADER_READ_ONLY_OPTIMAL)
            btransfers.add_transfer(font_image, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rff.image_size.x * rff.image_size.y, rff.image_data);

            vk::image_view font_view(dev, font_image, VK_IMAGE_VIEW_TYPE_2D);


            // create the font_face object (with the hardcoded 256 entries)
            font_faces_map.emplace(name, font_face(std::move(rff), std::move(font_image), std::move(font_view), image_alloc, &buffers.back(), current_buffer_offset / sizeof(internal::character_nfo), 256));

            // increment the offset in the current buffer:
            current_buffer_offset += sizeof(rff.table);
          }

          /// \brief Retrieve a font face from its name
          const font_face &get_font_face(const std::string &name) const
          {
            auto it = font_faces_map.find(name);
            check::on_vulkan_error::n_assert(it != font_faces_map.end(), "could not find font_face: " + name);

            return it->second;
          }

        private:
          vk::device &dev;
          batch_transfers &btransfers;
          memory_allocator &mem_alloc;

          std::deque<vk::buffer> buffers;
          std::deque<memory_allocation> buffer_allocations;
          size_t current_buffer_offset = buffer_size + 1; // that way, the next load will trigger a buffer allocation

          std::map<std::string, font_face> font_faces_map;
      };
    } // namespace gui
  } // namespace hydra
} // namespace neam

#endif // __N_687369083166416517_2966612692_FONT_MANAGER_HPP__

