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
#include <string>
#include <vector>

#include <ntools/raw_data.hpp>
#include <ntools/async/async.hpp>

#include "concepts.hpp"
#include "enums.hpp"
#include "metadata.hpp"

namespace neam::hydra { class core_context; }

namespace neam::resources::processor
{
  /// \brief Data sent to the packer. Final output of the processor.
  struct data
  {
    // filename, relative to the source folder (the resource type will be added by the packer)
    string_id resource_id;

    string_id resource_type; // selects the packer

    raw_data data;
    metadata_t metadata;
  };

  /// \brief data sent to a processor
  struct input_data
  {
    std::filesystem::path file;
    raw_data file_data;
    metadata_t metadata;
  };

  /// \brief result of a processor.
  /// It is important that a processor never directly call another processor but instead go through the `to_process`
  /// Directly calling another processor/importer breaks the link between the original resource, the different caches and the final resources.
  struct processed_data
  {
    std::vector<data> to_pack = {};
    std::vector<input_data> to_process = {};
  };

  using chain = async::chain<processed_data&&, status>;

  /// \brief process a file, return either its data as-is or a processed data that can feed multiple packers
  /// \note packers may be invoked out-of-order in multiple threads
  /// \note the filesystem path might not exist or be empty. Don't rely on it. (archives processors might create fake resources)
  ///
  /// The split between processors and packers is to allow separating source format to engine format:
  ///   an image packer will generate mips, perform format conversion, image resize, ...
  ///   an exr processor wil decode the exr file and forward the contained data to the image packer
  ///   a xz archive processor will decompress the file then forward it to the next processor
  ///
  ///   The file my_image.exr.xz will go through the xz processor, the exr processor and be packed by the image packer.
  using function = chain(*)(hydra::core_context& ctx, input_data&& input);

  /// \brief Register a new resource processor
  /// \note By convention, packers uses plain names (like `image` or `shader-module`)
  ///       processors should use mimetypes (like `image/tiff`, `model/obj`)
  ///       If there is no mime type (or it is `application/octet-stream` / `text/plain` / ...)
  ///       you can use `file-ext:.png`, `file-ext:.tar.gz` are also possible.
  bool register_processor(id_t name_id, id_t version_hash, function processor);
  bool unregister_processor(id_t name_id);

  /// \brief Return the processor that either matches the raw_data or the extension
  function get_processor(const raw_data& data, const std::filesystem::path& p);
  id_t get_processor_hash(const raw_data& data, const std::filesystem::path& p);

  /// \brief Return the processor that either matches the raw_data or the extension
  /// \note the extension must be of the following form: `file-ext:.png`
  /// \see the get_processor that takes a filesystem path:
  function get_processor(const raw_data& data, id_t file_extension);
  id_t get_processor_hash(const raw_data& data, id_t file_extension);

  /// \brief Return the processor that matches the type
  function _get_processor(id_t id);
  id_t _get_processor_hash(id_t id);

  /// \brief Basic processor that simply forwards data to a packer:
  /// for example:
  ///   processors::register_processor("file-ext:.txt"_rid, "passthrough"_rid, basic_processor<"raw-data">);
  template<ct::string_holder IDPacker>
  inline chain basic_processor(hydra::core_context& /*ctx*/, input_data&& input)
  {
    const std::string pstr = input.file;
    std::vector<data> to_pack;
    to_pack.push_back(
    {
      .resource_id = string_id::_runtime_build_from_string(pstr.c_str(), pstr.size()),
      .resource_type = string_id(IDPacker),

      .data = std::move(input.file_data),
      .metadata = std::move(input.metadata),
    });
    return chain::create_and_complete({.to_pack = std::move(to_pack)}, status::success);
  }

  /// \brief Create a null processor that ignore files of a given type / extension.
  /// \note It would be a lot, lot more resource efficient to put that file pattern in the excluded list.
  inline chain null_processor(hydra::core_context& /*ctx*/, input_data&& /*input*/)
  {
    return chain::create_and_complete({}, status::success);
  }

  // helpers:
  template<ct::string_holder IDName, typename Processor>
  class raii_register
  {
    public:
      raii_register() { register_processor(string_id(IDName), Processor::processor_hash, Processor::process_resource); }
      ~raii_register() { unregister_processor(string_id(IDName)); }
  };

  /// \brief a processor base class (processorss are just a plain function, but inheriting from this class may help),
  ///        handles automatic registration
  template<typename BaseType, ct::string_holder... IDName>
  class processor
  {
    public:
      // The child class must define this static function:
      //  static process_resource (signature: processor::function)
      //
      // The child class must define this static member:
      //  static constexpr id_t processor_hash = "my-company/my-processor:1.0.0"_rid; // can be any format, but should include provider and version

      [[nodiscard]] static string_id get_resource_id(const std::filesystem::path& p)
      {
        const std::string pstr = p;
        return string_id::_runtime_build_from_string(pstr.c_str(), pstr.size());
      }

  private:
      inline static std::tuple<raii_register<IDName, BaseType>...> _registration;

      // force instantiation of the static member: (and avoid a warning)
      static_assert(&_registration == &_registration);
  };
}

