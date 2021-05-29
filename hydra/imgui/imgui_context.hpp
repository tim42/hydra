//
// created by : Timothée Feuillet
// date: Mon May 24 2021 17:27:01 GMT+0200 (CEST)
//
//
// Copyright (c) 2021 Timothée Feuillet
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

#ifndef __N_23433187981268130206_308313449_IMGUI_CONTEXT_HPP__
#define __N_23433187981268130206_308313449_IMGUI_CONTEXT_HPP__

#include <vulkan/vulkan.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace neam
{
  namespace hydra
  {
    namespace imgui
    {
      class context : public glfw::events::raw_keyboard_listener, public glfw::events::raw_mouse_listener
      {
        public:
          context(glfw::window& win, vk::instance& inst, vk::device& dev, vk::swapchain& swapchain, glfw::events::manager& _emgr)
          : window(win),
            device(dev),
            emgr(_emgr),
            imgui_context(*ImGui::CreateContext()),
            io(ImGui::GetIO()),
            ds_pool(dev, 1,
            {
              { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
              { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
              { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
              { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
              { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
              { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
              { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
              { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
              { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
              { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
              { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
            }),
            render_pass(dev)
          {
            IMGUI_CHECKVERSION();
            switch_to();

            //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();

            // FIXME: a custom implem of this one, as hydra can handle multiple windows. For now it's OK, but in the future we'll need to fix this.
            ImGui_ImplGlfw_InitForVulkan(window._get_glfw_handle(), false);
            emgr.register_keyboard_listener(this);
            emgr.register_mouse_listener(this);

            std::pair<size_t, size_t> queue_info = device._get_queue_info(window._get_win_queue());

            init_info = {};
            init_info.Instance = inst._get_vk_instance();
            init_info.PhysicalDevice = device.get_physical_device()._get_vk_physical_device();
            init_info.Device = device._get_vk_device();
            init_info.QueueFamily = queue_info.first;
            init_info.Queue = (VkQueue)queue_info.second;
            init_info.PipelineCache = VK_NULL_HANDLE;
            init_info.DescriptorPool = ds_pool._get_vk_descriptor_pool();
            init_info.Allocator = nullptr;
            init_info.MinImageCount = window._get_surface().get_min_image_count();
            init_info.ImageCount = swapchain.get_image_count();
            init_info.CheckVkResultFn = +[](VkResult res){ check::on_vulkan_error::_log_and_check("[imgui vulkan call]", res); };

            render_pass.create_subpass().add_attachment(neam::hydra::vk::subpass::attachment_type::color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
            render_pass.create_subpass_dependency(VK_SUBPASS_EXTERNAL, 0)
              .dest_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
              .source_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0);

            render_pass.create_attachment().set_swapchain(&swapchain).set_samples(VK_SAMPLE_COUNT_1_BIT)
              .set_load_op(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
              .set_store_op(VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
              .set_layouts(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
            render_pass.refresh();

            ImGui_ImplVulkan_Init(&init_info, render_pass.get_vk_render_pass());
          }

          virtual ~context()
          {
            emgr.unregister_keyboard_listener(this);
            emgr.unregister_mouse_listener(this);

            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext(&imgui_context);
          }

          void switch_to()
          {
            ImGui::SetCurrentContext(&imgui_context);
          }

          bool is_current_context() const
          {
            return ImGui::GetCurrentContext() == &imgui_context;
          }

          void new_frame()
          {
            switch_to();
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
          }

          void init(vk::queue& queue, vk::command_pool& pool, vk_resource_destructor &vrd)
          {
            vk::command_buffer cbuffer = pool.create_command_buffer();

            cbuffer.begin_recording(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            ImGui_ImplVulkan_CreateFontsTexture(cbuffer._get_vk_command_buffer());
            cbuffer.end_recording();

            vk::fence end_init(device);
            vk::submit_info si;
            si << cbuffer >> end_init;
            queue.submit(si);

            // postpone the cleanup at the appropriate time:
            vrd.postpone_destruction(std::move(end_init), std::move(cbuffer));
          }

          void refresh(vk::swapchain& swapchain)
          {
            render_pass.refresh();
            ImGui_ImplVulkan_SetMinImageCount(swapchain._get_surface().get_min_image_count());
          }

          void draw(vk::command_buffer& cbuffer, vk::command_buffer_recorder& cbr, const vk::framebuffer &fb)
          {
            check::on_vulkan_error::n_assert(is_current_context(), "Trying to draw an imgui context when it's not the current one. There might be an error somewhere before.");
            ImGui::Render();

            cbr.begin_render_pass(render_pass, fb, vk::rect2D(glm::ivec2(0, 0), glm::ivec2(fb.get_dimensions().x, fb.get_dimensions().y)), VK_SUBPASS_CONTENTS_INLINE, {});
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cbuffer._get_vk_command_buffer());
            cbr.end_render_pass();
          }

        private:
          void on_mouse_button(int button, int action, int modifiers) final override
          {
            ImGui_ImplGlfw_MouseButtonCallback(window._get_glfw_handle(), button, action, modifiers);
          }
          void on_mouse_wheel(double x, double y) final override
          {
            ImGui_ImplGlfw_ScrollCallback(window._get_glfw_handle(), x, y);
          }
          void on_mouse_move(double /*x*/, double /*y*/) final override {}

          void on_key(int key, int scancode, int action, int modifiers) final override
          {
            ImGui_ImplGlfw_KeyCallback(window._get_glfw_handle(), key, scancode, action, modifiers);
          }
          void on_unicode_input(unsigned int code) final override
          {
            ImGui_ImplGlfw_CharCallback(window._get_glfw_handle(), code);
          }

        private:
          glfw::window& window;
          vk::device& device;
          glfw::events::manager& emgr;
          ImGuiContext& imgui_context;
          ImGuiIO& io;

          ImGui_ImplVulkan_InitInfo init_info;

          neam::hydra::vk::descriptor_pool ds_pool;
          neam::hydra::vk::render_pass render_pass;
      };
    } // namespace imgui
  } // namespace hydra
} // namespace neam

#endif // __N_23433187981268130206_308313449_IMGUI_CONTEXT_HPP__


