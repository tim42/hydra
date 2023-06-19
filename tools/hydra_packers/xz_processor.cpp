//
// created by : Timothée Feuillet
// date: 2022-5-3
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


#include <hydra/resources/processor.hpp>
#include <hydra/resources/compressor.hpp>


namespace neam::hydra::processor
{
  /// \brief Very simple processor that takes a .xz compressed file, uncompress it and forward it to the next processor
  struct xz_processor : resources::processor::processor<xz_processor, "application/x-xz">
  {
    static constexpr id_t processor_hash = "neam/xz-processor:0.1.0"_rid;

    static resources::processor::chain process_resource(hydra::core_context& /*ctx*/, resources::processor::input_data&& input)
    {
      const string_id res_id = get_resource_id(input.file);
      input.db.resource_name(res_id, input.file);

      return resources::uncompress_raw_xz(std::move(input.file_data)).then([input = std::move(input)](raw_data&& data) mutable
      {
        std::vector<resources::processor::input_data> ret;
        ret.emplace_back(resources::processor::input_data
        {
          .file = input.file / input.file.filename().replace_extension(),
          .file_data = std::move(data),
          .metadata = std::move(input.metadata),
          .db = input.db,
        });

        // forward the content of the .xz file to the next processor
        return resources::processor::chain::create_and_complete({.to_process = std::move(ret)}, resources::status::success);
      });
    }
  };
}
