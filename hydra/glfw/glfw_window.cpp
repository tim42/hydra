//
// created by : Timothée Feuillet
// date: 2022-7-9
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


#include <hydra/engine/hydra_context.hpp>
#include "glfw_window.hpp"
#include "glfw_engine_module.hpp"

#define GLFW_EXPOSE_NATIVE_X11 // FIXME: wayland/win32 ?
#include <GLFW/glfw3native.h>
#ifdef GLFW_EXPOSE_NATIVE_X11
  #include <X11/Xatom.h>
#endif

namespace neam::hydra::glfw
{
#ifdef GLFW_EXPOSE_NATIVE_X11
  // Action for EWMH client messages
#define _NET_WM_STATE_REMOVE        0
#define _NET_WM_STATE_ADD           1
#define _NET_WM_STATE_TOGGLE        2
  static void sendEventToWM(Display* disp, Window window, Atom type,
                            long a, long b, long c = 0, long d = 0, long e = 0)
  {
    auto screen = DefaultScreen(disp);
    auto root = RootWindow(disp, screen);
    XEvent event;
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.serial = 0;
    event.xclient.send_event = true;
    event.xclient.format = 32; // Data is 32-bit longs
    event.xclient.message_type = type;
    event.xclient.data.l[0] = a;
    event.xclient.data.l[1] = b;
    event.xclient.data.l[2] = c;
    event.xclient.data.l[3] = d;
    event.xclient.data.l[4] = e;

    XSendEvent(disp, root,
               False,
               SubstructureNotifyMask | SubstructureRedirectMask,
               &event);
  }
#endif

  void window::_set_window_type(_window_type wt)
  {
    if (glfwGetPlatform() != GLFW_PLATFORM_X11)
      return;
#ifdef GLFW_EXPOSE_NATIVE_X11
    execute_on_main_thread([this, wt]
    {
      Display* x11disp = glfwGetX11Display();
      Window x11win = glfwGetX11Window(win);

      const Atom NET_WM_WINDOW_TYPE = XInternAtom(x11disp, "_NET_WM_WINDOW_TYPE", false);

      const Atom _NET_WM_WINDOW_TYPE_NORMAL = XInternAtom(x11disp, "_NET_WM_WINDOW_TYPE_NORMAL", false);
      const Atom _NET_WM_WINDOW_TYPE_DIALOG = XInternAtom(x11disp, "_NET_WM_WINDOW_TYPE_DIALOG", false);
      const Atom _NET_WM_WINDOW_TYPE_MENU = XInternAtom(x11disp, "_NET_WM_WINDOW_TYPE_MENU", false);
      const Atom _NET_WM_WINDOW_TYPE_UTILITY = XInternAtom(x11disp, "_NET_WM_WINDOW_TYPE_UTILITY", false);
      const Atom _NET_WM_WINDOW_TYPE_SPLASH = XInternAtom(x11disp, "_NET_WM_WINDOW_TYPE_SPLASH", false);
      Atom atom[2] = {_NET_WM_WINDOW_TYPE_NORMAL, _NET_WM_WINDOW_TYPE_NORMAL};
      switch (wt)
      {
        default:
        case _window_type::normal: atom[0] = _NET_WM_WINDOW_TYPE_NORMAL;
        break;
        case _window_type::dialog: atom[0] = _NET_WM_WINDOW_TYPE_DIALOG;
        break;
        case _window_type::menu: atom[0] = _NET_WM_WINDOW_TYPE_MENU;
        break;
        case _window_type::utility: atom[0] = _NET_WM_WINDOW_TYPE_UTILITY;
        break;
        case _window_type::splash: atom[0] = _NET_WM_WINDOW_TYPE_SPLASH;
        break;
      }

      XChangeProperty(x11disp,
                      x11win,
                      NET_WM_WINDOW_TYPE,
                      XA_ATOM, 32, PropModeReplace,
                      (unsigned char*)atom, 2);
    });
#endif
  }

