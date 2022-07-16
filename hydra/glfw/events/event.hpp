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

#ifndef __N_355671688686967676_2128128696__EVENT_HPP__
# define __N_355671688686967676_2128128696__EVENT_HPP__

#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

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
      meta = GLFW_MOD_SUPER
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
      unknown = -1,
      space = 32,
      apostrophe = 39, /* ' */
      comma = 44, /* , */
      minus = 45, /* - */
      period = 46, /* . */
      slash = 47, /* / */
      key_0 = 48,
      key_1 = 49,
      key_2 = 50,
      key_3 = 51,
      key_4 = 52,
      key_5 = 53,
      key_6 = 54,
      key_7 = 55,
      key_8 = 56,
      key_9 = 57,
      semicolon = 59, /* ; */
      equal = 61, /* = */
      a = 65,
      b = 66,
      c = 67,
      d = 68,
      e = 69,
      f = 70,
      g = 71,
      h = 72,
      i = 73,
      j = 74,
      k = 75,
      l = 76,
      m = 77,
      n = 78,
      o = 79,
      p = 80,
      q = 81,
      r = 82,
      s = 83,
      t = 84,
      u = 85,
      v = 86,
      w = 87,
      x = 88,
      y = 89,
      z = 90,
      left_bracket = 91, /* [ */
      backslash = 92, /* \ */
      right_bracket = 93, /* ] */
      grave_accent = 96, /* ` */
      world_1 = 161, /* non-us #1 */
      world_2 = 162, /* non-us #2 */
      escape = 256,
      enter = 257,
      tab = 258,
      backspace = 259,
      insert = 260,
      del = 261,
      right = 262,
      left = 263,
      down = 264,
      up = 265,
      page_up = 266,
      page_down = 267,
      home = 268,
      end = 269,
      caps_lock = 280,
      scroll_lock = 281,
      num_lock = 282,
      print_screen = 283,
      pause = 284,
      F1 = 290,
      F2 = 291,
      F3 = 292,
      F4 = 293,
      F5 = 294,
      F6 = 295,
      F7 = 296,
      F8 = 297,
      F9 = 298,
      F10 = 299,
      F11 = 300,
      F12 = 301,
      F13 = 302,
      F14 = 303,
      F15 = 304,
      F16 = 305,
      F17 = 306,
      F18 = 307,
      F19 = 308,
      F20 = 309,
      F21 = 310,
      F22 = 311,
      F23 = 312,
      F24 = 313,
      F25 = 314,
      kp_0 = 320,
      kp_1 = 321,
      kp_2 = 322,
      kp_3 = 323,
      kp_4 = 324,
      kp_5 = 325,
      kp_6 = 326,
      kp_7 = 327,
      kp_8 = 328,
      kp_9 = 329,
      kp_decimal = 330,
      kp_divide = 331,
      kp_multiply = 332,
      kp_subtract = 333,
      kp_add = 334,
      kp_enter = 335,
      kp_equal = 336,
      left_shift = 340,
      left_control = 341,
      left_alt = 342,
      left_super = 343,
      right_shift = 344,
      right_control = 345,
      right_alt = 346,
      right_super = 347,
      menu = 348
    };
  } // namespace key_code

  struct keyboard_status
  {
    modifier_keys::modifier_keys modifiers = modifier_keys::none;
  };
} // namespace neam::hydra::events

#endif /*__N_355671688686967676_2128128696__EVENT_HPP__*/

// kate: indent-mode cstyle; indent-width 2; replace-tabs on;

