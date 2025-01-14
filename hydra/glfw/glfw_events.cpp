//
// created by : Timothée Feuillet
// date: 2023-7-15
//
//
// Copyright (c) 2023 Timothée Feuillet
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

#include "glfw_events.hpp"
#include "glfw_window.hpp"

namespace neam::hydra::events::glfw
{
  manager::manager(neam::hydra::glfw::window &_win) : win(_win)
  {
  }

  manager::~manager()
  {
    glfwSetWindowUserPointer(win._get_glfw_handle(), nullptr);

    // unregister callbacks
    glfwSetKeyCallback(win._get_glfw_handle(), nullptr);
    glfwSetCharCallback(win._get_glfw_handle(), nullptr);
    glfwSetMouseButtonCallback(win._get_glfw_handle(), nullptr);
    glfwSetCursorPosCallback(win._get_glfw_handle(), nullptr);
    glfwSetScrollCallback(win._get_glfw_handle(), nullptr);

    glfwSetWindowSizeCallback(win._get_glfw_handle(), nullptr);
    glfwSetWindowFocusCallback(win._get_glfw_handle(), nullptr);
    glfwSetWindowIconifyCallback(win._get_glfw_handle(), nullptr);
    glfwSetWindowPosCallback(win._get_glfw_handle(), nullptr);
    glfwSetWindowRefreshCallback(win._get_glfw_handle(), nullptr);
    glfwSetWindowCloseCallback(win._get_glfw_handle(), nullptr);
    glfwSetFramebufferSizeCallback(win._get_glfw_handle(), nullptr);
  }

  void manager::_register_hooks()
  {
    // register glfw callbacks and user pointer
    glfwSetWindowUserPointer(win._get_glfw_handle(), this);

    glfwSetKeyCallback(win._get_glfw_handle(), &manager::_t_key);
    glfwSetCharCallback(win._get_glfw_handle(), &manager::_t_unicode_input);
    glfwSetMouseButtonCallback(win._get_glfw_handle(), &manager::_t_mouse_button);
    glfwSetCursorPosCallback(win._get_glfw_handle(), &manager::_t_mouse_move);
    glfwSetScrollCallback(win._get_glfw_handle(), &manager::_t_mouse_wheel);

    glfwSetWindowSizeCallback(win._get_glfw_handle(), &manager::_t_window_size);
    glfwSetWindowFocusCallback(win._get_glfw_handle(), &manager::_t_focus);
    glfwSetWindowIconifyCallback(win._get_glfw_handle(), &manager::_t_iconify);
    glfwSetWindowPosCallback(win._get_glfw_handle(), &manager::_t_window_pos);
    glfwSetWindowRefreshCallback(win._get_glfw_handle(), &manager::_t_refresh);
    glfwSetWindowCloseCallback(win._get_glfw_handle(), &manager::_t_close);
    glfwSetFramebufferSizeCallback(win._get_glfw_handle(), &manager::_t_buffer_resize);

    // init the mouse state
    last_mouse_status.buttons = events::mouse_buttons::none;
    double _cpos[2];
    glfwGetCursorPos(win._get_glfw_handle(), _cpos, _cpos + 1);
    last_mouse_status.position.x = _cpos[0];
    last_mouse_status.position.y = _cpos[1];

    int _wsz[2];
    glfwGetWindowSize(win._get_glfw_handle(), _wsz, _wsz + 1);
    glm::vec2 wsz(_wsz[0], _wsz[1]);

    last_mouse_status.normalized_position = last_mouse_status.position / wsz;
    last_mouse_status.modifiers = events::modifier_keys::none;
    last_mouse_status.wheel = glm::vec2(0, 0);
  }

