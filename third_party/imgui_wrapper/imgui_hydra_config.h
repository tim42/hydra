//
// created by : Timothée Feuillet
// date: 2022-7-16
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

#pragma once

#include <cstdint>
#include <variant>

// forward decls for imgui (type-safe images)
namespace neam
{
  enum class id_t : uint64_t;
}
namespace neam::hydra::vk
{
  class image_view;
}
namespace neam::hydra::imgui
{
  struct texture_id_t
  {
    texture_id_t() = default;
    texture_id_t(const texture_id_t&) = default;
    texture_id_t(texture_id_t&&) = default;
    texture_id_t& operator = (const texture_id_t&) = default;
    texture_id_t& operator = (texture_id_t&&) = default;

    texture_id_t(long int) : variant { (const ::neam::hydra::vk::image_view*)nullptr } {} // for NULL that imgui sometimes uses
    texture_id_t(void*) : variant { (const ::neam::hydra::vk::image_view*)nullptr } {} // for NULL that imgui sometimes uses
    texture_id_t(int) : variant { (const ::neam::hydra::vk::image_view*)nullptr } {} // for NULL that imgui sometimes uses
    texture_id_t(std::nullptr_t) : variant { (const ::neam::hydra::vk::image_view*)nullptr } {}

    template<typename T> texture_id_t(T&& t) requires(!std::is_same_v<std::decay_t<T>, texture_id_t>) : variant{std::forward<T>(t)} {}
    template<typename T> texture_id_t& operator = (T&& t) requires(!std::is_same_v<std::decay_t<T>, texture_id_t>) { variant = std::forward<T>(t); return *this; }

    // some imgui performs a = 0...
    texture_id_t& operator = (int) { variant = {(const ::neam::hydra::vk::image_view*)nullptr}; return *this; }


    std::variant<const ::neam::hydra::vk::image_view*, unsigned, id_t> variant { (const ::neam::hydra::vk::image_view*)nullptr };


    bool operator == (const texture_id_t& o) const { return variant == o.variant; }
    bool operator != (const texture_id_t& o) const { return variant != o.variant; }

    // used by imgui
    operator long int () const
    {
      return std::visit([](auto& a)
      {
        return (long int)a;
      }, variant);
    }
  };
}

#define ImTextureID ::neam::hydra::imgui::texture_id_t
