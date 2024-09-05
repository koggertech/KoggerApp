#version 300 es

#ifdef GL_KHR_blend_equation_advanced
#extension GL_KHR_blend_equation_advanced : enable
#endif

#ifndef GL_FRAGMENT_PRECISION_HIGH
#define highp mediump
#endif

uniform highp vec4 color;
uniform bool isPoint;

out highp vec4 fragColor;

void main()
{
    if (isPoint) {
        highp vec2 coord = gl_PointCoord * 2.0 - 1.0;
        if (dot(coord, coord) > 1.0) {
            discard;
        }
    }
    fragColor = color;
}
