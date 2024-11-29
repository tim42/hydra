//
// file : event.hpp
// in : file:///home/tim/projects/yaggler/yaggler/bleunw/events/event.hpp
//
// created by : Timothée Feuillet on linux-coincoin.tim
// date: 19/02/2014 21:50:34
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
# define __N_355671688686967676_2128128696__EVENT_HPP__

#include <hydra_glm.hpp>


#include "../glfw.hpp"

namespace neam::hydra::events
{
  // simple glfw wrapper
  namespace modifier_keys
  {
    enum modifier_keys : int
    {
      none = 0,
      shift = GLFW_MOD_SHIFT,
      control = GLFW_MOD_CONTROL,
      alt = GLFW_MOD_ALT,
      super = GLFW_MOD_SUPER,
      meta = GLFW_MOD_SUPER,
      capslock = GLFW_MOD_CAPS_LOCK,
      numlock = GLFW_MOD_NUM_LOCK,
    };
  } // namespace modifier_keys

  // simple glfw wrapper
  namespace mouse_buttons
  {
    enum mouse_buttons : int
    {
      none = 0,

      // you can also use its numeric value (1 << (button_number - 1))
      button_1 = 1 << GLFW_MOUSE_BUTTON_1,
      button_2 = 1 << GLFW_MOUSE_BUTTON_2,
      button_3 = 1 << GLFW_MOUSE_BUTTON_3,
      button_4 = 1 << GLFW_MOUSE_BUTTON_4,
      button_5 = 1 << GLFW_MOUSE_BUTTON_5,
      button_6 = 1 << GLFW_MOUSE_BUTTON_6,
      button_7 = 1 << GLFW_MOUSE_BUTTON_7,
      button_8 = 1 << GLFW_MOUSE_BUTTON_8,
      button_left = button_1,
      button_right = button_2,
      button_middle = button_3,
    };
  } // namespace mouse_buttons


  struct mouse_status
  {
    glm::vec2 normalized_position; // position in [0, 1]
    glm::vec2 position; // pixel position

    glm::vec2 wheel;

    mouse_buttons::mouse_buttons buttons = mouse_buttons::none;

    modifier_keys::modifier_keys modifiers;

    struct
    {
      glm::vec2 normalized_position; // position in [0, 1]
      glm::vec2 position; // pixel position

      glm::vec2 wheel;

      mouse_buttons::mouse_buttons active_buttons = mouse_buttons::none;
    } delta;
  };

  namespace key_code
  {
    // stolen from GLFW
    enum key_code : int
    {
      unknown       = GLFW_KEY_UNKNOWN,

      space         = GLFW_KEY_SPACE,
      apostrophe    = GLFW_KEY_APOSTROPHE,
      comma         = GLFW_KEY_COMMA,
      minus         = GLFW_KEY_MINUS,
      period        = GLFW_KEY_PERIOD,
      slash         = GLFW_KEY_SLASH,
      key_0         = GLFW_KEY_0,
      key_1         = GLFW_KEY_1,
      key_2         = GLFW_KEY_2,
      key_3         = GLFW_KEY_3,
      key_4         = GLFW_KEY_4,
      key_5         = GLFW_KEY_5,
      key_6         = GLFW_KEY_6,
      key_7         = GLFW_KEY_7,
      key_8         = GLFW_KEY_8,
      key_9         = GLFW_KEY_9,
      semicolon     = GLFW_KEY_SEMICOLON,
      equal         = GLFW_KEY_EQUAL,
      a             = GLFW_KEY_A,
      b             = GLFW_KEY_B,
      c             = GLFW_KEY_C,
      d             = GLFW_KEY_D,
      e             = GLFW_KEY_E,
      f             = GLFW_KEY_F,
      g             = GLFW_KEY_G,
      h             = GLFW_KEY_H,
      i             = GLFW_KEY_I,
      j             = GLFW_KEY_J,
      k             = GLFW_KEY_K,
      l             = GLFW_KEY_L,
      m             = GLFW_KEY_M,
      n             = GLFW_KEY_N,
      o             = GLFW_KEY_O,
      p             = GLFW_KEY_P,
      q             = GLFW_KEY_Q,
      r             = GLFW_KEY_R,
      s             = GLFW_KEY_S,
      t             = GLFW_KEY_T,
      u             = GLFW_KEY_U,
      v             = GLFW_KEY_V,
      w             = GLFW_KEY_W,
      x             = GLFW_KEY_X,
      y             = GLFW_KEY_Y,
      z             = GLFW_KEY_Z,
      left_bracket  = GLFW_KEY_LEFT_BRACKET,
      backslash     = GLFW_KEY_BACKSLASH,
      right_bracket = GLFW_KEY_RIGHT_BRACKET,
      grave_accent  = GLFW_KEY_GRAVE_ACCENT,
      world_1       = GLFW_KEY_WORLD_1,
      world_2       = GLFW_KEY_WORLD_2,

