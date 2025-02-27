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
#include <hydra/utilities/shader_gen/block.hpp>
#include <hydra/utilities/shader_gen/descriptor_sets.hpp>

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/SPVRemapper.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include "string_utilities.hpp"

namespace neam::hydra::packer
{
  struct spirv_packer;

  extern const TBuiltInResource glslang_default_builtin_resource;

  static void glslang_print_log(resources::rel_db& db, id_t res_id, std::string str, bool* has_warnings = nullptr)
  {
    if (str.empty()) return;
    const auto msgs = split_string(str, "\n");
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

  static void resolve_hydra_layout(std::string& source, const std::string& stage, const std::string& entry_point)
  {
    // find and handle all hydra::layout(stage(mode), args...)
    //                                   ----- ----   -------
    //                                    CP1  CP2    CP3
    thread_local const std::regex layout_regex { "hydra::layout *\\( *([a-z_A-Z0-9]+)\\(([a-z]+)\\) *, *([^)]+)\\)" };
    std::smatch result;
    while (std::regex_search(source, result, layout_regex))
    {
      const bool match = result[1] == stage || result[1] == entry_point;
      const std::string sem = result[2];
      const std::string args = result[3];

      // replace/remove the layout from the code:
      if (match)
        source.replace(result.position(), result.length(), fmt::format("layout({}) {}", args, sem));
      else
        source.erase(result.position(), result.length());
    }
  }

  static bool resolve_hydra_gen_interface_block(std::string& source, resources::rel_db& db, id_t id, std::vector<id_t>& dependencies)
  {
    // find and handle all hydra::gen_interface_block(struct)
    //                                                ------
    //                                                  CP1
    thread_local const std::regex layout_regex { "hydra::gen_interface_block *\\( *([a-zA-Z0-9:_]+) *\\)" };
    bool success = true;
    std::smatch result;
    while (std::regex_search(source, result, layout_regex))
    {
      const std::string cpp_struct = result[1];

      id_t cpp_id = string_id::_runtime_build_from_string(cpp_struct);
      const std::string res = shaders::internal::generate_struct_body(cpp_id);
      shaders::internal::get_all_dependencies(cpp_id, dependencies);
      if (res.empty())
      {
        success = false;
        db.error<spirv_packer>(id, "hydra::gen_interface_block: could not find struct `{}`", cpp_struct);
      }
      source.replace(result.position(), result.length(), shaders::internal::generate_struct_body(string_id::_runtime_build_from_string(cpp_struct)));
    }
    return success;
  }

  static bool resolve_hydra_require_cpp_struct(std::string& source, resources::rel_db& db, id_t id, std::vector<id_t>& dependencies)
  {
    // find and handle all hydra::require_cpp_struct(struct)
    //                                                ------
    //                                                  CP1
    thread_local const std::regex layout_regex { "hydra::require_cpp_struct *\\( *([a-zA-Z0-9:_]+) *\\)" };
    bool success = true;
    std::smatch result;
    while (std::regex_search(source, result, layout_regex))
    {
      const std::string cpp_struct = result[1];

      id_t cpp_id = string_id::_runtime_build_from_string(cpp_struct);
      const bool struct_is_valid = shaders::internal::is_struct_registered(cpp_id);
      shaders::internal::get_all_dependencies(cpp_id, dependencies, /* insert self */ true);
      if (!struct_is_valid)
      {
        success = false;
        db.error<spirv_packer>(id, "hydra::gen_interface_block: could not find struct `{}`", cpp_struct);
      }
      source.erase(result.position(), result.length());
    }
    return success;
  }
  static bool resolve_hydra_push_constant(std::string& source, resources::rel_db& db, id_t id, std::vector<id_t>& dependencies, const spirv_shader_code& code)
  {
    // find and handle all hydra::push_constant(struct, opt-stages, ...)
    //                                          ------  -----------------
    //                                            CP1    CP2
    thread_local const std::regex layout_regex { "hydra::push_constant *\\( *([a-zA-Z0-9:_]+) *((, *[a-zA-Z0-9_]+ *)*)? *\\)" };
    thread_local const std::regex arg_regex("[a-zA-Z0-9_]+");
    bool success = true;
    std::smatch result;
    while (std::regex_search(source, result, layout_regex))
    {
      const std::string cpp_struct = result[1];

      bool found = false;
      std::smatch arg_result;
      auto it = result[2].first;
      while (std::regex_search(it, result[2].second, arg_result, arg_regex))
      {
        it += arg_result.position() + arg_result.length();
        if (arg_result[0].compare(code.mode) == 0 || arg_result[0].compare(code.entry_point) == 0)
          found = true;
      }

      id_t cpp_id = string_id::_runtime_build_from_string(cpp_struct);
      const std::string res = shaders::internal::generate_struct_body(cpp_id);
      shaders::internal::get_all_dependencies(cpp_id, dependencies);
      if (res.empty())
      {
        success = false;
        db.error<spirv_packer>(id, "hydra::push_constant: could not find struct `{}`", cpp_struct);
      }
      if (found)
      {
        const std::string replace_str = fmt::format("layout(push_constant, scalar) uniform restrict _push_constant_0 {{ {} }}",
                                                    shaders::internal::generate_struct_body(string_id::_runtime_build_from_string(cpp_struct)));
        source.replace(result.position(), result.length(), replace_str);
        //cr::out().log("  push-constant struct: {}", cpp_struct);
      }
      else
      {
        source.erase(result.position(), result.length());
      }

    }
    return success;
  }

  static bool resolve_hydra_descriptor_set(std::string& source, resources::rel_db& db, id_t id, std::vector<id_t>& dependencies, std::vector<assets::descriptor_set_entry>& ds)
  {
    // find and handle all hydra::descriptor_set(set, struct)
    //                                           ---  ------
    //                                           CP1   CP2
    thread_local const std::regex layout_regex { "hydra::descriptor_set *\\( *([0-9]+|_) *, *([a-zA-Z0-9:_]+) *\\)" };
    thread_local const std::regex arg_regex("[a-zA-Z0-9_]+");
    bool success = true;
    std::smatch result;
    std::vector<bool> used_sets;
    while (std::regex_search(source, result, layout_regex))
    {
      const std::string set_str = (std::string)result[1];
      uint32_t set;
      if (set_str == "_")
      {
        uint32_t i = 0;
        for (; i < used_sets.size() && used_sets[i] != false; ++i) {}
        set = i;
      }
      else
      {
        set = (uint32_t)std::stoi(set_str);
      }
      const std::string cpp_struct = result[2];

      used_sets.resize(set + 1, false);
      if (used_sets[set] == true)
      {
        success = false;
        db.error<spirv_packer>(id, "hydra::descriptor_set: duplicate descriptor_set {}: (error for struct `{}`)", set, cpp_struct);
      }
      used_sets[set] = true;
      std::smatch arg_result;
      id_t cpp_id = string_id::_runtime_build_from_string(cpp_struct);
      const std::string res = shaders::internal::generate_descriptor_set(cpp_id, set);
      shaders::internal::get_descriptor_set_dependencies(cpp_id, dependencies);
      if (res.empty())
      {
        success = false;
        db.error<spirv_packer>(id, "hydra::descriptor_set: could not find struct `{}` for set {}", cpp_struct, set);
      }
      ds.push_back({cpp_id, set});
      source.replace(result.position(), result.length(), res);
      //cr::out().log("  descriptor_set struct: (set: {}) {} ", set, cpp_struct);
      //cr::out().log("  descriptor_set struct: {} ", res);
    }
    return success;
  }

  static void resolve_hydra_gen_dependencies(std::string& source, const std::vector<id_t>& dependencies)
  {
    const std::string deps = shaders::internal::generate_structs(dependencies);
    replace_all(source, "hydra::generate_dependent_structs", deps);
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
      const bool match = result[1] == stage || result[1] == "*" || result[1] == "1" || result[1] == "true";
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
    std::vector<assets::push_constant_range> push_constant_ranges;
    std::vector<assets::descriptor_set_entry> descriptor_set;

    std::vector<unsigned int> bytecode;
    std::string entry_point;
    id_t res_index;
    uint32_t stage;
  };

  static async::chain<spirv_compiled_shader_t&&, resources::status> compile_glsl_to_spirv(core_context& ctx, resources::rel_db& db,
                                                                                          id_t root_id, std::string& in_source,
                                                                                          spirv_shader_code&& code)
  {
    async::chain<spirv_compiled_shader_t&&, resources::status> ret;

    ctx.tm.get_long_duration_task([&db, root_id, &in_source, code = std::move(code), state = ret.create_state()] mutable
    {
      TRACY_SCOPED_ZONE;
      const id_t id = parametrize(root_id, code.entry_point.c_str(), code.entry_point.size());
      db.resource_name(id, fmt::format("{}({})", db.resource_name(root_id), code.entry_point));

      std::string source = in_source;
      std::vector<assets::descriptor_set_entry> descriptor_set;

      // do source code replacement:
      replace_all(source, "${hydra::stage}", code.mode);
      replace_all(source, "${hydra::entry_point}", code.entry_point);

      resolve_hydra_is_generic(source, "is_stage", code.mode);
      resolve_hydra_is_generic(source, "is_entry_point", code.entry_point);

      resolve_hydra_source_replace(source, code.mode);

      resolve_hydra_layout(source, code.mode, code.entry_point);

      bool gen_success;
      {
        std::vector<id_t> dependencies;
        gen_success = resolve_hydra_gen_interface_block(source, db, id, dependencies);
        gen_success = gen_success && resolve_hydra_descriptor_set(source, db, id, dependencies, descriptor_set);
        gen_success = gen_success && resolve_hydra_push_constant(source, db, id, dependencies, code);
        gen_success = gen_success && resolve_hydra_require_cpp_struct(source, db, id, dependencies);

        resolve_hydra_gen_dependencies(source, dependencies);
      }

      // compile the shader:
      EShLanguage lang = EShLangCount; // invalid
      const id_t mode_id = string_id::_runtime_build_from_string(code.mode.c_str(), code.mode.size());
      uint32_t stage = 0;
      switch (mode_id)
      {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        case (id_t)"comp"_rid: lang = EShLangCompute; stage = VK_SHADER_STAGE_COMPUTE_BIT; break;

        case (id_t)"vert"_rid: lang = EShLangVertex; stage = VK_SHADER_STAGE_VERTEX_BIT; break;
        case (id_t)"geom"_rid: lang = EShLangGeometry; stage = VK_SHADER_STAGE_GEOMETRY_BIT; break;
        case (id_t)"tesc"_rid: lang = EShLangTessControl; stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT; break;
        case (id_t)"tese"_rid: lang = EShLangTessEvaluation; stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; break;

        case (id_t)"mesh"_rid: lang = EShLangMesh; stage = VK_SHADER_STAGE_MESH_BIT_EXT; break;
        case (id_t)"task"_rid: lang = EShLangTask; stage = VK_SHADER_STAGE_TASK_BIT_EXT; break;

        case (id_t)"frag"_rid: lang = EShLangFragment; stage = VK_SHADER_STAGE_FRAGMENT_BIT; break;

        case (id_t)"rgen"_rid: lang = EShLangRayGen; stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR; break;
        case (id_t)"rint"_rid: lang = EShLangIntersect; stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR; break;
        case (id_t)"rahit"_rid: lang = EShLangAnyHit; stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR; break;
        case (id_t)"rchit"_rid: lang = EShLangClosestHit; stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR; break;
        case (id_t)"rmiss"_rid: lang = EShLangMiss; stage = VK_SHADER_STAGE_MISS_BIT_KHR; break;
        case (id_t)"rcall"_rid: lang = EShLangCallable; stage = VK_SHADER_STAGE_CALLABLE_BIT_KHR; break;
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

      std::vector<assets::push_constant_range> push_constant_ranges;

      if (parse_success && link_success)
      {
        glslang::GlslangToSpv(*program.getIntermediate(lang), spirv, &logger, &spvOptions);
        glslang_print_log(db, id, logger.getAllMessages(), &has_warnings);


        // build vk reflection data:

        // FIXME: Should not be necessary, we already have the structs being used
        const bool has_reflection = program.buildReflection();
        if (has_reflection)
        {
          // Generate push-constant data
          const uint32_t num_ub = program.getNumUniformBlocks();
          std::set<uint32_t> push_constant_blocks;
          for (uint32_t i = 0; i < num_ub; ++i)
          {
            const glslang::TObjectReflection& tor = program.getUniformBlock(i);
            if (tor.getBinding() < 0)
            {
              push_constant_blocks.emplace(tor.index);
              push_constant_ranges.push_back(
              {
                .id = string_id::_runtime_build_from_string(tor.name),
                .size = (uint16_t)tor.size,
              });
            }
          }
        }

        db.debug<spirv_packer>(id, "stage: {}, entry-point: {}: spirv binary size: {}", code.mode, code.entry_point, spirv.size() * sizeof(unsigned int));
        db.debug<spirv_packer>(id, "successfully compiled shader module (stage: {}, entry-point: {})", db.resource_name(id), code.mode, code.entry_point);
      }
      state.complete
      (
        {
          std::move(push_constant_ranges),
          std::move(descriptor_set),
          std::move(spirv),
          std::move(code.entry_point),
          id, stage
        },
        link_success&& parse_success&&gen_success
          ? (has_warnings ? resources::status::partial_success : resources::status::success)
          : resources::status::failure
      );
    });
    return ret;
  }

  struct spirv_packer : resources::packer::packer<assets::spirv_shader, spirv_packer>
  {
    static inline int init = ShInitialize();
    static_assert(&init == &init);

    static constexpr id_t packer_hash = "neam/spirv-packer:0.0.1##[WIP: " __DATE__ ":" __TIME__ "]"_rid;

    static resources::packer::chain pack_resource(hydra::core_context& ctx, resources::processor::data&& data)
    {
      TRACY_SCOPED_ZONE;
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
        assets::spirv_shader root;
        resources::status status;
      };
      state_t state;
      state.root = assets::spirv_shader{.constant_id = std::move(in.constant_id)};
      state.status = (in.variations.size() == 0 ? resources::status::partial_success : resources::status::success);

      // insert the main resource:
      state.res.push_back(
      {
        .id = root_id,
        .metadata = std::move(data.metadata),
      });

      return async::multi_chain<state_t&&>(std::move(state), std::move(compilation_chains), [root_id, &db](state_t& state, spirv_compiled_shader_t&& r, resources::status st)
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
            .root = root_id,
            .stage = r.stage,
            .push_constant_ranges = std::move(r.push_constant_ranges),
            .descriptor_set = std::move(r.descriptor_set),
          }),
        });

        // merge push-constant ranges:

      })
      // keep the source alive until there:
      .then([root_id, source = std::move(source)](state_t&& state)
      {
        state.res.front().data = rle::serialize(state.root);
        return resources::packer::chain::create_and_complete(std::move(state.res), root_id, state.status);
      });
    }
  };
}
