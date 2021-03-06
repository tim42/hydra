#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 out_vertex_pos;
layout(location = 1) out vec2 out_uv;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    gl_Position = vec4(in_position, 0.0, 1.0);
    out_vertex_pos = vec3(in_position, 0.0);
    out_uv = in_uv;
}
