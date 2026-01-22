#version 300 es

#ifdef GL_KHR_blend_equation_advanced
#extension GL_KHR_blend_equation_advanced : enable
#endif

#ifndef GL_FRAGMENT_PRECISION_HIGH
#define highp mediump
#endif

uniform highp vec4 color;
uniform highp float yaw;

out highp vec4 fragColor;

void main()
{
    highp vec2 coord = gl_PointCoord * 2.0 - 1.0;

    highp float c = cos(yaw);
    highp float s = sin(yaw);
    highp vec2 p = vec2(coord.x * c - coord.y * s, coord.x * s + coord.y * c);

    highp float headBaseY = 0.2;
    highp float tipY = 0.9;
    highp float headHalfWidth = 0.6;

    bool inHead = (p.y >= headBaseY && p.y <= tipY);
    if (inHead) {
        highp float t = (tipY - p.y) / (tipY - headBaseY);
        highp float halfWidth = headHalfWidth * t;
        inHead = abs(p.x) <= halfWidth;
    }

    bool inTail = (p.y >= -0.6 && p.y <= headBaseY && abs(p.x) <= 0.15);

    if (!(inHead || inTail)) {
        discard;
    }

    fragColor = color;
}
