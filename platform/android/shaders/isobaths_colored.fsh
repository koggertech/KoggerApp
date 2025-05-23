#version 300 es

#ifdef GL_KHR_blend_equation_advanced
#extension GL_KHR_blend_equation_advanced : enable
#endif

precision highp float;

in vec3 vertice;
out vec4 fragColor;

uniform sampler2D paletteSampler;
uniform float  depthMin;
uniform float  invDepthRange;
uniform float  levelStep;
uniform int    levelCount;

void main()
{
    float norm = clamp((vertice.z - depthMin) * invDepthRange, 0.0, 1.0);

    float idx = floor(norm * float(levelCount));
    norm = idx / float(levelCount - 1.0);

    vec3 color = texture(paletteSampler, vec2(norm, 0.5)).rgb;
    fragColor = vec4(color, 1.0);
}