      escape        = GLFW_KEY_ESCAPE,
      enter         = GLFW_KEY_ENTER,
      tab           = GLFW_KEY_TAB,
      backspace     = GLFW_KEY_BACKSPACE,
      insert        = GLFW_KEY_INSERT,
      del           = GLFW_KEY_DELETE,
      right         = GLFW_KEY_RIGHT,
      left          = GLFW_KEY_LEFT,
      down          = GLFW_KEY_DOWN,
      up            = GLFW_KEY_UP,
      page_up       = GLFW_KEY_PAGE_UP,
      page_down     = GLFW_KEY_PAGE_DOWN,
      home          = GLFW_KEY_HOME,
      end           = GLFW_KEY_END,
      caps_lock     = GLFW_KEY_CAPS_LOCK,
      scroll_lock   = GLFW_KEY_SCROLL_LOCK,
      num_lock      = GLFW_KEY_NUM_LOCK,
      print_screen  = GLFW_KEY_PRINT_SCREEN,
      pause         = GLFW_KEY_PAUSE,
      F1            = GLFW_KEY_F1,
      F2            = GLFW_KEY_F2,
      F3            = GLFW_KEY_F3,
      F4            = GLFW_KEY_F4,
      F5            = GLFW_KEY_F5,
      F6            = GLFW_KEY_F6,
      F7            = GLFW_KEY_F7,
      F8            = GLFW_KEY_F8,
      F9            = GLFW_KEY_F9,
      F10           = GLFW_KEY_F10,
      F11           = GLFW_KEY_F11,
      F12           = GLFW_KEY_F12,
      F13           = GLFW_KEY_F13,
      F14           = GLFW_KEY_F14,
      F15           = GLFW_KEY_F15,
      F16           = GLFW_KEY_F16,
      F17           = GLFW_KEY_F17,
      F18           = GLFW_KEY_F18,
      F19           = GLFW_KEY_F19,
      F20           = GLFW_KEY_F20,
      F21           = GLFW_KEY_F21,
      F22           = GLFW_KEY_F22,
      F23           = GLFW_KEY_F23,
      F24           = GLFW_KEY_F24,
      F25           = GLFW_KEY_F25,
      kp_0          = GLFW_KEY_KP_0,
      kp_1          = GLFW_KEY_KP_1,
      kp_2          = GLFW_KEY_KP_2,
      kp_3          = GLFW_KEY_KP_3,
      kp_4          = GLFW_KEY_KP_4,
      kp_5          = GLFW_KEY_KP_5,
      kp_6          = GLFW_KEY_KP_6,
      kp_7          = GLFW_KEY_KP_7,
      kp_8          = GLFW_KEY_KP_8,
      kp_9          = GLFW_KEY_KP_9,
      kp_decimal    = GLFW_KEY_KP_DECIMAL,
      kp_divide     = GLFW_KEY_KP_DIVIDE,
      kp_multiply   = GLFW_KEY_KP_MULTIPLY,
      kp_subtract   = GLFW_KEY_KP_SUBTRACT,
      kp_add        = GLFW_KEY_KP_ADD,
      kp_enter      = GLFW_KEY_KP_ENTER,
      kp_equal      = GLFW_KEY_KP_EQUAL,
      left_shift    = GLFW_KEY_LEFT_SHIFT,
      left_control  = GLFW_KEY_LEFT_CONTROL,
      left_alt      = GLFW_KEY_LEFT_ALT,
      left_super    = GLFW_KEY_LEFT_SUPER,
      right_shift   = GLFW_KEY_RIGHT_SHIFT,
      right_control = GLFW_KEY_RIGHT_CONTROL,
      right_alt     = GLFW_KEY_RIGHT_ALT,
      right_super   = GLFW_KEY_RIGHT_SUPER,
      menu          = GLFW_KEY_MENU,
    };
  } // namespace key_code

  struct keyboard_status
  {
    modifier_keys::modifier_keys modifiers = modifier_keys::none;
  };
} // namespace neam::hydra::events



// kate: indent-mode cstyle; indent-width 2; replace-tabs on;

