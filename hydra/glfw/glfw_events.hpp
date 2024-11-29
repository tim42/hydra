//
// file : glfw_events.hpp
// in : file:///home/tim/projects/hydra/hydra/glfw/glfw_events.hpp
//
// created by : Timothée Feuillet
// date: Sat Nov 05 2016 21:02:12 GMT-0400 (EDT)
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

#include <ntools/event.hpp>

#include "events/event.hpp"
#include "events/event_listener.hpp"

namespace neam::hydra::glfw
{
  class window;
}

namespace neam::hydra::events::glfw
{
  class manager
  {
    public:
      cr::raw_event<events::keyboard_listener*> on_keyboard_event;
      cr::raw_event<events::mouse_listener*> on_mouse_event;
      cr::raw_event<events::raw_keyboard_listener*> on_raw_keyboard_event;
      cr::raw_event<events::raw_mouse_listener*> on_raw_mouse_event;
      cr::raw_event<events::window_listener*> on_window_event;

    public:
      ~manager();

      // delete them all
      manager(const manager &) = delete;
      manager(manager &&) = delete;
      manager &operator = (const manager &) = delete;
      manager &operator = (manager &&) = delete;

      uint32_t get_event_count() const { return event_count; }
      void clear_event_count() { event_count = 0; }

    private: // window interface
      manager(neam::hydra::glfw::window &_win);
      void _register_hooks();

    private: // trampos
      static void _t_mouse_button(GLFWwindow *glfw_win, int button, int action, int modifiers);
      static void _t_mouse_wheel(GLFWwindow *glfw_win, double x, double y);
      static void _t_mouse_move(GLFWwindow *glfw_win, double x, double y);
      static void _t_mouse_entered(GLFWwindow* glfw_win, int entered);

      static void _t_key(GLFWwindow *glfw_win, int key, int scancode, int action, int modifiers);
      static void _t_unicode_input(GLFWwindow *glfw_win, unsigned int code);

      static void _t_window_pos(GLFWwindow *glfw_win, int x, int y);
      static void _t_window_size(GLFWwindow *glfw_win, int x, int y);
      static void _t_close(GLFWwindow *glfw_win);
      static void _t_refresh(GLFWwindow *glfw_win);
      static void _t_focus(GLFWwindow *glfw_win, int focus);
      static void _t_iconify(GLFWwindow *glfw_win, int iconify);
      static void _t_buffer_resize(GLFWwindow *glfw_win, int x, int y);

    private: // called by the trampos
      void _mouse_button(GLFWwindow *, int button, int action, int modifiers);
      void _mouse_wheel(GLFWwindow *, double x, double y);
      void _mouse_move(GLFWwindow *glfw_win, double x, double y);
      void _mouse_entered(GLFWwindow* glfw_win, bool entered);

      void _key(GLFWwindow *, int key, int scancode, int action, int modifiers);
      void _unicode_input(GLFWwindow *, unsigned int code);

      void _window_pos(GLFWwindow *, int x, int y);
      void _window_size(GLFWwindow *, int x, int y);
      void _close(GLFWwindow *);
      void _refresh(GLFWwindow *);
      void _focus(GLFWwindow *, int focus);
      void _iconify(GLFWwindow *, int iconify);
      void _buffer_resize(GLFWwindow *, int x, int y);

    private:
      neam::hydra::glfw::window& win;

      events::mouse_status last_mouse_status;
      events::keyboard_status last_keyboard_status;

      uint64_t event_count = 0;

      friend neam::hydra::glfw::window;
  };
} // namespace neam



// kate: indent-mode cstyle; indent-width 2; replace-tabs on; 


