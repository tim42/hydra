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

#define GLFW_EXPOSE_NATIVE_X11 // FIXME: wayland/win32 ?
#include <GLFW/glfw3native.h>
#include <X11/Xatom.h>

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
#ifdef GLFW_EXPOSE_NATIVE_X11
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
#endif
  }
}

