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

#include "spirv_packer.hpp"

#include <hydra/engine/core_context.hpp>

#include <glslang/Public/ShaderLang.h>
#include <SPIRV/SPVRemapper.h>
#include <SPIRV/GlslangToSpv.h>

#include "string_utilities.hpp"

namespace neam::hydra::packer
{
  struct spirv_packer;

  extern const TBuiltInResource glslang_default_builtin_resource;

  static void glslang_print_log(resources::rel_db& db, id_t res_id, std::string str, bool* has_warnings = nullptr)
  {
    if (str.empty()) return;
    const auto msgs = split_string(str, "\n");
    auto logger = neam::cr::out(true);
    for (const auto& msg : msgs)
    {
      if (!msg.size()) continue;
      if (msg.find("ERROR: ") != std::string::npos)
      {
        if (has_warnings) *has_warnings = true;
        db.error<spirv_packer>(res_id, "{}", msg);
      }
      else if (msg.find("WARNING: ") != std::string::npos)
      {
        if (has_warnings) *has_warnings = true;
        db.warning<spirv_packer>(res_id, "{}", msg);
      }
      else
      {
        db.message<spirv_packer>(res_id, "{}", msg);
      }
    }
  }

  static void resolve_hydra_is_generic(std::string& source, const std::string& func_name, const std::string& value)
  {
    // find and handle all hydra::is<func_name>(blah, ...)
    thread_local const std::regex is_stage_regex("hydra::" + func_name + " *\\( *(([a-zA-Z0-9_]+ *, *)* *[a-zA-Z0-9_]+) *\\)");
    thread_local const std::regex arg_regex("[a-zA-Z0-9_]+");

    std::smatch result;
    while (std::regex_search(source, result, is_stage_regex))
    {
      bool found = false;
      std::smatch arg_result;
      auto it = result[1].first;
      while (std::regex_search(it, result[1].second, arg_result, arg_regex))
      {
        it += arg_result.position() + arg_result.length();
        if (arg_result[0].compare(value) == 0)
          found = true;
      }

      if (found)
        source.replace(result.position(), result.length(), "1");
      else
        source.replace(result.position(), result.length(), "0");
    }
  }

  static void resolve_hydra_layout(std::string& source, const std::string& stage)
  {
    // find and handle all hydra::layout(stage(mode), args...)
    //                                   ----- ----   -------
    //                                    CP1  CP2    CP3
    thread_local const std::regex layout_regex { "hydra::layout *\\( *([a-z]+)\\(([a-z]+)\\) *, *([^)]+)\\)" };
    std::smatch result;
    while (std::regex_search(source, result, layout_regex))
    {
      const bool match = result[1] == stage;
      const std::string sem = result[2];
      const std::string args = result[3];

      // replace/remove the layout from the code:
      if (match)
        source.replace(result.position(), result.length(), fmt::format("layout({}) {}", args, sem));
      else
        source.erase(result.position(), result.length());
    }
  }
  static void resolve_hydra_source_replace(std::string& source, const std::string& stage)
  {
    // source replace has the following signature:
    // hydra::source_replace(stage/regex/dest-match/dest-fallback/)
    thread_local const std::regex source_replace_regex { "hydra::source_replace *\\( *([^/]+)/([^/]+)/([^/]*)/([^/]*)/ *\\)" };

    std::smatch result;
    while (std::regex_search(source, result, source_replace_regex))
    {
      // copy the params:
      const bool match = result[1] == stage || result[1] == "*";
      const std::regex re { (std::string)result[2] };
      const std::string token = result[match ? 3 : 4];
      // remove the source_replace from the code:
      source.erase(result.position(), result.length());

      // apply the source replace:
      source = std::regex_replace(source, re, token);
    }
  }

  struct spirv_compiled_shader_t
  {
    std::vector<unsigned int> bytecode;
    std::string entry_point;
    id_t res_index;
  };

