//
// file : glfw_window.hpp
// in : file:///home/tim/projects/hydra/hydra/glfw/glfw_window.hpp
//
// created by : Timothée Feuillet
// date: Wed Apr 27 2016 17:55:13 GMT+0200 (CEST)
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

#ifndef __N_448418003760327956_2472417823_GLFW_WINDOW_HPP__
#define __N_448418003760327956_2472417823_GLFW_WINDOW_HPP__

#include <string>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../hydra_exception.hpp"
#include "../hydra_types.hpp"
#include "../vulkan/surface.hpp"

namespace neam
{
  namespace hydra
  {
    namespace glfw
    {
      /// \brief A GLFW window, as supported by hydra
      class window
      {
        public:
          /// \brief create a non-fullscreen window
          /// \param window_size the size of the window in pixel.
          /// \param w_hints is a list of additional window creation hints as handled by GLFW
          ///       \code {{HINT_NAME, HINT_VALUE}, {HINT_NAME2, HINT_VALUE2}, ...} \endcode
          window(const glm::uvec2 &window_size, const std::string &title = "HYDRA", std::initializer_list<std::pair<int, int>> w_hints = std::initializer_list<std::pair<int, int>>())
            : win(nullptr)
          {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

            // init additional hints
            for (auto &it : w_hints)
              glfwWindowHint(it.first, it.second);

            if (!(win = glfwCreateWindow(window_size.x, window_size.y, title.data(), 0, 0)))
              throw neam::hydra::exception_tpl<window>("GLFW: glfwCreateWindow call failed", __FILE__, __LINE__);

            select();
            _set_hydra_icon();
          }

          /// \brief create a fullscreen window
          /// \note the window size is deduced from the resolution of the primary monitor
          /// \param w_hints is a list of additional window creation hints as handled by GLFW
          ///       \code {{HINT_NAME, HINT_VALUE}, {HINT_NAME2, HINT_VALUE2}, ...} \endcode
          window(const std::string &title = "HYDRA", std::initializer_list<std::pair<int, int>> w_hints = std::initializer_list<std::pair<int, int>>())
            : win(nullptr)
          {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

            // init additional hints
            for (auto &it : w_hints)
              glfwWindowHint(it.first, it.second);

            const GLFWvidmode *vmode = glfwGetVideoMode(glfwGetPrimaryMonitor());

            if (!(win = glfwCreateWindow(vmode->width, vmode->height, title.data(), glfwGetPrimaryMonitor(), 0)))
              throw neam::hydra::exception_tpl<window>("GLFW: glfwCreateWindow call failed", __FILE__, __LINE__);

            select();
            _set_hydra_icon();
          }

          /// \brief Move constructor
          window(window &&o)
            : win(o.win), surface(o.surface)
          {
            o.win = nullptr;
            o.surface = nullptr;
          }

          /// \brief destroy the window
          ~window()
          {
            if (win)
              glfwDestroyWindow(win);
          }

          /// \brief Makes the context of the specified window current for the calling thread.
          /// This function makes the context of the specified window current on the
          /// calling thread.  A context can only be made current on a single thread at
          /// a time and each thread can have only a single current context at a time.
          /// (from the GLFW documentation for glfwMakeContextCurrent())
          /// \see glfwMakeContextCurrent()
          void select() const
          {
            glfwMakeContextCurrent(win);
          }

          /// \brief resize the window
          /// \param window_size the new window size. MUST be an integer, NOT a fixed point size.
          void set_size(const glm::uvec2 &window_size)
          {
            glfwSetWindowSize(win, window_size.x, window_size.y);
          }

          /// \brief Return the current size of the window
          glm::uvec2 get_size() const
          {
            int ret[2];

            glfwGetWindowSize(win, ret, ret + 1);

            return glm::uvec2(ret[0], ret[1]);
          }

          /// \brief Return the size of the framebuffer associated with the window
          glm::uvec2 get_framebuffer_size() const
          {
            int ret[2];

            glfwGetFramebufferSize(win, ret, ret + 1);

            return glm::uvec2(ret[0], ret[1]);
          }

          /// \brief change the window position
          /// \param window_pos the new coordinates of the window (in pixel). MUST be an integer, NOT a fixed point position.
          void set_position(const glm::uvec2 &window_pos)
          {
            glfwSetWindowPos(win, window_pos.x, window_pos.y);
          }

