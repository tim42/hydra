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

#pragma once


#include <string>

#include <hydra_glm.hpp>


#include <ntools/raw_ptr.hpp>

#include "glfw.hpp"
#include "glfw_events.hpp"
#include "../hydra_debug.hpp"
#include "../hydra_types.hpp"
#include "../hydra_logo.hpp"
#include "../vulkan/vulkan.hpp"
#include "../vulkan/surface.hpp"
#include "../vulkan/swapchain.hpp"
#include "../engine/hydra_context.hpp"

namespace neam::hydra::glfw
{
  class glfw_module;

  /// \brief A GLFW window, as supported by hydra
  /// \warning In violation of hydra's good practice, the creation of the object is synchronous but initializatin may not be
  class window
  {
    public:
      enum class _window_type
      {
        normal,
        utility,
        menu,
        dialog,
        splash,
      };

    public:
      window(hydra_context& hctx, glfw_module* _glfw_mod, glm::uvec2 window_size, const std::string& title = "HYDRA", _window_type wt = _window_type::normal,  std::initializer_list<std::pair<int, int>> w_hints = {})
        : window(_glfw_mod, window_size, title, wt, w_hints)
      {
        execute_on_main_thread([this, &hctx]
        {
          _create_surface(hctx.instance);
          _get_surface().set_physical_device(hctx.device.get_physical_device());
          swapchain.emplace(hctx.device, _get_surface(), get_framebuffer_size());
        });
      }

      /// \brief create a non-fullscreen window
      /// \param window_size the size of the window in pixel.
      /// \param w_hints is a list of additional window creation hints as handled by GLFW
      ///       \code {{HINT_NAME, HINT_VALUE}, {HINT_NAME2, HINT_VALUE2}, ...} \endcode
      window(glfw_module* _glfw_mod, const glm::uvec2 &window_size, const std::string &title = "HYDRA", _window_type wt = _window_type::normal, std::initializer_list<std::pair<int, int>> w_hints = std::initializer_list<std::pair<int, int>>());

      /// \brief create a fullscreen window
      /// \note the window size is deduced from the resolution of the primary monitor
      /// \param w_hints is a list of additional window creation hints as handled by GLFW
      ///       \code {{HINT_NAME, HINT_VALUE}, {HINT_NAME2, HINT_VALUE2}, ...} \endcode
      window(glfw_module* _glfw_mod, const std::string &title = "HYDRA", std::initializer_list<std::pair<int, int>> w_hints = std::initializer_list<std::pair<int, int>>());

      /// \brief Move constructor
      window(window &&o) = default;

      /// \brief destroy the window
      ~window()
      {
        emgr.reset();
        swapchain.reset();
        surface.reset();

        if (win)
        {
          execute_on_main_thread([win = win.release()]
          {
            glfwDestroyWindow(win);
          });
        }
      }

      bool is_window_ready() const { return win != nullptr && !!swapchain; }

      /// \brief resize the window
      /// \param window_size the new window size. MUST be an integer, NOT a fixed point size.
      void set_size(const glm::uvec2 &size)
      {
        execute_on_main_thread([this, size]
        {
          window_size = size;
          glfwSetWindowSize(win, window_size.x, window_size.y);
          glfwGetWindowSize(win, (int*)&window_size.x, (int*)&window_size.y);
          glfwGetFramebufferSize(win, (int*)&window_framebuffer_size.x, (int*)&window_framebuffer_size.y);
        });
      }

      /// \brief Return the current size of the window
      glm::uvec2 get_size() const
      {
        return window_size;
      }

      /// \brief Return the size of the framebuffer associated with the window
      glm::uvec2 get_framebuffer_size() const
      {
        return window_framebuffer_size;
      }

      /// \brief change the window position
      /// \param window_pos the new coordinates of the window (in pixel). MUST be an integer, NOT a fixed point position.
      void set_position(const glm::uvec2 &window_pos)
      {
        execute_on_main_thread([this, window_pos]
        {
          window_position = window_pos;
          glfwSetWindowPos(win, window_pos.x, window_pos.y);
          glfwGetWindowPos(win, (int*)&window_position.x, (int*)&window_position.y);
        });
      }

      /// \brief return the position of the window (in screen coordinates -- pixels)
      glm::uvec2 get_position() const
      {
        return window_position;
      }

      /// \brief change the title of the window
      void set_title(const std::string &title = "[ neam/yaggler")
      {
        execute_on_main_thread([this, title]{glfwSetWindowTitle(win, title.c_str());});
      }

      /// \brief Set the window icon
      /// \param icon_size The size of the icon (good sizes are 16x16, 32x32 and 48x48)
      /// \param icon_data RGBA data of the image, arranged left-to-right, top-to-bottom
      void set_icon(glm::uvec2 icon_size, raw_data&& icon_data)
      {
        execute_on_main_thread([this, icon_size, icon_data = std::move(icon_data)]
        {
          GLFWimage img = {(int)icon_size.x, (int)icon_size.y, (uint8_t*)icon_data.data.get()};
          glfwSetWindowIcon(win, 1, &img);
        });
      }

      /// \brief focus the window
      void focus()
      {
        execute_on_main_thread([this]
        {
          glfwFocusWindow(win);
          window_is_focused = glfwGetWindowAttrib(win, GLFW_FOCUSED) != 0;
        });
      }

      bool is_focused() const
      {
        return window_is_focused;
      }
      bool is_iconified() const
      {
        return window_is_iconified;
      }

