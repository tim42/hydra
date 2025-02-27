//
// created by : Timothée Feuillet
// date: 2/1/25
//
//
// Copyright (c) 2025 Timothée Feuillet
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

#include <hydra/engine/hydra_context.hpp>
#include <hydra/utilities/holders.hpp>

#include "imgui_context.hpp"
#include "shader_structs.hpp"
#include "imgui_engine_module.hpp"
#include <ntools/tracy.hpp>

#include "renderer/ecs/gpu_task_producer.hpp"

namespace neam::hydra::imgui::internals
{
  class setup_pass : public ecs::internal_component<setup_pass>, renderer::concepts::gpu_task_producer::concept_provider<setup_pass>
  {
    public:
      setup_pass(param_t p, hydra_context& _hctx)
        : internal_component_t(p)
        , gpu_task_producer_provider_t(*this, _hctx)
        , related_context(hctx.engine->get_module<imgui_module>()->get_imgui_context())
      {}

    private:
      // skip doing anything if there's no need
//      bool can_render() const { return related_context.should_regenerate_fonts(); }

      void prepare(renderer::gpu_task_context& gtctx)
      {
        // FIXME:
        if (!related_context.should_regenerate_fonts()) return;
        // FIXME: EXTERN TEXTURE instead of using the related-context
        // unalloc the texture and its mem allocation
        if (related_context.font_texture)
        {
          hctx.dfe.defer_destruction(std::move(*related_context.font_texture));
          related_context.font_texture.reset();
        }

        unsigned char* pixels;
        int width, height;
        related_context.get_io().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        size_t upload_size = width * height * 4 * sizeof(char);
        raw_data pixel_data = raw_data::allocate(upload_size);
        memcpy(pixel_data.get(), pixels, pixel_data.size);

        cr::out().debug("imgui: generated a {} x {} font texture", width, height);

        // create the texture:
        related_context.font_texture.emplace
        (
          hctx.allocator, hctx.device, vk::image::create_image_arg
          (
            hctx.device,
            neam::hydra::vk::image_2d
            (
              {width, height}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            )
          )
        );
        related_context.font_texture->image._set_debug_name("imgui::font-texture");
        related_context.font_texture->view._set_debug_name("imgui::font-texture[view]");

        // upload the data to the texture:
        gtctx.transfers.acquire(related_context.font_texture->image, VK_IMAGE_LAYOUT_UNDEFINED);
        gtctx.transfers.transfer(related_context.font_texture->image, std::move(pixel_data));
        gtctx.transfers.release(related_context.font_texture->image, hctx.gqueue, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        related_context.set_font_as_regenerated();
      }

    private:
      imgui_context& related_context;

      friend gpu_task_producer_provider_t;
  };
}
