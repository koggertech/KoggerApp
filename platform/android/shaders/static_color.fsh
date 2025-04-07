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
    highp vec2 coord = gl_PointCoord * 2.0 - 1.0;

    if (isPoint) {
        if (dot(coord, coord) > 1.0) {
            discard;
        }
    }

    if (isTriangle) {
        // top half circle
        highp vec2 circleCenter = vec2(0.0, -0.75);
        highp float radius = 0.25;
        bool insideCircle = dot(coord - circleCenter, coord - circleCenter) <= radius * radius;

        // bot triangle
        highp float baseY = 0.0;
        highp float tipY = -0.75;
        highp float halfWidth = 0.25;
        bool insideTriangle = (coord.y >= tipY && coord.y <= baseY && abs(coord.x) <= (1.0 - (coord.y - tipY) / (baseY - tipY)) * halfWidth);

        if (!(insideCircle || insideTriangle)) {
            discard;
        }
    }
	
    fragColor = color;
}
