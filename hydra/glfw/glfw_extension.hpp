//
// file : glfw_extension.hpp
// in : file:///home/tim/projects/hydra/hydra/glfw/glfw_extension.hpp
//
// created by : Timothée Feuillet
// date: Wed Apr 27 2016 17:54:04 GMT+0200 (CEST)
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

#ifndef __N_21511285133006622983_3175326442_GLFW_EXTENSION_HPP__
#define __N_21511285133006622983_3175326442_GLFW_EXTENSION_HPP__

#include <GLFW/glfw3.h>
#include "glfw_feature_requester.hpp"
#include "glfw_window.hpp"
#include "glfw_events.hpp"

#include  "../init/hydra_init_interface.hpp"
#include  "../hydra_debug.hpp"

namespace neam
{
  namespace hydra
  {
    namespace glfw
    {
      /// \brief The GLFW hydra extension
      class init_extension : public neam::hydra::init_interface
      {
        public:
          ~init_extension() {}

        public:
          /// \brief Create and initialize a new window
          /// \note You should call this method before creating a device, as the device
          ///       should be queried for a queue that supports presenting. If you already have a
          ///       device with a queue that supports presenting (you already have a window, ...)
          ///       please use the create_window_skip() method that will skip this part
          /// \see class window
          template<typename... WindowConstructorParameters>
          window create_window(neam::hydra::vk::instance &instance, WindowConstructorParameters&&... params)
          {
            window win(std::forward<WindowConstructorParameters>(params)...);
            _create_surface(win, instance);
            return win;
          }

          /// \brief Create and initialize a new window, but don't ask for a device queue
          /// \note You should only use this if you already created a device and that device
          ///       has a queue that supports presenting.
          /// \see class window
          template<typename... WindowConstructorParameters>
          window create_window_skip(neam::hydra::vk::instance &instance, WindowConstructorParameters&&... params)
          {
            window win(std::forward<WindowConstructorParameters>(params)...);
            _create_surface(win, instance, false);
            return win;
          }

        public: // advanced
          /// \brief Create a surface for a given window
          /// A surface is needed in order to draw something in the window/screen/...
          void _create_surface(window &win, neam::hydra::vk::instance &instance, bool ask_for_queue = true)
          {
            win._create_surface(instance);
          }

        public: // neam::hydra::init_interface
          void on_register() override { glfwInit(); }

          void pre_instance_creation() override {}
          void post_instance_creation(neam::hydra::vk::instance &) override {}

          void pre_device_creation(neam::hydra::vk::instance &) override {}
          void post_device_creation(neam::hydra::vk::device &dev) override
          {
//             for (window *w : requester.windows)
//               w->_get_surface().set_physical_device(dev.get_physical_device());
//             requester.windows.clear();
          }

//           neam::hydra::feature_requester_interface &get_feature_requester() override { return requester; }

        private:

        private:
      };
    } // namespace glfw
  } // namespace hydra
} // namespace neam

#endif // __N_21511285133006622983_3175326442_GLFW_EXTENSION_HPP__

