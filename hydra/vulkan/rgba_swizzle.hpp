//
// file : rgba_swizzle.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/rgba_swizzle.hpp
//
// created by : Timothée Feuillet
// date: Mon Aug 08 2016 15:03:24 GMT+0200 (CEST)
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


#include <initializer_list>
#include <vulkan/vulkan.h>

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      enum class component
      {
        r = VK_COMPONENT_SWIZZLE_R,
        g = VK_COMPONENT_SWIZZLE_G,
        b = VK_COMPONENT_SWIZZLE_B,
        a = VK_COMPONENT_SWIZZLE_A,

        identity = VK_COMPONENT_SWIZZLE_IDENTITY,
        one = VK_COMPONENT_SWIZZLE_ONE,
        zero = VK_COMPONENT_SWIZZLE_ZERO,
      };

      /// \brief Class that represent a RGBA swizzle operation (VkComponentMapping)
      class rgba_swizzle
      {
        public:
          /// \brief Default construct the swizzle (identity swizzle)
          constexpr rgba_swizzle() : r(component::identity), g(component::identity), b(component::identity), a(component::identity) {}

          /// \brief Construct the swizzle from a list of components (missing components are set to identity)
          rgba_swizzle(std::initializer_list<component> il) : rgba_swizzle()
          {
            auto it = il.begin();
            if (it != il.end())
            {
              r = *it;
              ++it;
            }
            if (it != il.end())
            {
              g = *it;
              ++it;
            }
            if (it != il.end())
            {
              b = *it;
              ++it;
            }
            if (it != il.end())
            {
              a = *it;
              ++it;
            }
          }

          /// \brief Initialize the swizzle via a string (can be a constexpr string or whatever null terminated or 4-char long)
          /// Format is the following:
          ///   r -> the red component
          ///   g -> the green component
          ///   b -> the blue component
          ///   a -> the alpha component
          ///   i . _ -> the component (identity)
          ///   0 -> set component to 0
          ///   1 -> set component to 1
          /// missing components are set to identity
          ///
          /// \code
          /// rgba_swizzle s1("bgra");
          /// rgba_swizzle s2("0__1");
          /// rgba_swizzle s3("0__r");
          /// \endcode
          consteval rgba_swizzle(const char *il) : rgba_swizzle()
          {
            size_t i = 0;
            if (il[i] != '\0')
              r = get_component_from_char(il[i++]);
            if (il[i] != '\0')
              g = get_component_from_char(il[i++]);
            if (il[i] != '\0')
              b = get_component_from_char(il[i++]);
            if (il[i] != '\0')
              a = get_component_from_char(il[i++]);
          }

          /// \brief Construct from the vulkan structure
          constexpr rgba_swizzle(const VkComponentMapping &o)
            : r(static_cast<component>(o.r)),
              g(static_cast<component>(o.g)),
              b(static_cast<component>(o.b)),
              a(static_cast<component>(o.a))
          {
          }

          /// \brief Copy constructor
          constexpr rgba_swizzle(const rgba_swizzle &o) : r(o.r), g(o.g), b(o.b), a(o.a) {}

          /// \brief Affectation operator
          rgba_swizzle &operator = (const rgba_swizzle &o)
          {
            r = o.r;
            g = o.g;
            b = o.b;
            a = o.a;
            return *this;
          }

          /// \brief Affectation operator (froma char *)
          /// \see rgba_swizzle(const char *)
          rgba_swizzle &operator = (const char *il)
          {
            size_t i = 0;
            if (il[i] != '\0')
              r = get_component_from_char(il[i++]);
            if (il[i] != '\0')
              g = get_component_from_char(il[i++]);
            if (il[i] != '\0')
              b = get_component_from_char(il[i++]);
            if (il[i] != '\0')
              a = get_component_from_char(il[i++]);
            return *this;
          }

          /// \brief Yield a VkComponentMapping from a rgba_swizzle instance
          constexpr operator VkComponentMapping() const
          {
            return VkComponentMapping
            {
              .r = static_cast<VkComponentSwizzle>(r),
              .g = static_cast<VkComponentSwizzle>(g),
              .b = static_cast<VkComponentSwizzle>(b),
              .a = static_cast<VkComponentSwizzle>(a),
            };
          }

          /// \brief Comparison operator (==)
          constexpr bool operator == (const rgba_swizzle &o) const
          {
            return r == o.r && g == o.g && b == o.b && a == o.a;
          }

          /// \brief Comparison operator (!=)
          constexpr bool operator != (const rgba_swizzle &o) const
          {
            return r != o.r || g != o.g || b != o.b || a != o.a;
          }

        public: // properties
          component r;
          component g;
          component b;
          component a;

        private: // helpers
          static constexpr component get_component_from_char(char ch)
          {
            switch (ch)
            {
              case 'R':
              case 'r': return component::r;
              case 'G':
              case 'g': return component::g;
              case 'B':
              case 'b': return component::b;
              case 'A':
              case 'a': return component::a;
              case '.':
              case '_':
              case 'I':
              case 'i': return component::identity;
              case '0': return component::zero;
              case '1': return component::one;

              default: return component::identity;
            }
          }
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam



