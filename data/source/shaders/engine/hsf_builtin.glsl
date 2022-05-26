
#pragma once

// This source file is included at the very top of all .hsf files.
// It contains all the necessary transformations for an easier glsl shader building

// fallback variables:
#define HSF_FALLBACK_VAR(type)  _hydra_hsf_##type
#define HSF_FALLBACK_ARRAY(type)  _hydra_hsf_##type##_ar
#define HSF_DECLARE_FALLBACK_VAR(type) type HSF_FALLBACK_VAR(type)
#define HSF_DECLARE_FALLBACK_ARRAY(type, n) type HSF_FALLBACK_ARRAY(type)[n]

HSF_DECLARE_FALLBACK_VAR(int);
HSF_DECLARE_FALLBACK_VAR(vec4);
HSF_DECLARE_FALLBACK_VAR(float);
HSF_DECLARE_FALLBACK_ARRAY(float, 6);

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

#undef HSF_FALLBACK_VAR
#undef HSF_DECLARE_FALLBACK_VAR
