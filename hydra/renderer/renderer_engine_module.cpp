//
// created by : Timothée Feuillet
// date: 2022-5-23
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

#include "renderer_engine_module.hpp"


namespace neam::hydra
{
  render_context_ref_t::~render_context_ref_t() { mod._request_removal(*this); }

  renderer_module::render_pass_pair& renderer_module::get_pass_pair_for(VkFormat framebuffer_format, VkImageLayout output_layout)
  {
    const uint64_t key = (uint64_t)output_layout << 32 | (uint64_t)framebuffer_format;
    if (auto it = render_pass_cache.find(key); it != render_pass_cache.end())
      return it->second;

    auto[it, in] = render_pass_cache.emplace(key, render_pass_pair{{hctx->device}, {hctx->device}});
    render_pass_pair& ret = it->second;

    ret.init_render_pass.create_subpass().add_attachment(neam::hydra::vk::subpass::attachment_type::color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
    ret.init_render_pass.create_subpass_dependency(VK_SUBPASS_EXTERNAL, 0)
      .dest_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
      .source_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0);

    ret.init_render_pass.create_attachment().set_format(framebuffer_format).set_samples(VK_SAMPLE_COUNT_1_BIT)
      .set_load_op(VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
      .set_store_op(VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
      .set_layouts(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ret.init_render_pass.refresh();

    // create the output render pass:
    ret.output_render_pass.create_subpass().add_attachment(neam::hydra::vk::subpass::attachment_type::color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
    ret.output_render_pass.create_subpass_dependency(VK_SUBPASS_EXTERNAL, 0)
      .dest_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
      .source_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0);

    ret.output_render_pass.create_attachment().set_format(framebuffer_format).set_samples(VK_SAMPLE_COUNT_1_BIT)
      .set_load_op(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
      .set_store_op(VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
      .set_layouts(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, output_layout);
    ret.output_render_pass.refresh();
    return ret;
  }

  void renderer_module::render_context(neam::hydra::renderer_module::context_ref_t& ref)
  {
    context_t& context = *ref;

    context.begin();
    const uint32_t index = context.get_framebuffer_index();

    render_pass_context rpctx
    {
      .output_size = context.size,
      .viewport = vk::viewport{glm::vec2{context.size}},
      .viewport_rect = vk::rect2D{{0, 0}, context.size},
      .output_format = context.framebuffer_format,
      .final_fb = context.framebuffers[index],
    };

    context.pm.setup(rpctx, context.need_setup);
    context.need_setup = false;

    context.pm.prepare(rpctx);

    vk::submit_info gsi;
    vk::submit_info csi;

    auto& rpp = get_pass_pair_for(context.framebuffer_format, context.output_layout);
//     gsi << vk::cmd_sema_pair {state->image_ready, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    context.pre_render(gsi);
    neam::hydra::vk::command_buffer init_frame_command_buffer = context.graphic_transient_cmd_pool.create_command_buffer();
    {
      neam::hydra::vk::command_buffer_recorder cbr = init_frame_command_buffer.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
      cbr.begin_render_pass(rpp.init_render_pass, rpctx.final_fb, rpctx.viewport_rect, VK_SUBPASS_CONTENTS_INLINE, {});
      cbr.end_render_pass();
    }
    init_frame_command_buffer.end_recording();
    gsi << init_frame_command_buffer;

    bool has_any_compute = false;
    context.pm.render(gsi, csi, rpctx, has_any_compute);

    neam::hydra::vk::command_buffer frame_command_buffer = context.graphic_transient_cmd_pool.create_command_buffer();
    {
      neam::hydra::vk::command_buffer_recorder cbr = frame_command_buffer.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
      cbr.begin_render_pass(rpp.output_render_pass, rpctx.final_fb, rpctx.viewport_rect, VK_SUBPASS_CONTENTS_INLINE, {});
      cbr.end_render_pass();
    }
    frame_command_buffer.end_recording();

//     gsi << frame_command_buffer >> state->render_finished;
    context.post_render(gsi);

    vk::fence gframe_done{hctx->device};
    gsi >> gframe_done;

    std::optional<vk::fence> cframe_done;
    if (has_any_compute)
    {
      cframe_done.emplace(hctx->device);
      csi >> *cframe_done;
    }

    hctx->gqueue.submit(gsi);
    if (has_any_compute)
      hctx->cqueue.submit(csi);

    // TODO: present from compute queue support
//     hctx->gqueue.present(state->swapchain, index, { &state->render_finished }, &out_of_date);
    context.post_submit();

    hctx->vrd.postpone_end_frame_cleanup(hctx->gqueue, hctx->allocator);

    hctx->vrd.postpone_destruction_to_next_fence(hctx->gqueue, std::move(init_frame_command_buffer), std::move(frame_command_buffer));

    context.pm.cleanup();

    hctx->vrd.postpone_destruction_inclusive(hctx->gqueue, std::move(gframe_done));
    if (has_any_compute)
      hctx->vrd.postpone_destruction_inclusive(hctx->cqueue, std::move(*cframe_done));

    context.end();
//     if (out_of_date || recreate)
//       state->refresh();
  }
}