      /// \brief minimize/iconify the window
      /// \see restore()
      void iconify()
      {
        execute_on_main_thread([this]
        {
          glfwIconifyWindow(win);
          window_is_iconified = glfwGetWindowAttrib(win, GLFW_ICONIFIED) != 0;
        });
      }

      /// \brief hide the window (only for windows in windowed mode)
      /// \see show()
      void hide()
      {
        execute_on_main_thread([this]
        {
          glfwHideWindow(win);
          window_is_focused = glfwGetWindowAttrib(win, GLFW_FOCUSED) != 0;
          window_is_iconified = glfwGetWindowAttrib(win, GLFW_ICONIFIED) != 0;
        });
      }

      /// \brief show the window (if already hidden)
      /// \see hide()
      void show()
      {
        execute_on_main_thread([this]
        {
          glfwShowWindow(win);
          window_is_focused = glfwGetWindowAttrib(win, GLFW_FOCUSED) != 0;
          window_is_iconified = glfwGetWindowAttrib(win, GLFW_ICONIFIED) != 0;
        });
      }

      /// \brief restore the window if it was previously minimized
      /// \note This function may only be called from the main thread. (from GLFW documentation)
      /// \see iconify()
      void restore()
      {
        execute_on_main_thread([this]
        {
          glfwRestoreWindow(win);
          window_is_focused = glfwGetWindowAttrib(win, GLFW_FOCUSED) != 0;
          window_is_iconified = glfwGetWindowAttrib(win, GLFW_ICONIFIED) != 0;
        });
      }

      /// \brief check the close flag of the window
      bool should_close() const
      {
        return window_should_close;
      }

      /// \brief set the close flag of the window
      void should_close(bool flag)
      {
        execute_on_main_thread([this, flag]
        {
          glfwSetWindowShouldClose(win, flag ? 1 : 0);
          window_should_close = glfwWindowShouldClose(win);
        });
      }

      void fullscreen(bool fullscreen, unsigned /*monitor_idx*/ = 0)
      {
        execute_on_main_thread([this, fullscreen]
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

          initialize_window_state();
        });
      }

      bool is_fullscreen() const
      {
        return is_window_fullscreen;
      }

      /// \brief Returns the window content scale (as set by the OS)
      /// May change over time
      glm::vec2 get_content_scale() const
      {
        return window_content_scale;
      }

      const char* get_clipboard_text() const
      {
        assert_is_main_thread();
        return glfwGetClipboardString(win);
      }

      void set_clipboard_text(const char* text)
      {
        std::string t = text;
        execute_on_main_thread([this, t]{glfwSetClipboardString(win, t.c_str());});
      }

    public: // cursor/mouse interraction
      void set_cursor(cursor c);

      void disable_cursor(bool disable)
      {
        execute_on_main_thread([this, disable]{glfwSetInputMode(win, GLFW_CURSOR, disable ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);});
      }
      void hide_cursor(bool hide)
      {
        execute_on_main_thread([this, hide]{glfwSetInputMode(win, GLFW_CURSOR, hide ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);});
      }

    public: // advanced
      /// \brief return the GLFW handle of the current window
      /// \note for an advanced usage
      GLFWwindow* _get_glfw_handle()
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
        surface.reset(new vk::surface(std::move(_surface)));
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
      void _set_hydra_icon(uint32_t color = 0, size_t icon_sz = 256, size_t glyph_count = 1)
      {
        raw_data pixels = raw_data::allocate(icon_sz * icon_sz * 4);
        generate_rgba_logo((uint8_t*)pixels.data.get(), icon_sz, glyph_count, color);
        set_icon(glm::uvec2(icon_sz, icon_sz), std::move(pixels));
      }

      /// \brief Set the widow type
      /// \note require platform-specific code
      void _set_window_type(_window_type wt);

      events::glfw::manager& get_event_manager() { return *emgr; }
      vk::swapchain& get_swapchain()
      {
        check::debug::n_assert(!!swapchain, "Trying to get a swapchain when it's not been created yet");
        return *swapchain;
      }
      const vk::swapchain& get_swapchain() const
      {
        check::debug::n_assert(!!swapchain, "Trying to get a swapchain when it's not been created yet");
        return *swapchain;
      }

    private:
      void initialize_window_state();
      void execute_on_main_thread(threading::function_t&& fnc) const;
      void assert_is_main_thread() const;

      void _set_hint(int hint, int value)
      {
        assert_is_main_thread();
        glfwWindowHint(hint, value);
      }

      void _set_hint(int hint, const char* const value)
      {
        assert_is_main_thread();
        glfwWindowHintString(hint, value);
      }

    private:
      glfw_module* glfw_mod;
      cr::raw_ptr<GLFWwindow> win;
      std::unique_ptr<neam::hydra::vk::surface> surface;
      temp_queue_familly_id_t pres_id;

      std::unique_ptr<events::glfw::manager> emgr {new events::glfw::manager{*this}};
      std::optional<vk::swapchain> swapchain;

      // fullscreen state (switch between fullscreen and not fullscreen)
      bool is_window_fullscreen = false;
      glm::uvec2 last_position {0, 0};
      glm::uvec2 last_size {0, 0};

      // window state (updated by events)
      glm::uvec2 window_size = {0, 0};
      glm::uvec2 window_framebuffer_size {0, 0};
      glm::uvec2 window_position {0, 0};
      glm::vec2 window_content_scale {1, 1};
      bool window_is_focused = true;
      bool window_is_iconified = false;
      bool window_should_close = false;

      cursor last_set_cursor = cursor::arrow;

      friend events::glfw::manager;
  };
} // namespace neam



