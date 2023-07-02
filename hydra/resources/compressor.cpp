//
// created by : Timothée Feuillet
// date: 2021-12-18
//
//
// Copyright (c) 2021 Timothée Feuillet
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


#include "compressor.hpp"

#if N_RES_LZMA_COMPRESSION
  #include <lzma.h>
#endif

namespace neam::resources
{
  // the removed header from the stream
  constexpr unsigned int k_header[] = { 0x587a37fd, 0x0000005a, };
  constexpr uint64_t k_size_header = 0xFF00FF0000000000;
  constexpr uint64_t k_size_header_mask = 0x000000FFFFFFFFFF;

  async::chain<raw_data> compress(raw_data&& in, threading::task_manager* tm, threading::group_t group)
  {
#if N_RES_LZMA_COMPRESSION
    auto lbd = [in = std::move(in)]() mutable
    {
      TRACY_SCOPED_ZONE;
      raw_data out = raw_data::allocate(lzma_stream_buffer_bound(in.size));

      size_t real_size = 0;
      lzma_ret ret = lzma_easy_buffer_encode(9 | LZMA_PRESET_EXTREME, LZMA_CHECK_NONE, nullptr,
                                             (uint8_t*)in.data.get(), in.size,
                                             (uint8_t*)out.data.get(), &real_size, out.size);
      out.size = real_size;
      if (ret != LZMA_OK)
      {
        cr::out().error("resources::compress: lzma encoder failed (code: {})", std::to_underlying(ret));
        return async::chain<raw_data>::create_and_complete({});
      }

      // Check that the header that we are going to remove is the correct one:
      check::debug::n_check(((uint32_t*)out.data.get())[0] == k_header[0], "bad lzma header @0: got 0x{:X}, expected: 0x{:X}",
                            ((uint32_t*)out.data.get())[0], k_header[0]);
      check::debug::n_check(((uint32_t*)out.data.get())[1] == k_header[1], "bad lzma header @1: got 0x{:X}, expected: 0x{:X}",
                            ((uint32_t*)out.data.get())[1], k_header[1]);

      // write the original size over the header (the uncompress restore the header):
      *(uint64_t*)out.data.get() = k_size_header | in.size;

      cr::out().debug("resources::compress: compressed {} bytes into {} bytes (output is {}% of input)", in.size, out.size, out.size * 100 / in.size);
      return async::chain<raw_data>::create_and_complete(std::move(out));
    };
    if (tm != nullptr)
    {
      async::chain<raw_data> ret;
      tm->get_task(group, [lbd = std::move(lbd), state = ret.create_state()]() mutable
      {
        lbd().use_state(state);
      });
      return ret;
    }
    else
    {
      return lbd();
    }
#else
    return async::chain<raw_data>::create_and_complete(std::move(in));
#endif
  }

  async::chain<raw_data> uncompress(raw_data&& in, threading::task_manager* /*tm*/, threading::group_t /*group*/)
  {
#if N_RES_LZMA_COMPRESSION
    TRACY_SCOPED_ZONE;
    if (in.size < sizeof(uint64_t))
    {
      cr::out().error("resources::uncompress: cannot uncompress a data smaller than the minimal header (got {} bytes)", in.size);
      return async::chain<raw_data>::create_and_complete({});
    }

    // grab the original size:
    const uint64_t header_size = *(const uint64_t*)in.data.get();
    const uint64_t size = header_size & k_size_header_mask;
    const uint64_t header_check = header_size & ~k_size_header_mask;
    if (header_check != k_size_header)
    {
      cr::out().error("resources::uncompress: invalid header. Corrupted data?", in.size);
      return async::chain<raw_data>::create_and_complete({});
    }

    // write-back the header
    ((uint32_t*)in.data.get())[0] = k_header[0];
    ((uint32_t*)in.data.get())[1] = k_header[1];

    raw_data out = raw_data::allocate(size);

    size_t memlimit = UINT64_MAX;
    size_t in_end_pos = 0;
    size_t out_end_pos = 0;
    lzma_ret ret = lzma_stream_buffer_decode(&memlimit, LZMA_IGNORE_CHECK, nullptr,
                                             (const uint8_t*)in.data.get(), &in_end_pos, in.size,
                                             (uint8_t*)out.data.get(), &out_end_pos, out.size);

    if (ret != LZMA_OK)
    {
      cr::out().error("resources::uncompress: lzma decoder failed (code: {})", std::to_underlying(ret));
      return async::chain<raw_data>::create_and_complete({});
    }

    if (out_end_pos != out.size)
    {
      cr::out().error("resources::uncompress: uncompressed size is different from the expected size");
      return async::chain<raw_data>::create_and_complete({});
    }

    return async::chain<raw_data>::create_and_complete(std::move(out));
#else
    return async::chain<raw_data>::create_and_complete(std::move(in));
#endif
  }

