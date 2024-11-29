//
// file : event_listener.hpp
// in : file:///home/tim/projects/yaggler/yaggler/bleunw/events/event_listener.hpp
//
// created by : Timothée Feuillet on linux-coincoin.tim
// date: 19/02/2014 21:38:49
//
//
// Copyright (C) 2014 Timothée Feuillet
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#pragma once
# define __N_955277827674830425_113778684__EVENT_LISTENER_HPP__

#include "event.hpp"

namespace neam::hydra::events
{
  struct raw_mouse_listener
  {
    virtual ~raw_mouse_listener() {}

    virtual void on_mouse_button(int /*button*/, int /*action*/, int /*modifiers*/) {}
    virtual void on_mouse_wheel(double /*x*/, double /*y*/) {}
    virtual void on_mouse_move(double /*x*/, double /*y*/) {}
    virtual void on_mouse_entered(bool /*entered*/) {}
  };

  struct raw_keyboard_listener
  {
    virtual ~raw_keyboard_listener() {}

    virtual void on_key(int /*key*/, int /*scancode*/, int /*action*/, int /*modifiers*/) {}
    virtual void on_unicode_input(unsigned int /*code*/) {}
  };

  struct mouse_listener
  {
    virtual ~mouse_listener() {}

    virtual void mouse_moved(const mouse_status &/*ms*/) {}
    virtual void mouse_scrolled(const mouse_status &/*ms*/) {}
    virtual void button_pressed(const mouse_status &/*ms*/, mouse_buttons::mouse_buttons /*mb*/) {}
    virtual void button_released(const mouse_status &/*ms*/, mouse_buttons::mouse_buttons /*mb*/) {}
    virtual void on_mouse_entered(const mouse_status &/*ms*/, bool /*entered*/) {}
  };

  struct keyboard_listener
  {
    virtual ~keyboard_listener() {}

    virtual void key_pressed(const keyboard_status &/*ks*/, key_code::key_code /*kc*/) {}
    virtual void key_released(const keyboard_status &/*ks*/, key_code::key_code /*kc*/) {}

    virtual void on_input(const keyboard_status &, uint32_t) {} // for (unicode) text input
  };

  struct window_listener
  {
    virtual ~window_listener() {}

    virtual void window_closed() {}
    virtual void window_focused(bool /*focused*/) {}
    virtual void window_iconified(bool /*iconified*/) {}

    virtual void window_resized(const glm::vec2 &/*new_size*/) {}
    virtual void framebuffer_resized(const glm::vec2 &/*new_size*/) {}

    virtual void window_content_refresh() {}
    virtual void window_position_changed(const glm::vec2 &/*new_pos*/) {}
  };

  struct listener : public mouse_listener, public keyboard_listener, public window_listener
  {
    virtual ~listener() {}
  };
} // namespace neam::hydra::events



// kate: indent-mode cstyle; indent-width 2; replace-tabs on; 

