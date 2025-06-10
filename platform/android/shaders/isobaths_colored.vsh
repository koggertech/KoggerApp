#version 300 es

#ifdef GL_KHR_blend_equation_advanced
#extension GL_KHR_blend_equation_advanced : enable
#endif

precision highp float;

layout(location = 0) in vec3 position;

uniform mat4 matrix;

out vec3 vertice;

void main()
{
    if (isnan(position.x)) {
        gl_Position = vec4(0.0);
        return;
    }

    vertice = position;
    gl_Position = matrix * vec4(position, 1.0);
}
