#version 300 es

#ifdef GL_KHR_blend_equation_advanced
#extension GL_KHR_blend_equation_advanced : enable
#endif

precision highp float;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 matrix;

out vec3 vertice;
out vec3 vertNormal;

void main()
{
    if (isnan(position.x)) {
        gl_Position = vec4(0.0);
        return;
    }

    vertice = position;
    vertNormal = normal;
    gl_Position = matrix * vec4(position, 1.0);
}
