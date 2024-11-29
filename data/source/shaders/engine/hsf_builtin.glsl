
#pragma once

// require the necessary extensions:
@glsl:extension GL_EXT_samplerless_texture_functions : require
@glsl:extension GL_EXT_scalar_block_layout : require
@glsl:extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
@glsl:extension GL_EXT_shader_explicit_arithmetic_types_float64 : require
@glsl:extension GL_EXT_shader_explicit_arithmetic_types : require
@glsl:extension GL_EXT_shader_16bit_storage : require
@glsl:extension GL_EXT_shader_8bit_storage : require

@glsl:extension GL_KHR_shader_subgroup_ballot : require

@glsl:if hydra::is_stage(task, mesh)
  @glsl:extension GL_EXT_mesh_shader : require
@glsl:endif

@glsl:extension GL_EXT_nonuniform_qualifier : require

// remove the EXT:
#define nonuniform nonuniformEXT

// This source file is included at the very top of all .hsf files.
// It contains all the necessary transformations for an easier glsl shader building

// fallback variables:
#define HSF_FALLBACK_VAR(type)  _hydra_hsf_##type
#define HSF_DECLARE_FALLBACK_VAR(type) type HSF_FALLBACK_VAR(type)

#define HSF_FALLBACK_CONST_VAR(type)  _hydra_hsf_const_##type
#define HSF_DECLARE_FALLBACK_CONST_VAR(type) const type HSF_FALLBACK_CONST_VAR(type) = type(0)

#define HSF_FALLBACK_ARRAY(type)  _hydra_hsf_##type##_ar
#define HSF_DECLARE_FALLBACK_ARRAY(type, n) type HSF_FALLBACK_ARRAY(type)[n]

HSF_DECLARE_FALLBACK_VAR(int);
HSF_DECLARE_FALLBACK_VAR(uint);
HSF_DECLARE_FALLBACK_VAR(vec4);
HSF_DECLARE_FALLBACK_VAR(uvec3);
HSF_DECLARE_FALLBACK_VAR(float);

HSF_DECLARE_FALLBACK_ARRAY(float, 6);
HSF_DECLARE_FALLBACK_ARRAY(uvec3, 6);

HSF_DECLARE_FALLBACK_CONST_VAR(uint);

// hydra::source_replace has the following semantic:
// hydra::source_replace(<stage>/<regex>/<stage-match>/<stage-mismatch>/)
// with stage being any valid stage or * (in this case stage-mismatch is ignored, same as if it was ${hydra::stage})
// the order of execution is from top to bottom, so preceding source_replace can affect the following ones.
//
// Some special tokens are globally availlable (${hydra::stage} and ${hydra::entry_point}

// vertex outputs:
hydra::source_replace(hydra::is_stage(vert, mesh)/gl_Position/gl_Position/HSF_FALLBACK_VAR(vec4)/)
hydra::source_replace(vert/gl_ClipDistance/gl_ClipDistance/HSF_FALLBACK_ARRAY(float)/)
hydra::source_replace(vert/gl_PointSize/gl_PointSize/HSF_FALLBACK_VAR(float)/)

// vertex inputs:
// NOTE: Might be dangerous
hydra::source_replace(vert/gl_VertexIndex/gl_VertexIndex/HSF_FALLBACK_VAR(int)/)
hydra::source_replace(vert/gl_InstanceID/gl_InstanceID/HSF_FALLBACK_VAR(int)/)
hydra::source_replace(vert/gl_InstanceIndex/gl_InstanceIndex/HSF_FALLBACK_VAR(int)/)
hydra::source_replace(vert/gl_DrawID/gl_DrawID/HSF_FALLBACK_VAR(int)/)
hydra::source_replace(vert/gl_BaseVertex/gl_BaseVertex/HSF_FALLBACK_VAR(int)/)
hydra::source_replace(vert/gl_BaseInstance/gl_BaseInstance/HSF_FALLBACK_VAR(int)/)

// compute inputs/consts:
// NOTE: Might be dangerous
hydra::source_replace(hydra::is_stage(comp, mesh, task)/gl_NumWorkGroups/gl_NumWorkGroups/HSF_FALLBACK_VAR(uvec3)/)
hydra::source_replace(hydra::is_stage(comp, mesh, task)/gl_WorkGroupID/gl_WorkGroupID/HSF_FALLBACK_VAR(uvec3)/)
hydra::source_replace(hydra::is_stage(comp, mesh, task)/gl_LocalInvocationID/gl_LocalInvocationID/HSF_FALLBACK_VAR(uvec3)/)
hydra::source_replace(hydra::is_stage(comp, mesh, task)/gl_GlobalInvocationID/gl_GlobalInvocationID/HSF_FALLBACK_VAR(uvec3)/)
hydra::source_replace(hydra::is_stage(comp, mesh, task)/gl_LocalInvocationIndex/gl_LocalInvocationIndex/HSF_FALLBACK_VAR(uint)/)
hydra::source_replace(hydra::is_stage(comp, mesh, task)/gl_WorkGroupSize/gl_WorkGroupSize/HSF_FALLBACK_CONST_VAR(uint)/)


// task shaders:
void Fallback_EmitMeshTasks(uint, uint, uint) {}

hydra::source_replace(task/EmitMeshTasksEXT/EmitMeshTasksEXT/Fallback_EmitMeshTasks/)


// mesh shaders:
void Fallback_SetMeshOutputs(uint, uint) {}

struct Fallback_StructMeshVertices
{
  HSF_DECLARE_FALLBACK_VAR(vec4);
};
Fallback_StructMeshVertices Fallback_gl_MeshVertices[6];

// NOTE: Might be hackish/dangerous
hydra::source_replace(mesh/SetMeshOutputsEXT/SetMeshOutputsEXT/Fallback_SetMeshOutputs/)
hydra::source_replace(mesh/gl_PrimitiveTriangleIndicesEXT/gl_PrimitiveTriangleIndicesEXT/HSF_FALLBACK_ARRAY(uvec3)/)
hydra::source_replace(mesh/gl_MeshVerticesEXT/gl_MeshVerticesEXT/Fallback_gl_MeshVertices/)



#undef HSF_FALLBACK_VAR
#undef HSF_DECLARE_FALLBACK_VAR

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define int2 ivec2
#define int3 ivec3
#define int4 ivec4

#define float2 vec2
#define float3 vec3
#define float4 vec4


// Generate all the dependent structs
hydra::generate_dependent_structs
