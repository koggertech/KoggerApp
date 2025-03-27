#version 300 es

precision highp float;
in vec3 position;
uniform mat4 matrix;
uniform float width;
out vec3 vertice;

void main()
{
    vertice = position;
    gl_PointSize = width;
    gl_Position = matrix * vec4(position, 1.0);
}
