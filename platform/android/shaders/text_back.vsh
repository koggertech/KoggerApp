#version 300 es

uniform mat4 mvp_matrix;

layout(location = 0) in vec3 a_position;

void main()
{
    gl_Position = mvp_matrix * vec4(a_position, 1.0);
}
