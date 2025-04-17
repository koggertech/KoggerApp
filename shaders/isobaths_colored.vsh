#version 140

in vec3 position;

uniform mat4 matrix;

varying vec3 vertice;

void main()
{
    vertice = position;
    gl_Position = matrix * vec4(position, 1.0);
}