  static async::chain<spirv_compiled_shader_t&&, resources::status> compile_glsl_to_spirv(core_context& ctx, resources::rel_db& db,
                                                                                          id_t root_id, std::string& in_source,
                                                                                          spirv_shader_code&& code)
  {
    async::chain<spirv_compiled_shader_t&&, resources::status> ret;

    ctx.tm.get_long_duration_task([&db, root_id, &in_source, code = std::move(code), state = ret.create_state()] mutable
    {
      const id_t id = parametrize(root_id, code.entry_point.c_str(), code.entry_point.size());
      db.resource_name(id, fmt::format("{}({})", db.resource_name(root_id), code.entry_point));

      std::string source = in_source;

      // do source code replacement:
      replace_all(source, "${hydra::stage}", code.mode);
      replace_all(source, "${hydra::entry_point}", code.entry_point);

      resolve_hydra_is_generic(source, "is_stage", code.mode);
      resolve_hydra_is_generic(source, "is_entry_point", code.entry_point);

      resolve_hydra_source_replace(source, code.mode);

      resolve_hydra_layout(source, code.mode);

      // compile the shader:
      EShLanguage lang = EShLangCount; // invalid
      const id_t mode_id = string_id::_runtime_build_from_string(code.mode.c_str(), code.mode.size());
      switch (mode_id)
      {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        case (id_t)"comp"_rid: lang = EShLangCompute; break;

        case (id_t)"vert"_rid: lang = EShLangVertex; break;
        case (id_t)"geom"_rid: lang = EShLangGeometry; break;
        case (id_t)"tesc"_rid: lang = EShLangTessControl; break;
        case (id_t)"tese"_rid: lang = EShLangTessEvaluation; break;
        case (id_t)"mesh"_rid: lang = EShLangMeshNV; break;
        case (id_t)"task"_rid: lang = EShLangTaskNV; break;
        case (id_t)"frag"_rid: lang = EShLangFragment; break;

        case (id_t)"rgen"_rid: lang = EShLangRayGen; break;
        case (id_t)"rint"_rid: lang = EShLangIntersect; break;
        case (id_t)"rahit"_rid: lang = EShLangAnyHit; break;
        case (id_t)"rchit"_rid: lang = EShLangClosestHit; break;
        case (id_t)"rmiss"_rid: lang = EShLangMiss; break;
        case (id_t)"rcall"_rid: lang = EShLangCallable; break;
#pragma GCC diagnostic pop
        case id_t::none:
        case id_t::invalid:
          state.complete({}, resources::status::failure);
          return;
      }

      glslang::TShader shader(lang);

      const char* str[] = { source.c_str() };
      const int len[] = { (int)source.size() };
      shader.setStringsWithLengths(str, len, 1);

      shader.setEntryPoint(code.entry_point.c_str());
      shader.setSourceEntryPoint("main");
      shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 130);
      shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
      shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
      glslang::TShader::ForbidIncluder includer;
      EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules | EShMsgHlslEnable16BitTypes | EShMsgEnhanced);
      const bool parse_success = shader.parse(&glslang_default_builtin_resource, 130, false, messages);
      bool has_warnings = false;
      glslang_print_log(db, id, shader.getInfoLog(), &has_warnings);
      if (!parse_success)
        db.error<spirv_packer>(id, "failed to compile shader module (stage: {}, entry-point: {}) (see errors above)", code.mode, code.entry_point);

      glslang::TProgram program;
      program.addShader(&shader);
      const bool link_success = program.link(messages);
      glslang_print_log(db, id, program.getInfoLog(), &has_warnings);
      if (!link_success)
        db.error<spirv_packer>(id, "failed to link shader module (stage: {}, entry-point: {}) (see errors above)", code.mode, code.entry_point);

      std::vector<unsigned int> spirv;
      spv::SpvBuildLogger logger;
      glslang::SpvOptions spvOptions;
      // FIXME: Add options:
      spvOptions.generateDebugInfo = true;
//             spvOptions.stripDebugInfo = true;
      spvOptions.disableOptimizer = false;
      if (parse_success && link_success)
      {
        glslang::GlslangToSpv(*program.getIntermediate(lang), spirv, &logger, &spvOptions);
        glslang_print_log(db, id, logger.getAllMessages(), &has_warnings);

        db.debug<spirv_packer>(id, "stage: {}, entry-point: {}: spirv binary size: {}", code.mode, code.entry_point, spirv.size() * sizeof(unsigned int));
      }
      db.debug<spirv_packer>(id, "successfully compiled shader module (stage: {}, entry-point: {})", db.resource_name(id), code.mode, code.entry_point);
      state.complete({std::move(spirv), std::move(code.entry_point), id},
                     link_success && parse_success
                     ? (has_warnings ? resources::status::partial_success : resources::status::success)
                     : resources::status::failure);
    });
    return ret;
  }

  struct spirv_packer : resources::packer::packer<assets::spirv_shader, spirv_packer>
  {
    static inline int init = ShInitialize();
    static_assert(&init == &init);

    static constexpr id_t packer_hash = "neam/spirv-packer:0.0.1"_rid;

    static resources::packer::chain pack_resource(hydra::core_context& ctx, resources::processor::data&& data)
    {
      auto& db = data.db;
      const id_t root_id = get_root_id(data.resource_id);
      db.resource_name(root_id, get_root_name(db, data.resource_id));

      // cannot be const, data must be moved from
      spirv_packer_input in;
      if (rle::in_place_deserialize(data.data, in) == rle::status::failure)
      {
        db.error<spirv_packer>(root_id, "failed to deserialize processor data");
        return resources::packer::chain::create_and_complete({}, id_t::invalid, resources::status::failure);
      }

      if (in.variations.size() > 0)
        db.debug<spirv_packer>(root_id, "received {} variations", in.variations.size());
      else
        db.warning<spirv_packer>(root_id, "received {} variations", in.variations.size());

      std::vector<async::chain<spirv_compiled_shader_t&&, resources::status>> compilation_chains;
      compilation_chains.reserve(in.variations.size());

      std::unique_ptr source = std::make_unique<std::string>(in.shader_code);
      for (auto& it : in.variations)
      {
        compilation_chains.push_back(compile_glsl_to_spirv(ctx, db, root_id, *source.get(), std::move(it)));
      }

      struct state_t
      {
        std::vector<resources::packer::data> res;
        resources::status status;
      };
      state_t state;
      state.status = (in.variations.size() == 0 ? resources::status::partial_success : resources::status::success);

      // insert the main resource:
      state.res.push_back(
      {
        .id = root_id,
        .data = rle::serialize(assets::spirv_shader{.constant_id = std::move(in.constant_id)}),
        .metadata = std::move(data.metadata),
      });

      return async::multi_chain<state_t&&>(std::move(state), std::move(compilation_chains), [root_id](state_t& state, spirv_compiled_shader_t&& r, resources::status st)
      {
        static spinlock lock;
        std::lock_guard _l(lock);
        state.status = resources::worst(state.status, st);
        state.res.push_back(
        {
          .id = r.res_index,
          .data = rle::serialize(assets::spirv_variation
          {
            .entry_point = std::move(r.entry_point),
            .module = raw_data::allocate_from(r.bytecode),
            .root = root_id
          }),
        });
      })
      // keep the source alive until there:
      .then([root_id, source = std::move(source)](state_t&& state)
      {
        return resources::packer::chain::create_and_complete(std::move(state.res), root_id, state.status);
      });
    }
  };
}
