#version 300 es
precision mediump float;

uniform mat4 mvp_matrix;

in vec4 a_position;
in vec2 a_texcoord;

out vec2 v_texcoord;

void main()
{
    gl_Position = mvp_matrix * a_position;
    v_texcoord = a_texcoord;
}