  /// \brief create a non-fullscreen window
  /// \param window_size the size of the window in pixel.
  /// \param w_hints is a list of additional window creation hints as handled by GLFW
  ///       \code {{HINT_NAME, HINT_VALUE}, {HINT_NAME2, HINT_VALUE2}, ...} \endcode
  window::window(glfw_module* _glfw_mod, const glm::uvec2 &_window_size, const std::string &title, _window_type wt, std::initializer_list<std::pair<int, int>> w_hints)
    : glfw_mod(_glfw_mod), win(nullptr)
  {
    window_size = _window_size;
    window_framebuffer_size = _window_size;
    execute_on_main_thread([this, title, wt, w_hints]
    {
      assert_is_main_thread();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

      // init additional hints
      for (auto &it : w_hints)
        glfwWindowHint(it.first, it.second);

      if (!(win = glfwCreateWindow(window_size.x, window_size.y, title.c_str(), 0, 0)))
      {
        const char* er = nullptr;
        glfwGetError(&er);
        check::on_vulkan_error::n_assert(false, "GLFW: glfwCreateWindow call failed: {}", (er == nullptr ? "no error" : er));
      }

      _set_window_type(wt);

      initialize_window_state();
      set_size((glm::uvec2)((glm::vec2)window_size * window_content_scale));
      emgr->_register_hooks();
      _set_hydra_icon();
    });
  }

  /// \brief create a fullscreen window
  /// \note the window size is deduced from the resolution of the primary monitor
  /// \param w_hints is a list of additional window creation hints as handled by GLFW
  ///       \code {{HINT_NAME, HINT_VALUE}, {HINT_NAME2, HINT_VALUE2}, ...} \endcode
  window::window(glfw_module* _glfw_mod, const std::string &title, std::initializer_list<std::pair<int, int>> w_hints)
    : glfw_mod(_glfw_mod), win(nullptr)
  {
    execute_on_main_thread([this, title, w_hints]
    {
      assert_is_main_thread();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

      // init additional hints
      for (auto &it : w_hints)
        glfwWindowHint(it.first, it.second);

      const GLFWvidmode *vmode = glfwGetVideoMode(glfwGetPrimaryMonitor());

      if (!(win = glfwCreateWindow(vmode->width, vmode->height, title.c_str(), glfwGetPrimaryMonitor(), 0)))
      {
        const char* er = nullptr;
        glfwGetError(&er);
        check::on_vulkan_error::n_assert(false, "GLFW: glfwCreateWindow call failed: {}", (er == nullptr ? "no error" : er));
      }

      initialize_window_state();
      emgr->_register_hooks();
      _set_hydra_icon();
    });
  }

  void window::set_cursor(cursor c)
  {
    check::debug::n_assert((uint32_t)c < (uint32_t)cursor::_count, "specifying an out-of-range cursor");
    execute_on_main_thread([this, c] mutable
    {
      if (!check::debug::n_check(win != nullptr || c == cursor::arrow, "Trying to set a cursor on a window that doesn't exist yet"))
        return;
      if (win == nullptr)
        return;
      if (glfw_mod->cursors[(uint32_t)c] == nullptr)
        c = cursor::arrow;
      if (c == last_set_cursor)
        return;
      if (!check::debug::n_check(glfw_mod->cursors[(uint32_t)cursor::arrow] != nullptr, "arrow cursor is not defined yet, which is an error"))
        return;
      last_set_cursor = c;
      glfwSetCursor(win, glfw_mod->cursors[(uint32_t)c]);
      glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    });
  }

  window::~window()
  {
    emgr.reset();
    glfw_mod->hctx->dfe.defer_destruction(std::move(surface), std::move(swapchain));
    swapchain.reset();
    surface.reset();

    if (win)
    {
      glfw_mod->hctx->dfe.defer([win = win.release(), glfw_mod = glfw_mod]
      {
        glfw_mod->execute_on_main_thread([win]
        {
          glfwDestroyWindow(win);
        });
      });
    }
  }

  void window::initialize_window_state()
  {
    assert_is_main_thread();

    int vct[2];

    glfwGetWindowSize(win, vct, vct + 1);
    window_size = { vct[0], vct[1] };

    glfwGetFramebufferSize(win, vct, vct + 1);
    window_framebuffer_size = { vct[0], vct[1] };

    glfwGetWindowPos(win, vct, vct + 1);
    window_position = { vct[0], vct[1] };

    glfwGetWindowContentScale(win, &window_content_scale.x, &window_content_scale.y);

    window_is_focused = glfwGetWindowAttrib(win, GLFW_FOCUSED) != 0;
    window_is_iconified = glfwGetWindowAttrib(win, GLFW_ICONIFIED) != 0;
    window_should_close = glfwWindowShouldClose(win);
  }

  void window::execute_on_main_thread(threading::function_t&& fnc) const
  {
    glfw_mod->execute_on_main_thread(std::move(fnc));
  }

  void window::assert_is_main_thread() const
  {
    glfw_mod->assert_is_main_thread();
  }
}

