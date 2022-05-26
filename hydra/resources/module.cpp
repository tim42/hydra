//
// created by : Timothée Feuillet
// date: 2022-4-25
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

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif
#include <sys/mman.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include <fmt/format.h>


#include "module.hpp"

namespace neam::resource
{
  /// \brief workaround dlopen not being able to load from memory but requiring a pathc
  ///        workaround fdlopen being a bsd-ism
  static void* memdlopen(const void* const memory, const size_t size)
  {
    // ONLY WORKS ON LINUX, USING GNU LIBC EXTENSIONS
    const int fd = memfd_create("", MFD_CLOEXEC);
    check::unx::n_assert_success(fd);
    check::unx::n_assert_success(ftruncate(fd, size));

    void* mem = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
    check::unx::n_assert(mem != nullptr, "mmap failed");
    memcpy(mem, memory, size);
    check::unx::n_assert_success(munmap(mem, size));

    std::string path = fmt::format("/proc/self/fd/{}", fd);
    void* ret = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    check::unx::n_assert_success(close(fd));
    return ret;
  }
}

