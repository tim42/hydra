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
#include <hydra/assets/spirv.hpp>
#include <hydra/engine/core_context.hpp>
#include <ntools/io/io.hpp>

#include <spawn.h>
#include <sys/wait.h>
#include <regex>

#include "process_helpers.hpp"
#include "string_utilities.hpp"
#include "spirv_packer.hpp"

namespace neam::hydra::processor
{
  static constexpr const char* k_deps_target_name = "<!!<ca/ca>!!>";
  static constexpr const char k_final_shader_prefix[] = R"(
// HYDRA SHADER PREFIX:
#version 460
#extension GL_GOOGLE_cpp_style_line_directive : require
#extension GL_ARB_separate_shader_objects : enable

)";

  static std::map<id_t, uint32_t> resolve_hydra_id(std::string& source)
  {
    // find and handle all hydra::id(blah)
    thread_local const std::regex id_builtin_regex("hydra::id *\\( *([a-zA-Z0-9_]+) *\\)");
    std::map<id_t, uint32_t> index_map;

    std::smatch result;
    uint32_t current_index = 1;
    while (std::regex_search(source, result, id_builtin_regex))
    {
      const id_t value = string_id::_runtime_build_from_string(&*result[1].first, result[1].length());
      index_map[value] = current_index;
      source.replace(result.position(), result.length(), fmt::format("{}", current_index));
      ++current_index;
    }
    return index_map;
  }
  static std::vector<packer::spirv_shader_code> get_all_entry_points(std::string& source)
  {
    // find and handle all hydra::entry_point(blah)
    thread_local std::regex entry_point_regex { "hydra::entry_point *\\( *([a-zA-Z0-9_]+) *, *([a-zA-Z0-9_]+) *\\)" };

    std::vector<packer::spirv_shader_code> ret;
    std::smatch result;
    while(std::regex_search(source, result, entry_point_regex))
    {
      std::string func_name = result[1];
      std::string mode = result[2];
      ret.push_back({func_name, mode});
      source.erase(result.position(), result.length());
    }
    return ret;
  }

  // spawn the preprocessor
  struct cpp_output_t
  {
    std::string output;
    std::string messages;
    std::string dependencies;
  };

  pid_t spawn_cpp(hydra::core_context& ctx, const std::string& input, const std::filesystem::path& file, async::chain<cpp_output_t&&>::state&& state)
  {
    constexpr uint32_t read = 0;
    constexpr uint32_t write = 1;

    // create all the pipes (they need to be manually closed)
    id_t output_pipe[2];
    ctx.io.create_pipe(output_pipe[read], output_pipe[write]);
    id_t messages_pipe[2];
    ctx.io.create_pipe(messages_pipe[read], messages_pipe[write]);
    id_t deps_pipe[2];
    ctx.io.create_pipe(deps_pipe[read], deps_pipe[write]);
    id_t file_pipe[2];
    ctx.io.create_pipe(file_pipe[read], file_pipe[write]);

    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_init(&file_actions);
    // close the unused pipe fds:
    // we might have extra fds around, but all of io automanaged fds are close-on-exec, so it should be fine
    // the goal is to limit our bleeding into the other process
    posix_spawn_file_actions_addclose(&file_actions, ctx.io._get_fd(output_pipe[read]));
    posix_spawn_file_actions_addclose(&file_actions, ctx.io._get_fd(messages_pipe[read]));
    posix_spawn_file_actions_addclose(&file_actions, ctx.io._get_fd(deps_pipe[read]));
    posix_spawn_file_actions_addclose(&file_actions, ctx.io._get_fd(file_pipe[write]));
    // setup stdin/out/err fds: (and close all unused fds)
    posix_spawn_file_actions_adddup2(&file_actions, ctx.io._get_fd(file_pipe[read]), 0);
    posix_spawn_file_actions_addclose(&file_actions, ctx.io._get_fd(file_pipe[read]));
    posix_spawn_file_actions_adddup2(&file_actions, ctx.io._get_fd(output_pipe[write]), 1);
    posix_spawn_file_actions_addclose(&file_actions, ctx.io._get_fd(output_pipe[write]));
    posix_spawn_file_actions_adddup2(&file_actions, ctx.io._get_fd(messages_pipe[write]), 2);
    posix_spawn_file_actions_addclose(&file_actions, ctx.io._get_fd(messages_pipe[write]));

    posix_spawn_file_actions_addchdir_np(&file_actions, ctx.res.source_folder.lexically_normal().c_str());

    pid_t cpp_pid;
    posix_spawnp(&cpp_pid, "cpp", &file_actions, nullptr, (char*const*)(std::vector<const char*>{
      {
        "cpp",
        "-x", "c",

        // include dirs:
        "-I", "./", // source folder root
        "-I", file.parent_path().lexically_normal().c_str(), // file parent folder

        // force include files:
        "-include", "shaders/engine/hsf_builtin.glsl",

        // dependency generation:
        "-MMD",
        "-MT", k_deps_target_name, // dest file (for dependencies, will be removed)
        "-MF", fmt::format("/proc/self/fd/{}", ctx.io._get_fd(deps_pipe[write])).c_str(), // dep file

        // final options:
        "-E", "-",

        nullptr,
      }}.data()), environ);
    posix_spawn_file_actions_destroy(&file_actions);

    // close all the unused pipes: (close the write-end for when we only need read, ...)
    ctx.io.close(output_pipe[write]);
    ctx.io.close(messages_pipe[write]);
    ctx.io.close(deps_pipe[write]);
    ctx.io.close(file_pipe[read]);

    struct pipe_operation_t
    {
      std::string content = {};
      id_t fd = id_t::none;
    };
    using pipe_chain = async::chain<pipe_operation_t&&>;

    // write into the stdin pipe:
    pipe_chain write_chain = ctx.io.queue_write(file_pipe[write], 0, raw_data::allocate_from(input))
    .then([&ctx, file_pipe = file_pipe[write]](bool)
    {
      ctx.io.close(file_pipe);
      return pipe_chain::create_and_complete({});
    });

    // queue the read operations:
    pipe_chain msg_chain = read_pipe(ctx, messages_pipe[read])
    .then([fd = messages_pipe[read]](std::string&& content) { return pipe_chain::create_and_complete({std::move(content), fd}); });

    pipe_chain output_chain = read_pipe(ctx, output_pipe[read])
    .then([fd = output_pipe[read]](std::string&& content) { return pipe_chain::create_and_complete({std::move(content), fd}); });

    pipe_chain deps_chain = read_pipe(ctx, deps_pipe[read])
    .then([fd = deps_pipe[read]](std::string&& content) { return pipe_chain::create_and_complete({std::move(content), fd}); });

    // perform the big wait:
    async::multi_chain<cpp_output_t&&>({},
     [messages_fd = messages_pipe[read], output_fd = output_pipe[read], deps_fd = deps_pipe[read]]
     (cpp_output_t& state, pipe_operation_t&& res)
    {
      // no need to have a lock as each touch a separate memory area
      if (res.fd == messages_fd)
        state.messages = std::move(res.content);
      if (res.fd == output_fd)
        state.output = std::move(res.content);
      if (res.fd == deps_fd)
        state.dependencies = std::move(res.content);
    }, write_chain, msg_chain, output_chain, deps_chain).use_state(state);

    return cpp_pid;
  }

  /// \brief Hydra Shader File pre-processor
  /// Pre-process a shader module (from .hsf to .raw-hsf). It does a bit of text-replace and generate the dependency list
  struct hsf_processor : resources::processor::processor<hsf_processor, "file-ext:.hsf">
  {
    static constexpr id_t processor_hash = "neam/hsf-preprocessor:0.1.0"_rid;

    static resources::processor::chain process_resource(hydra::core_context& ctx, resources::processor::input_data&& input)
    {
      const string_id res_id = get_resource_id(input.file);
      input.db.resource_name(res_id, input.file);
      input.db.set_processor_for_file(input.file, processor_hash);

      // get the shader file:
      std::string shader_file;
      shader_file += std::string_view((const char*)input.file_data.data.get(), input.file_data.size);

      // replace all #glsl: with a token that the preprocessor will not hit
      replace_all(shader_file, "#glsl:", "@glsl:");

      // spawn CPP (the C preprocessor) an post-process the result output:
      async::chain<cpp_output_t&&> chain;
      auto proc_chain = queue_process(ctx, [&ctx, file = input.file, shader_file = std::move(shader_file), state = chain.create_state()] mutable
      {
        return spawn_cpp(ctx, shader_file, file, std::move(state));
      });

      struct state_t
      {
        cpp_output_t cpp;
        int ret = 0;
      };
      return async::multi_chain<state_t&&>({}, []<typename T>(state_t& state, T&& in)
      {
        if constexpr (std::is_same_v<cpp_output_t, T>)
        {
          state.cpp = std::move(in);
        }
        else if constexpr (std::is_same_v<int, T>)
        {
          state.ret = in;
        }
      }, chain, proc_chain)
      .then([&ctx, input = std::move(input), res_id](state_t&& out) mutable
      {
        replace_all(out.cpp.messages, "<stdin>", input.file.c_str());
        replace_all(out.cpp.output, "<stdin>", input.file.c_str());

        // output preprocessor messages: (lock the logger to avoid interleaved stuff)
        bool has_warnings = false;
        {
          const auto msgs = split_string(out.cpp.messages, "\n");
          auto logger = neam::cr::out(true);

          for (const auto& msg : msgs)
          {
            if (!msg.size()) continue;
            if (msg.find("error: ") != std::string::npos)
            {
              has_warnings = true;
              input.db.error<hsf_processor>(res_id, "{}", msg);
            }
            else if (msg.find("warning: ") != std::string::npos)
            {
              has_warnings = true;
              input.db.warning<hsf_processor>(res_id, "{}", msg);
            }
            else
            {
              input.db.message<hsf_processor>(res_id, "{}", msg);
            }
          }

          // preprocessor failed: fail
          if (out.ret != 0)
          {
            input.db.error<hsf_processor>(res_id, "failed to pre-process shader file");
            std::vector<resources::processor::data> ret;
            ret.emplace_back(resources::processor::data
            {
              .resource_id = res_id,
              .resource_type = assets::spirv_shader::type_name,
              .data = {},
              .metadata = std::move(input.metadata),
              .db = input.db,
            });

            return resources::processor::chain::create_and_complete({.to_pack = std::move(ret)}, resources::status::failure);
          }
        }

        // normalize the output file / the dependencies:

        // for the output file, we have to replace `# <line> <file> <extra>` with `#line <line> <file>`
        static const std::regex line_reg = std::regex("# ([0-9]+) \"<?([^>\"]*)>?\".*");
        out.cpp.output = std::regex_replace(out.cpp.output, line_reg, "#line $1 \"$2\"");
        // and re-replace the !<glsl>! with #
        replace_all(out.cpp.output, "@glsl:", "#");

        // for the deps file, we replace the target name with nothing,
        replace_all(out.cpp.dependencies, std::string(k_deps_target_name) + ": ", "");
        // replace every \$ with a simple space
        replace_all(out.cpp.dependencies, "\\\n", " ");
        // replace all the double spaces with a single space: (spaces are escaped, so double spaces are ours)
        static const std::regex double_space_reg = std::regex("  *");
        out.cpp.dependencies = std::regex_replace(out.cpp.dependencies, double_space_reg, " ");
        // replace all spaces non prefixed with \ with a newline:
        static const std::regex non_prefixed_space_reg = std::regex("([^\\\\]) ");
        out.cpp.dependencies = std::regex_replace(out.cpp.dependencies, non_prefixed_space_reg, "$1\n");
        // remove all '\'
        replace_all(out.cpp.dependencies, "\\", "");
        // then split the string on newlines:
        if (out.cpp.dependencies.size() > 0)
        {
          std::vector<std::string> deps;
          deps = split_string(out.cpp.dependencies, "\n");
          for (auto& it : deps)
            input.db.add_file_to_file_dependency(input.file, it);
        }

        std::map<id_t, uint32_t> index_map = resolve_hydra_id(out.cpp.output);
        std::vector<packer::spirv_shader_code> entry_points = get_all_entry_points(out.cpp.output);

        std::vector<resources::processor::data> ret;
        ret.emplace_back(resources::processor::data
        {
          .resource_id = res_id,
          .resource_type = assets::spirv_shader::type_name,
          .data = rle::serialize(packer::spirv_packer_input{ .shader_code = (k_final_shader_prefix + out.cpp.output), .constant_id = std::move(index_map), .variations = std::move(entry_points)}),
          .metadata = std::move(input.metadata),
          .db = input.db,
        });
        return resources::processor::chain::create_and_complete({.to_pack = std::move(ret)},
      has_warnings ? resources::status::partial_success : resources::status::success);
    });
    }
  };
}

