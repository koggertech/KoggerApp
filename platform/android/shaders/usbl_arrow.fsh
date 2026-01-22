#version 300 es

#ifdef GL_KHR_blend_equation_advanced
#extension GL_KHR_blend_equation_advanced : enable
#endif

#ifndef GL_FRAGMENT_PRECISION_HIGH
#define highp mediump
#endif

uniform highp vec4 color;
uniform highp float yaw;
uniform highp float baseScale;

out highp vec4 fragColor;

void main()
{
    highp vec2 coord = gl_PointCoord * 2.0 - 1.0;

    highp float c = cos(yaw);
    highp float s = sin(yaw);
    highp vec2 p = vec2(coord.x * c - coord.y * s, coord.x * s + coord.y * c);

    highp float baseY = max(baseScale - 0.2, -0.2);
    highp float tipY = 1.0;
    highp float baseHalfWidth = baseScale * 0.95;

    bool inTriangle = (p.y >= baseY && p.y <= tipY);
    if (inTriangle) {
        highp float t = (tipY - p.y) / (tipY - baseY);
        highp float halfWidth = baseHalfWidth * t;
        inTriangle = abs(p.x) <= halfWidth;
    }

    if (!inTriangle) {
        discard;
    }

    fragColor = color;
}
