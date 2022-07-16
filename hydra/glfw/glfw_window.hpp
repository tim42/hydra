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

#include "../hydra_debug.hpp"
#include "../hydra_types.hpp"
#include "../hydra_logo.hpp"
#include "../vulkan/surface.hpp"
#include "../vulkan/swapchain.hpp"

namespace neam
{
  namespace hydra
  {
    namespace glfw
    {
      enum class cursor
      {
        arrow,
        ibeam,
        crosshair,
        pointing_hand,
        resize_ew,
        resize_ns,
        resize_nwse,
        resize_nesw,
        resize_all,
        not_allowed,
        _count,
      };

      /// \brief A GLFW window, as supported by hydra
      class window
      {
        public:
          window(hydra_context& hctx, glm::uvec2 window_size, const std::string& title = "HYDRA", std::initializer_list<std::pair<int, int>> w_hints = {})
           : window(window_size, title, w_hints)
          {
            _create_surface(hctx.instance);
            _get_surface().set_physical_device(hctx.device.get_physical_device());
          }
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
              check::on_vulkan_error::n_assert(false, "GLFW: glfwCreateWindow call failed");

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
              check::on_vulkan_error::n_assert(false, "GLFW: glfwCreateWindow call failed");

            select();
            _set_hydra_icon();
          }

          /// \brief Move constructor
          window(window &&o)
            : win(o.win), surface(o.surface), pres_id(o.pres_id)
            , is_window_fullscreen(o.is_window_fullscreen)
            , last_position(o.last_position)
            , last_size(o.last_size)
          {
            o.win = nullptr;
            o.surface = nullptr;
          }

          /// \brief destroy the window
          ~window()
          {
            if (surface)
              delete surface;
            if (win)
              glfwDestroyWindow(win);

            for (auto* it : cursors)
            {
              if (it != nullptr)
                glfwDestroyCursor(it);
            }
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

          void set_opacity(float alpha)
          {
            glfwSetWindowOpacity(win, alpha);
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

          /// \brief focus the window
          /// \note This function may only be called from the main thread. (from GLFW documentation)
          void focus() const
          {
            glfwFocusWindow(win);
          }

          bool is_focused() const
          {
            return glfwGetWindowAttrib(win, GLFW_FOCUSED) != 0;
          }
          bool is_iconified() const
          {
            return glfwGetWindowAttrib(win, GLFW_ICONIFIED) != 0;
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

          void fullscreen(bool fullscreen, unsigned /*monitor_idx*/ = 0)
          {
            if (fullscreen)
            {
              if (!is_window_fullscreen)
              {
                last_position = get_position();
                last_size = get_size();
              }

              GLFWmonitor* monitor = glfwGetPrimaryMonitor();
              const  GLFWvidmode* mode = glfwGetVideoMode(monitor);
              glfwSetWindowMonitor(win, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            else
            {
              glfwSetWindowMonitor(win, nullptr, last_position.x, last_position.y, last_size.x, last_size.y, GLFW_DONT_CARE);
            }
            is_window_fullscreen = fullscreen;
          }

          bool is_fullscreen() const
          {
            return is_window_fullscreen;
          }

          /// \brief Returns the window content scale (as set by the OS)
          /// May change over time
          glm::vec2 get_content_scale() const
          {
            glm::vec2 scale;
            glfwGetWindowContentScale(win, &scale.x, &scale.y);
            return scale;
          }

          const char* get_clipboard_text() const
          {
            return glfwGetClipboardString(win);
          }

          void set_clipboard_text(const char* text)
          {
            glfwSetClipboardString(win, text);
          }

        public: // cursor/mouse interraction
          void set_cursor(cursor c)
          {
            if (cursors[0] == nullptr)
              _init_cursors();
            glfwSetCursor(win, cursors[(uint32_t)c] ? cursors[(uint32_t)c] : cursors[(uint32_t)cursor::arrow]);
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
          }

          void disable_cursor(bool disable)
          {
            glfwSetInputMode(win, GLFW_CURSOR, disable ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
          }
          void hide_cursor(bool hide)
          {
            glfwSetInputMode(win, GLFW_CURSOR, hide ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
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
          bool _has_surface() const
          {
            return surface != nullptr;
          }

          /// \brief Return the surface of the window
          /// \see _have_surface()
          vk::surface &_get_surface()
          {
            return *surface;
          }

          /// \brief Return the surface of the window
          /// \see _have_surface()
          const vk::surface &_get_surface() const
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

          void _create_surface(vk::instance& instance)
          {
            VkSurfaceKHR surface;
            check::on_vulkan_error::n_assert_success(glfwCreateWindowSurface(instance._get_vk_instance(), _get_glfw_handle(), nullptr, &surface));
            _set_surface(vk::surface(instance, surface));
          }

          /// \brief Create a swapchain (filled with default parameters).
          /// That should be good enough for most applications
          vk::swapchain _create_swapchain(vk::device &dev)
          {
            check::debug::n_assert(_has_surface(), "Cannot create a swapchain without a surface. Call _create_surface first.");
            return vk::swapchain(dev, _get_surface(), get_size());
          }

          /// \brief Set the hydra icon (bonus function)
          /// \note icon_sz must be a power of 2
          /// \note glyph_count can't be more than 5, and more than 4 if icon_sz is 16
          void _set_hydra_icon(size_t icon_sz = 256, size_t glyph_count = 1)
          {
            uint8_t pixels[icon_sz * icon_sz * 4];
            set_icon(glm::uvec2(icon_sz, icon_sz), generate_rgba_logo(pixels, icon_sz, glyph_count));
          }

          void _set_hint(int hint, int value)
          {
            glfwWindowHint(hint, value);
          }

          void _set_hint(int hint, const char* const value)
          {
            glfwWindowHintString(hint, value);
          }

          enum class _window_type
          {
            normal,
            utility,
            menu,
            dialog,
            splash,
          };
          /// \brief Set the widow type
          /// \note require platform-specific code
          void _set_window_type(_window_type wt);

        private:
          void _init_cursors()
          {
            cursors[(uint32_t)cursor::arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
            cursors[(uint32_t)cursor::ibeam] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
            cursors[(uint32_t)cursor::crosshair] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
            cursors[(uint32_t)cursor::pointing_hand] = glfwCreateStandardCursor(GLFW_POINTING_HAND_CURSOR);
            cursors[(uint32_t)cursor::resize_ew] = glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
            cursors[(uint32_t)cursor::resize_ns] = glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
            cursors[(uint32_t)cursor::resize_nwse] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
            cursors[(uint32_t)cursor::resize_nesw] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
            cursors[(uint32_t)cursor::resize_all] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
            cursors[(uint32_t)cursor::not_allowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
          }
          
        private:
          GLFWwindow *win;
          neam::hydra::vk::surface *surface = nullptr;
          temp_queue_familly_id_t pres_id;

          bool is_window_fullscreen = false;
          glm::uvec2 last_position;
          glm::uvec2 last_size;

          GLFWcursor* cursors[(uint32_t)cursor::_count] = {nullptr};
      };
    } // namespace glfw
  } // namespace hydra
} // namespace neam

#endif // __N_448418003760327956_2472417823_GLFW_WINDOW_HPP__

