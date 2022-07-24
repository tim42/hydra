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
  async::chain<raw_data> compress(raw_data&& in, threading::task_manager* tm, threading::group_t group)
  {
#if N_RES_LZMA_COMPRESSION
    auto lbd = [in = std::move(in)]() mutable
    {
      raw_data out = raw_data::allocate(lzma_stream_buffer_bound(in.size));

      size_t real_size = 0;
      lzma_ret ret = lzma_easy_buffer_encode(9 | LZMA_PRESET_EXTREME, LZMA_CHECK_NONE, nullptr,
                                             (uint8_t*)in.data.get(), in.size,
                                             (uint8_t*)out.data.get(), &real_size, out.size);
      out.size = real_size;
      if (ret != LZMA_OK)
      {
        cr::out().error("resources::compress: lzma encoder failed (code: {})", ret);
        return async::chain<raw_data>::create_and_complete({});
      }

      // write the original size over the header (the uncompress restore the header):
      *(uint64_t*)out.data.get() = in.size;

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
    // grab the original size:
    const uint64_t size = *(const uint64_t*)in.data.get();

    // the removed header from the stream
    const unsigned int header[] = { 0x587a37fd, 0x0000005a, };

    // write-back the header
    ((uint32_t*)in.data.get())[0] = header[0];
    ((uint32_t*)in.data.get())[1] = header[1];

    raw_data out = raw_data::allocate(size);

    size_t memlimit = UINT64_MAX;
    size_t in_end_pos = 0;
    size_t out_end_pos = 0;
    lzma_ret ret = lzma_stream_buffer_decode(&memlimit, 0, nullptr,
                                             (const uint8_t*)in.data.get(), &in_end_pos, in.size,
                                             (uint8_t*)out.data.get(), &out_end_pos, out.size);

    if (ret != LZMA_OK)
    {
      cr::out().error("resources::compress: lzma encoder failed (code: {})", ret);
      return async::chain<raw_data>::create_and_complete({});
    }

    if (out_end_pos != out.size)
    {
      cr::out().error("resources::compress: uncompressed size is different from the expected size");
      return async::chain<raw_data>::create_and_complete({});
    }

    return async::chain<raw_data>::create_and_complete(std::move(out));
#else
    return async::chain<raw_data>::create_and_complete(std::move(in));
#endif
  }
}

