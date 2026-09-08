#version 300 es

uniform mat4 mvp_matrix;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_box;

out vec2 v_local;
out vec2 v_half;

void main()
{
    v_local = a_box.xy;
    v_half = a_box.zw;
    gl_Position = mvp_matrix * vec4(a_position, 1.0);
}
