
#pragma once

// require the necessary extensions:
@glsl:extension GL_EXT_samplerless_texture_functions : require

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

HSF_DECLARE_FALLBACK_CONST_VAR(uint);

// hydra::source_replace has the following semantic:
// hydra::source_replace(<stage>/<regex>/<stage-match>/<stage-mismatch>/)
// with stage being any valid stage or * (in this case stage-mismatch is ignored, same as if it was ${hydra::stage})
// the order of execution is from top to bottom, so preceding source_replace can affect the following ones.
//
// Some special tokens are globally availlable (${hydra::stage} and ${hydra::entry_point}

// vertex outputs:
hydra::source_replace(vert/gl_Position/gl_Position/HSF_FALLBACK_VAR(vec4)/)
hydra::source_replace(vert/gl_PointSize/gl_PointSize/HSF_FALLBACK_VAR(float)/)
hydra::source_replace(vert/gl_ClipDistance/gl_ClipDistance/HSF_FALLBACK_ARRAY(float)/)

// vertex inputs:
// NOTE: Might be dangerous
hydra::source_replace(vert/gl_VertexIndex/gl_VertexIndex/HSF_FALLBACK_VAR(int)/)
hydra::source_replace(vert/gl_InstanceID/gl_InstanceID/ HSF_FALLBACK_VAR(int)/)
hydra::source_replace(vert/gl_DrawID/gl_DrawID/HSF_FALLBACK_VAR(int)/)
hydra::source_replace(vert/gl_BaseVertex/gl_BaseVertex/HSF_FALLBACK_VAR(int)/)
hydra::source_replace(vert/gl_BaseInstance/gl_BaseInstance/HSF_FALLBACK_VAR(int)/)

// compute inputs/consts:
// NOTE: Might be dangerous
hydra::source_replace(comp/gl_NumWorkGroups/gl_NumWorkGroups/HSF_FALLBACK_VAR(uvec3)/)
hydra::source_replace(comp/gl_WorkGroupID/gl_WorkGroupID/HSF_FALLBACK_VAR(uvec3)/)
hydra::source_replace(comp/gl_LocalInvocationID/gl_LocalInvocationID/HSF_FALLBACK_VAR(uvec3)/)
hydra::source_replace(comp/gl_GlobalInvocationID/gl_GlobalInvocationID/HSF_FALLBACK_VAR(uvec3)/)
hydra::source_replace(comp/gl_LocalInvocationIndex/gl_LocalInvocationIndex/HSF_FALLBACK_VAR(uint)/)
hydra::source_replace(comp/gl_WorkGroupSize/gl_WorkGroupSize/HSF_FALLBACK_CONST_VAR(uint)/)


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

