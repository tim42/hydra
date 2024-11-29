//
// created by : Timothée Feuillet
// date: 2023-9-15
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

#pragma once

#include <vector>

#include "renderer_engine_module.hpp"

namespace neam::hydra
{
  /// \brief Simple implementation of a offscreen render-context
  /// \note this contexts owns and maintains the images
  class offscreen_render_context_t : public render_context_t
  {
    public:
      offscreen_render_context_t(hydra_context& _hctx, std::vector<VkFormat>&& _formats, glm::uvec2 _size)
         : render_context_t(_hctx)
         , formats(std::move(_formats))
      {
        size = _size;
      }
      offscreen_render_context_t(hydra_context& _hctx, std::initializer_list<VkFormat>&& _formats, glm::uvec2 _size)
         : render_context_t(_hctx)
         , formats(std::move(_formats))
      {
        size = _size;
      }

      std::vector<VkFormat> formats;

      allocation_type allocation = allocation_type::persistent_optimal_image;
      VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

      bool recreate = true;

      std::vector<vk::image> images;
      std::vector<memory_allocation> images_allocations;
      std::vector<vk::image_view> images_views;

      std::vector<vk::image> images_to_delete;
      std::vector<memory_allocation> images_allocations_to_delete;
      std::vector<vk::image_view> images_views_to_delete;

      std::vector<VkFormat> get_framebuffer_format() const override { return formats; }

      void begin() override
      {
        if (!images_to_delete.empty())
        {
          hctx.dfe.defer_destruction(std::move(images_views_to_delete),
                                     std::move(images_to_delete),
                                     std::move(images_allocations_to_delete));
        }

        if (recreate)
        {
          recreate = false;
          if (size.x == 0)
            size.x = 1;
          if (size.y == 0)
            size.y = 1;

          images_to_delete = std::move(images);
          images_views_to_delete = std::move(images_views);
          images_allocations_to_delete = std::move(images_allocations);

          images.reserve(formats.size());
          images_allocations.reserve(formats.size());
          images_views.reserve(formats.size());
          for (auto& it : formats)
          {
            // create the image
            images.emplace_back
            (
              neam::hydra::vk::image::create_image_arg
              (
                hctx.device,
                neam::hydra::vk::image_2d(size, it, VK_IMAGE_TILING_OPTIMAL, usage_flags)
              )
            );

            // allocate the memory
            images_allocations.emplace_back
            (
              hctx.allocator.allocate_memory(images.back().get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocation)
            );
            // bind the memory
            images.back().bind_memory(*images_allocations.back().mem(), images_allocations.back().offset());

            // create the image view
            images_views.emplace_back(hctx.device, images.back());
          }
        }
      }

      std::vector<vk::image*> get_images() override
      {
        std::vector<vk::image*> ret;
        ret.reserve(images.size());
        for (auto& it : images)
          ret.emplace_back(&it);
        return ret;
      }

      std::vector<vk::image_view*> get_images_views() override
      {
        std::vector<vk::image_view*> ret;
        ret.reserve(images_views.size());
        for (auto& it : images_views)
          ret.emplace_back(&it);
        return ret;
      }
  };
}