  void manager::_t_mouse_button(GLFWwindow *glfw_win, int button, int action, int modifiers)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_mouse_button(glfw_win, button, action, modifiers);
  }

  void manager::_t_mouse_wheel(GLFWwindow *glfw_win, double x, double y)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_mouse_wheel(glfw_win, x, y);
  }

  void manager::_t_mouse_move(GLFWwindow *glfw_win, double x, double y)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_mouse_move(glfw_win, x, y);
  }

  void manager::_t_mouse_entered(GLFWwindow* glfw_win, int entered)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_mouse_entered(glfw_win, entered != 0);
  }

  void manager::_t_key(GLFWwindow *glfw_win, int key, int scancode, int action, int modifiers)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_key(glfw_win, key, scancode, action, modifiers);
  }

  void manager::_t_unicode_input(GLFWwindow *glfw_win, unsigned int code)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_unicode_input(glfw_win, code);
  }

  void manager::_t_window_pos(GLFWwindow *glfw_win, int x, int y)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_window_pos(glfw_win, x, y);
  }

  void manager::_t_window_size(GLFWwindow *glfw_win, int x, int y)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_window_size(glfw_win, x, y);
  }

  void manager::_t_close(GLFWwindow *glfw_win)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_close(glfw_win);
  }

  void manager::_t_refresh(GLFWwindow *glfw_win)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_refresh(glfw_win);
  }

  void manager::_t_focus(GLFWwindow *glfw_win, int focus)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_focus(glfw_win, focus);
  }

  void manager::_t_iconify(GLFWwindow *glfw_win, int iconify)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_iconify(glfw_win, iconify);
  }

  void manager::_t_buffer_resize(GLFWwindow *glfw_win, int x, int y)
  {
    manager *emgr = reinterpret_cast<manager *>(glfwGetWindowUserPointer(glfw_win));
    emgr->_buffer_resize(glfw_win, x, y);
  }

  void manager::_mouse_button(GLFWwindow *, int button, int action, int modifiers)
  {
    ++event_count;
    on_raw_mouse_event.for_each([&](events::raw_mouse_listener* ml)
    {
      ml->on_mouse_button(button, action, modifiers);
    });

    events::mouse_status current_mouse_status = last_mouse_status;

    current_mouse_status.delta.normalized_position = glm::vec2(0, 0);
    current_mouse_status.delta.position = glm::vec2(0, 0);
    current_mouse_status.delta.wheel = glm::vec2(0, 0);
    current_mouse_status.delta.active_buttons = events::mouse_buttons::none;
    if (action == GLFW_PRESS)
      current_mouse_status.delta.active_buttons = static_cast<events::mouse_buttons::mouse_buttons>(1 << button);


    current_mouse_status.modifiers = static_cast<events::modifier_keys::modifier_keys>(modifiers);
    if (action == GLFW_PRESS)
      current_mouse_status.buttons =  static_cast<events::mouse_buttons::mouse_buttons>(current_mouse_status.buttons | (1 << button));
    else if (action == GLFW_RELEASE)
      current_mouse_status.buttons = static_cast<events::mouse_buttons::mouse_buttons>(current_mouse_status.buttons & ~(1 << button));

    on_mouse_event.for_each([&](events::mouse_listener* ml)
    {
      if (action == GLFW_PRESS)
        ml->button_pressed(current_mouse_status, current_mouse_status.delta.active_buttons);
      else if (action == GLFW_RELEASE)
        ml->button_released(current_mouse_status, static_cast<events::mouse_buttons::mouse_buttons>(1 << button));
    });

    last_mouse_status = current_mouse_status;
  }

  void manager::_mouse_wheel(GLFWwindow *, double x, double y)
  {
    ++event_count;
    on_raw_mouse_event.for_each([&](events::raw_mouse_listener * ml)
    {
      ml->on_mouse_wheel(x, y);
    });

    events::mouse_status current_mouse_status = last_mouse_status;

    current_mouse_status.delta.normalized_position = glm::vec2(0, 0);
    current_mouse_status.delta.position = glm::vec2(0, 0);
    current_mouse_status.delta.wheel = glm::vec2(x, y);
    current_mouse_status.delta.active_buttons = events::mouse_buttons::none;


    current_mouse_status.wheel += glm::vec2(x, y);

    on_mouse_event.for_each([&](events::mouse_listener* ml)
    {
      ml->mouse_scrolled(current_mouse_status);
    });

    last_mouse_status = current_mouse_status;
  }

  void manager::_mouse_move(GLFWwindow *glfw_win, double x, double y)
  {
    ++event_count;
    on_raw_mouse_event.for_each([&](events::raw_mouse_listener* ml)
    {
      ml->on_mouse_move(x, y);
    });

    int _wsz[2];
    glfwGetWindowSize(glfw_win, _wsz, _wsz + 1);
    glm::vec2 wsz(_wsz[0], _wsz[1]);

    events::mouse_status current_mouse_status = last_mouse_status;

    current_mouse_status.delta.normalized_position = last_mouse_status.normalized_position - (glm::vec2(x, y) / wsz);
    current_mouse_status.delta.position = last_mouse_status.position - glm::vec2(x, y);
    current_mouse_status.delta.wheel = glm::vec2(0, 0);
    current_mouse_status.delta.active_buttons = events::mouse_buttons::none;


    current_mouse_status.normalized_position = (glm::vec2(x, y) / wsz);
    current_mouse_status.position = glm::vec2(x, y);

    on_mouse_event.for_each([&](events::mouse_listener* ml)
    {
      ml->mouse_moved(current_mouse_status);
    });

    last_mouse_status = current_mouse_status;
  }

  void manager::_mouse_entered(GLFWwindow* glfw_win, bool entered)
  {
    ++event_count;
    on_raw_mouse_event.for_each([&](events::raw_mouse_listener* ml)
    {
      ml->on_mouse_entered(entered);
    });

    on_mouse_event.for_each([&](events::mouse_listener* ml)
    {
      ml->on_mouse_entered(last_mouse_status, entered);
    });
  }


  void manager::_key(GLFWwindow *, int key, int scancode, int action, int modifiers)
  {
    ++event_count;
    on_raw_keyboard_event.for_each([&](events::raw_keyboard_listener* kl)
    {
      kl->on_key(key, scancode, action, modifiers);
    });

    events::keyboard_status ks;

    ks.modifiers = static_cast<events::modifier_keys::modifier_keys>(modifiers);
    last_mouse_status.modifiers = ks.modifiers;

    events::key_code::key_code key_code = static_cast<events::key_code::key_code>(key);

    // mouse modifers
    switch (key_code)
    {
      case events::key_code::left_control:
      case events::key_code::right_control:
        if (action == GLFW_PRESS)
          last_mouse_status.modifiers = static_cast<events::modifier_keys::modifier_keys>(last_mouse_status.modifiers | events::modifier_keys::modifier_keys::control);
        else
          last_mouse_status.modifiers = static_cast<events::modifier_keys::modifier_keys>(last_mouse_status.modifiers & ~events::modifier_keys::modifier_keys::control);
        break;
      case events::key_code::left_alt:
      case events::key_code::right_alt:
        if (action == GLFW_PRESS)
          last_mouse_status.modifiers = static_cast<events::modifier_keys::modifier_keys>(last_mouse_status.modifiers | events::modifier_keys::modifier_keys::alt);
        else
          last_mouse_status.modifiers = static_cast<events::modifier_keys::modifier_keys>(last_mouse_status.modifiers & ~events::modifier_keys::modifier_keys::alt);
        break;
      case events::key_code::left_shift:
      case events::key_code::right_shift:
        if (action == GLFW_PRESS)
          last_mouse_status.modifiers = static_cast<events::modifier_keys::modifier_keys>(last_mouse_status.modifiers | events::modifier_keys::modifier_keys::shift);
        else
          last_mouse_status.modifiers = static_cast<events::modifier_keys::modifier_keys>(last_mouse_status.modifiers & ~events::modifier_keys::modifier_keys::shift);
        break;
      case events::key_code::left_super:
      case events::key_code::right_super:
        if (action == GLFW_PRESS)
          last_mouse_status.modifiers = static_cast<events::modifier_keys::modifier_keys>(last_mouse_status.modifiers | events::modifier_keys::modifier_keys::super);
        else
          last_mouse_status.modifiers = static_cast<events::modifier_keys::modifier_keys>(last_mouse_status.modifiers & ~events::modifier_keys::modifier_keys::super);
        break;
      default: // this makes gcc happy.
        break;
    }

    on_keyboard_event.for_each([&](events::keyboard_listener* kl)
    {
      if (action == GLFW_PRESS)
        kl->key_pressed(ks, key_code);
      else if (action == GLFW_RELEASE)
        kl->key_released(ks, key_code);
    });
    last_keyboard_status = ks;
  }

  void manager::_unicode_input(GLFWwindow *, unsigned int code)
  {
    ++event_count;
    on_raw_keyboard_event.for_each([&](events::raw_keyboard_listener* kl)
    {
      kl->on_unicode_input(code);
    });

    on_keyboard_event.for_each([&](events::keyboard_listener* kl)
    {
      kl->on_input(last_keyboard_status, code);
    });
  }

  void manager::_window_pos(GLFWwindow *, int x, int y)
  {
    ++event_count;
    win.window_position = {x, y};

    glm::vec2 v(x, y);
    on_window_event.for_each([&](events::window_listener* wl)
    {
      wl->window_position_changed(v);
    });
  }

  void manager::_window_size(GLFWwindow *, int x, int y)
  {
    ++event_count;
    win.window_size = {x, y};

    glm::vec2 v(x, y);
    on_window_event.for_each([&](events::window_listener* wl)
    {
      wl->window_resized(v);
    });
  }

  void manager::_close(GLFWwindow *)
  {
    ++event_count;
    win.window_should_close = true;

    on_window_event.for_each([&](events::window_listener* wl)
    {
      wl->window_closed();
    });
  }

  void manager::_refresh(GLFWwindow *)
  {
    ++event_count;
    on_window_event.for_each([&](events::window_listener* wl)
    {
      wl->window_content_refresh();
    });
  }

  void manager::_focus(GLFWwindow *, int focus)
  {
    ++event_count;
    win.window_is_focused = focus;

    on_window_event.for_each([&](events::window_listener* wl)
    {
      wl->window_focused(focus);
    });
  }

  void manager::_iconify(GLFWwindow *, int iconify)
  {
    ++event_count;
    win.window_is_iconified = iconify;

    on_window_event.for_each([&](events::window_listener* wl)
    {
      wl->window_iconified(iconify);
    });
  }

  void manager::_buffer_resize(GLFWwindow *, int x, int y)
  {
    ++event_count;
    win.window_framebuffer_size = {x, y};

    glm::vec2 v(x, y);
    on_window_event.for_each([&](events::window_listener* wl)
    {
      wl->framebuffer_resized(v);
    });
  }
}

