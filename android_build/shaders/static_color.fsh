#version 300 es

#ifdef GL_KHR_blend_equation_advanced
#extension GL_KHR_blend_equation_advanced : enable
#endif

#ifndef GL_FRAGMENT_PRECISION_HIGH
#define highp mediump
#endif

uniform highp vec4 color;
uniform bool isPoint;
uniform bool isTriangle;

out highp vec4 fragColor;

void main()
{
    if (isPoint) {
        highp vec2 coord = gl_PointCoord * 2.0 - 1.0;
        if (dot(coord, coord) > 1.0) {
            discard;
        }
    }
    if (isTriangle) {
        highp vec2 coord = gl_PointCoord * 2.0 - 1.0;
        coord.y += 2.0;

        if (coord.y < 0.0 || coord.y > 2.0 || abs(coord.x) > (1.0 - coord.y / 2.0)) {
            discard;
        }
    }

    fragColor = color;
}
