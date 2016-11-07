#version 450
#extension GL_ARB_separate_shader_objects : enable

// raymarcher defines
#define RM_MAX_STEPS                    22.
#define INTO_STEP_DST                   0.35

// prog defines
#define ROTATION_SPEED                  0.2000

#define CAM_DISTANCE                    30.
#define CAM_DISTANCE_MVT                10.

// glow
#define GLOW_MAX_DST                    1.0

#define GLOW_COLOR                      scene_color

#define GLOW_STRENGTH                   0.04


layout(location = 0) in vec3 in_vertex_pos;
layout(location = 1) in vec2 in_uv;

layout(binding = 0) uniform sampler2D tex_sampler;
layout(std140, binding = 1) uniform ubo_t
{
  float time;
  vec2 screen_resolution;
} ubo;

layout(location = 0) out vec4 out_color;


const vec3 scene_color = vec3(0.7, 0.70, 1.5);

mat3 rotation_matrix_xY(float angle)
{
  vec3 axis = vec3(0, 1, 0);
  float s = sin(angle);
  float c = cos(angle);
  float oc = 1.0 - c;
  return mat3
  (
    c,          0.0,                            axis.y * s,
    0.0,        oc * axis.y * axis.y + c,       0.0,
    -axis.y * s, 0.0,                           c
  );
}

// where the computations (the distance field) are
float map(vec3 position)
{
  const float c = 15.;

  float d = length(mod(position, 50.) - 50. * 0.5) - c;

  vec3 p = mod(position, c) - c * 0.5;
  return max(-(length(p) - c / 1.8), d);
}

// do the raymarch
vec3 raymarch(vec3 from, in vec3 dir)
{
  vec3 color = vec3(0, 0, 0);

  float dst_acc = 0.;
  float dst_rmg = 0.;

  for(int it = 0; it < RM_MAX_STEPS; ++it)
  {
    dst_rmg = map(from + dir * dst_acc) * 1.0;
    dst_acc += max(INTO_STEP_DST, (dst_rmg));

    if (abs(dst_rmg) <= GLOW_MAX_DST)
      color += GLOW_STRENGTH * GLOW_COLOR * (GLOW_MAX_DST - abs(dst_rmg)) * 10. / ((dst_acc + 1.)); // "glow"
  }

  return sqrt(color);
}

#define PI      3.1415927
void main()
{
  vec2 uv = (in_vertex_pos.xy); // some good trick for a fs quad :)
  uv.x *= ubo.screen_resolution.x / ubo.screen_resolution.y;
  vec3 dir = normalize(vec3(uv * 1.0, 1.0));

  mat3 rotmat = rotation_matrix_xY(mod(ubo.time * ROTATION_SPEED, 2. * PI));

  vec3 pos = vec3(0., 5., -1. * CAM_DISTANCE + sin(ubo.time * ROTATION_SPEED) * CAM_DISTANCE_MVT);
  out_color = vec4(raymarch(pos * rotmat, dir * rotmat), 1.0);

  uv = in_uv;
  uv.x *= ubo.screen_resolution.x / ubo.screen_resolution.y;
  vec4 tex = texture(tex_sampler, uv);
  out_color.rgb = out_color.rgb * (1. - tex.a) + tex.a * vec3(1.);
}
