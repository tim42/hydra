//
// file : hydra_exception.hpp
// in : file:///home/tim/projects/hydra/hydra/hydra_exception.hpp
//
// created by : Timothée Feuillet
// date: Sat Apr 23 2016 15:40:41 GMT+0200 (CEST)
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

#ifndef __N_171722537216030995_2439329636_HYDRA_EXCEPTION_HPP__
#define __N_171722537216030995_2439329636_HYDRA_EXCEPTION_HPP__

#include "tools/demangle.hpp"
#include "tools/debug/vk_errors.hpp"
#include "tools/debug/assert.hpp"

#include "hydra_reflective.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief An exception class that log the exception/report to reflective on throw
    class exception : public std::exception
    {
      public:
        exception(const std::string &s, const std::string &file, size_t line) noexcept
         : str(s)
        {
          NHR_FAIL(neam::r::exception_reason(N_REASON_INFO, strdup(s.data())));
          HYDRA_LOG_TPL(error, file, line, "[EXCEPTION]: " << s << std::endl);
        }
        exception(const std::string &s) noexcept
         : str(s)
        {
          NHR_FAIL(neam::r::exception_reason(N_REASON_INFO, strdup(s.data())));
          HYDRA_LOG(error, "[EXCEPTION]: " << s << std::endl);
        }
        exception(std::string &&s, const std::string &file, size_t line) noexcept
         : str(s)
        {
          NHR_FAIL(neam::r::exception_reason(N_REASON_INFO, strdup(s.data())));
          HYDRA_LOG_TPL(error, file, line, "[EXCEPTION]: " << s << std::endl);
        }
        exception(const std::string &&s) noexcept
         : str(s)
        {
          NHR_FAIL(neam::r::exception_reason(N_REASON_INFO, strdup(s.data())));
          HYDRA_LOG(error, "[EXCEPTION]: " << s << std::endl);
        }

        virtual ~exception() noexcept
        {
        }

        virtual const char *what() const noexcept
        {
          return str.data();
        }

      private:
        std::string str;
    };

    /// \brief a generic exception class that appends the ExceptionType type name to the string
    template<typename ExceptionType>
    class exception_tpl : public exception
    {
      public:
        exception_tpl(const std::string &s, const std::string &file, size_t line) noexcept
         : exception(neam::demangle<ExceptionType>() + ": " + s, file, line)
        {
        }
        exception_tpl(const std::string &s) noexcept
         : exception(neam::demangle<ExceptionType>() + ": " + s)
        {
        }
        exception_tpl(std::string &&s, const std::string &file, size_t line) noexcept
         : exception(neam::demangle<ExceptionType>() + ": " + (s), file, line)
        {
        }
        exception_tpl(const std::string &&s) noexcept
         : exception(neam::demangle<ExceptionType>() + ": " + s)
        {
        }

        virtual ~exception_tpl() noexcept
        {
        }
    };

    namespace check
    {
      /// \brief a shorthand to throwing exceptions when vulkan make some errors
      using on_vulkan_error = neam::debug::on_error<neam::debug::errors::vulkan_errors, neam::hydra::exception_tpl>;
    } // namespace check
  } // namespace hydra
} // namespace neam

#endif // __N_171722537216030995_2439329636_HYDRA_EXCEPTION_HPP__

