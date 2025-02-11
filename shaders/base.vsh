#version 120
attribute vec3 position;
uniform mat4 matrix;
uniform float width;
varying vec3 vertice;

void main()
{
    vertice = position;
    gl_PointSize = width;
    gl_Position = matrix * vec4(position, 1.0);
}
