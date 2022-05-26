//
// file : sample.hpp
// in : file:///home/tim/projects/hydra/samples/terrain/sample.hpp
//
// created by : Timothée Feuillet
// date: Sat Nov 05 2016 18:39:18 GMT-0400 (EDT)
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

#ifndef __N_94954772477713296_2016124446_SAMPLE_HPP__
#define __N_94954772477713296_2016124446_SAMPLE_HPP__

// #define HYDRA_AUTO_BUFFER_NO_SMART_SYNC
// #define HYDRA_AUTO_BUFFER_SMART_SYNC_FORCE_TRANSFER
#include <memory>

#include "app.hpp"
#include "fs_quad_pass.hpp"

namespace neam
{
  class sample_app : public application
  {
    private:

    public:
      sample_app(const glm::uvec2 &window_size, const std::string &window_name)
       : application(window_size, window_name),
         test_pass(context),
         imgui_pass(imgui_ctx, context)
      {
        imgui_pass.setup_font_texture();
        rate_limit = 1. / 144.; // (144 fps max)
      }

    protected: // hooks

      void refresh_hook() override
      {
      }

      void reload_all_shaders()
      {
#ifndef HYDRA_NO_MESSAGES
          neam::cr::out().log("reloading all shaders...");
#endif

        context.device.wait_idle();
        context.shmgr.refresh();
        context.ppmgr.refresh();
      }

      void setup_hook() override
      {
        test_pass.setup(swapchain);
        imgui_pass.setup(swapchain);
      }

      void prepare_hook() override
      {
        test_pass.prepare();
        imgui_pass.prepare();
      }

      void submit_hook(hydra::vk::command_buffer_recorder& cbr, hydra::vk::framebuffer& fb) override
      {
        test_pass.submit(cbr, fb);
        imgui_pass.submit(cbr, fb);
      }

      void cleanup_hook() override
      {
        test_pass.cleanup();
        imgui_pass.cleanup();
      }

      virtual void render_loop_hook() override
      {
        {
          ImGui::Begin("Conf", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings );
          const uint32_t mti = 0;//neam::hydra::vk::device_memory::get_memory_type_index(context.device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniform_buffer.get_buffer().get_memory_requirements().memoryTypeBits);

          bool fs = window.is_fullscreen();
          ImGui::Checkbox("Fullscreen", &fs);
          if (fs != window.is_fullscreen())
            window.fullscreen(fs);

          ImGui::Checkbox("Unlock framerate", &unlock_framerate);
          double inv_rl = 1./rate_limit;
          ImGui::InputDouble("Rate Limit", &inv_rl);
          if (inv_rl <= 1)
            rate_limit = 1.;
          else
            rate_limit = 1./inv_rl;

          if (ImGui::Button("Reload all shaders"))
          {
            reload_all_shaders();
          }

          ImGui::Checkbox("Perform memory allocator stress test", &test_memory);

          if (unlock_framerate)
          {
            rate_limit = 0.0;
          }
          else if (rate_limit == 0)
          {
            rate_limit = 1. / 144.;
          }

          // test memory allocation to see sub-optimal things and bugs (we waste 6.8[1-3]ms per frame in average)
          if (test_memory)
          {
            uint32_t seed = rand();

            unsigned allocation_count = 0;
            unsigned free_count = 0;
            for (size_t itcount = 0; itcount < 4096&&false; ++itcount)
            {
              seed += (seed * seed) | 5u;

              // NOTE: I really don't care if the randomness is bad in that method. Really.
              //       for every intended purpose it's ok.
              switch (seed & 0x3F)
              {
                case 0: // free some memory
                {
                  while (memory_allocation_tests.size())
                  {
                    seed += (seed * seed) | 5u;

                    size_t idx = seed % memory_allocation_tests.size();
                    memory_allocation_tests[idx].free();
                    std::swap(memory_allocation_tests[idx], memory_allocation_tests.back());
                    memory_allocation_tests.pop_back();
                    ++free_count;
                    seed += (seed * seed) | 5u;
                    if ((seed & 0x3F) == 0x3F) break;
                  }
                  break;
                }
                default: // exit the loop
                {
                  seed += (seed * seed) | 5u;
                  const size_t sz = (seed & 0xFFF) + 1;
                  if (memory_allocation_tests.size() < /*16384 * 2*/1500 && context.allocator.get_allocation_count() < 500) // limit the number of allocations
                  {
                    memory_allocation_tests.emplace_back(context.allocator.allocate_memory(sz, 1, mti, neam::hydra::allocation_type::short_lived));
                    ++allocation_count;
                  }
                  break;
                }
              }
            }

            // smash the frame allocator:
            for (unsigned i = 0; i < 5000000; ++i)
            {
              seed += (seed * seed) | 5u;
              const size_t sz = (seed & 0xFFFF) + 1/*(rand() & 0xFFFF) + 1*/;
//                   if (context.allocator.get_allocation_count() < 2000) // limit the number of allocations
              {
                context.allocator.allocate_memory(sz, 1, mti, neam::hydra::allocation_type::single_frame);
                ++allocation_count;
              }
//                   break;
            }

            ImGui::Text("Allocation count (this frame): %5u", allocation_count);
            ImGui::Text("Free count (this frame):       %5u", free_count);
          }
          else if (memory_allocation_tests.size() > 0)
          {
            for (auto& it : memory_allocation_tests)
            {
              it.free();
            }
            memory_allocation_tests.clear();
          }
          ImGui::End();
        }
      }

    protected:
      fs_quad_pass test_pass;
      hydra::imgui::render_pass imgui_pass;

      // memory testing
      bool unlock_framerate = false;
      bool test_memory = false;
      std::deque<neam::hydra::memory_allocation> memory_allocation_tests;
  };
} // namespace neam

#endif // __N_94954772477713296_2016124446_SAMPLE_HPP__