          /// \brief return the position of the window (in screen coordinates -- pixels)
          glm::uvec2 get_position() const
          {
            int ret[2];

            glfwGetWindowPos(win, ret, ret + 1);

            return glm::uvec2(ret[0], ret[1]);
          }

          /// \brief change the title of the window
          /// \note This function may only be called from the main thread. (from GLFW documentation)
          void set_title(const std::string &title = "[ neam/yaggler")
          {
            glfwSetWindowTitle(win, title.data());
          }

          /// \brief Set the window icon
          /// \note This function may only be called from the main thread. (from GLFW documentation)
          /// \param icon_size The size of the icon (good sizes are 16x16, 32x32 and 48x48)
          /// \param icon_data RGBA data of the image, arranged left-to-right, top-to-bottom
          void set_icon(glm::uvec2 icon_size, uint8_t *icon_data)
          {
            GLFWimage img = {(int)icon_size.x, (int)icon_size.y, icon_data};
            glfwSetWindowIcon(win, 1, &img);
          }

          /// \brief minimize/iconify the window
          /// \note This function may only be called from the main thread. (from GLFW documentation)
          /// \see restore()
          void iconify() const
          {
            glfwIconifyWindow(win);
          }

          /// \brief hide the window (only for windows in windowed mode)
          /// \note This function may only be called from the main thread. (from GLFW documentation)
          /// \see show()
          void hide() const
          {
            glfwHideWindow(win);
          }

          /// \brief show the window (if already hidden)
          /// \note This function may only be called from the main thread. (from GLFW documentation)
          /// \see hide()
          void show() const
          {
            glfwShowWindow(win);
          }

          /// \brief restore the window if it was previously minimized
          /// \note This function may only be called from the main thread. (from GLFW documentation)
          /// \see iconify()
          void restore() const
          {
            glfwRestoreWindow(win);
          }

          /// \brief check the close flag of the window
          bool should_close() const
          {
            return glfwWindowShouldClose(win);
          }

        public: // advanced
          /// \brief return the GLFW handle of the current window
          /// \note for an advanced usage
          GLFWwindow *_get_glfw_handle()
          {
            return win;
          }

          /// \brief Return the id of the queue that support presenting
          temp_queue_familly_id_t _get_win_queue()
          {
            return pres_id;
          }

          /// \brief Set the id of the queue that support presenting
          void _set_win_queue(temp_queue_familly_id_t _pres_id)
          {
            pres_id = _pres_id;
          }

          /// \brief Check if the window has a surface
          bool _have_surface() const
          {
            return surface != nullptr;
          }

          /// \brief Return the surface of the window
          /// \see _have_surface()
          vk::surface &_get_surface()
          {
            return *surface;
          }

          /// \brief Set the surface of the window
          void _set_surface(vk::surface &&_surface)
          {
            if (surface)
              delete surface;
            surface = new vk::surface(std::move(_surface));
          }

          /// \brief Set the hydra icon (bonus function)
          void _set_hydra_icon()
          {
            // from the icon sample of GLFW
            const unsigned char icon_colors[][4] =
            {
              {  0,   0,   0, 255}, // black
              { 20,   0,   0, 127}, // red
              {  0,   0,   0,  80}, // black (a bit transparent)
              {  0,   0,   0, 100}, // black (a bit transparent)
            };

            const char* const hydra_icon[] =
            {
              "                                ",
              "                                ",
              "                                ",
              "                                ",
              "                                ",
              "                                ",
              "                                ",
              "00  00  00  00  0000    000000  ",
              "000000  000000  00  00  0000    ",
              "00  00      00  000000  00  00  ",
              "                                ",
              "                                ",
              "                                ",
              "                                ",
              "                                ",
              "                                ",
            };
            unsigned char pixels[16 * 16 * 4] = {0};
            unsigned char *target = pixels;
            for (int y = 0;  y < 16;  y++)
            {
              for (int x = 0;  x < 32;  x += 2)
              {
                if (hydra_icon[y][x] >= '0' && hydra_icon[y][x] <= '9')
                  memcpy(target, icon_colors[hydra_icon[y][x] - '0'], 4);

                target += 4;
              }
            }

            set_icon(glm::uvec2(16, 16), pixels);
          }

        private:
          GLFWwindow *win;
          neam::hydra::vk::surface *surface = nullptr;
          temp_queue_familly_id_t pres_id;
      };
    } // namespace glfw
  } // namespace hydra
} // namespace neam

#endif // __N_448418003760327956_2472417823_GLFW_WINDOW_HPP__

