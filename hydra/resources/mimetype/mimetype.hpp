//
// created by : Timothée Feuillet
// date: 2022-4-26
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

#include <filesystem>

#include <ntools/id/id.hpp>
#include <ntools/raw_data.hpp>

namespace neam::resources::mime
{
  // Please use the raw data + liburing/io::context instead
  // there's less overhead this way (and this can be made async/run on any thread)
  namespace bad_idea
  {
    /// \brief returns the mimetype from a file
    const char* get_mimetype(const std::filesystem::path& path);

    /// \brief returns the mimetype id from a file (for faster comparisons)
    id_t get_mimetype_id(const std::filesystem::path& path);
  }

  /// \brief returns the mimetype from a buffer
  const char* get_mimetype(const void* data, size_t len);

  /// \brief returns the mimetype id from a buffer
  id_t get_mimetype_id(const void* data, size_t len);

  /// \brief returns the mimetype from a buffer
  [[maybe_unused]] static inline const char* get_mimetype(const raw_data& rd)
  {
      return get_mimetype(rd.data.get(), rd.size);
  }

  /// \brief returns the mimetype id from a buffer
  [[maybe_unused]] static inline id_t get_mimetype_id(const raw_data& rd)
  {
    return get_mimetype_id(rd.data.get(), rd.size);
  }
}