  // from https://stackoverflow.com/a/2174323
  static size_t get_uncompressed_size(const raw_data& data)
  {
    // Invalid data, so we can avoid doing anything with it
    if (data.size <= LZMA_STREAM_HEADER_SIZE)
      return 0;

    const uint8_t* data_ptr = (uint8_t*)data.data.get();
    const uint8_t* footer_ptr = data_ptr + data.size - LZMA_STREAM_HEADER_SIZE;

    lzma_stream_flags stream_flags;
    lzma_ret ret = lzma_stream_footer_decode(&stream_flags, (const uint8_t*)footer_ptr);
    if (ret != LZMA_OK)
      return 0;

    const uint8_t* index_ptr = footer_ptr - stream_flags.backward_size;

    lzma_index* index = lzma_index_init(nullptr);

    uint64_t memlimit = UINT64_MAX;
    size_t in_pos = 0;
    ret = lzma_index_buffer_decode(&index, &memlimit, nullptr, index_ptr, &in_pos, footer_ptr - index_ptr);

    if (ret != LZMA_OK || in_pos != stream_flags.backward_size)
    {
      lzma_index_end(index, nullptr);
      return 0;
    }

    const lzma_vli size = lzma_index_uncompressed_size(index);
    lzma_index_end(index, nullptr);

    return (size_t)size;
  }

  async::chain<raw_data> uncompress_raw_xz(raw_data&& in, threading::task_manager* tm, threading::group_t group)
  {
#if N_RES_LZMA_COMPRESSION
    auto lbd = [in = std::move(in)]()
    {
      TRACY_SCOPED_ZONE;
      if (in.size < sizeof(uint64_t))
      {
        cr::out().error("resources::uncompress_raw_xz: cannot uncompress a data smaller than the minimal header (got {} bytes)", in.size);
        return async::chain<raw_data>::create_and_complete({});
      }

      // grab the original size:
      const uint64_t size = get_uncompressed_size(in);
      if (size == 0)
      {
        cr::out().error("resources::uncompress_raw_xz: data does not seem to be a valid XZ stream");
        return async::chain<raw_data>::create_and_complete({});
      }

      raw_data out = raw_data::allocate(size);

      size_t memlimit = UINT64_MAX;
      size_t in_end_pos = 0;
      size_t out_end_pos = 0;
      lzma_ret ret = lzma_stream_buffer_decode(&memlimit, 0, nullptr,
                                               (const uint8_t*)in.data.get(), &in_end_pos, in.size,
                                               (uint8_t*)out.data.get(), &out_end_pos, out.size);

      if (ret != LZMA_OK)
      {
        cr::out().error("resources::uncompress_raw_xz: lzma decoder failed (code: {})", std::to_underlying(ret));
        return async::chain<raw_data>::create_and_complete({});
      }

      if (out_end_pos != out.size)
      {
        cr::out().error("resources::uncompress_raw_xz: uncompressed size is different from the expected size");
        return async::chain<raw_data>::create_and_complete({});
      }
      return async::chain<raw_data>::create_and_complete(std::move(out));
    };

    // dispatch
    if (tm != nullptr)
    {
      async::chain<raw_data> ret;
      tm->get_task(group, [lbd = std::move(lbd), state = ret.create_state()]() mutable
      {
        lbd().use_state(state);
      });
      return ret;
    }
    else
    {
      return lbd();
    }
#else
    check::debug::n_assert(false, "resources::uncompress_raw_xz: Trying to decompress a XZ stream without building with XZ support");
    return async::chain<raw_data>::create_and_complete({});
#endif
  }

}

