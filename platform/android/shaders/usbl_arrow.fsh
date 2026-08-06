#version 300 es

#ifdef GL_KHR_blend_equation_advanced
#extension GL_KHR_blend_equation_advanced : enable
#endif

#ifndef GL_FRAGMENT_PRECISION_HIGH
#define highp mediump
#endif

uniform highp vec4 color;
// Compass bearing in RADIANS: 0 points north (up), increasing turns clockwise.
uniform highp float yaw;
// Wing half-span, in sprite radii. The dart's length is fixed; this is what makes it narrow.
uniform highp float halfWidth;

out highp vec4 fragColor;

void main()
{
    highp vec2 coord = gl_PointCoord * 2.0 - 1.0;

    // gl_PointCoord's origin is UPPER-LEFT, so its y grows DOWNWARD. Flipping it here puts the
    // sprite in a screen-up frame, which is the whole reason `yaw` below can be read as a
    // bearing rather than as an arbitrary angle with an arbitrary sign.
    highp vec2 d = vec2(coord.x, -coord.y);

    // Into the dart's own frame, nose at +y. yaw = 0 is the identity, so the nose points up =
    // north; yaw = pi/2 sends it to screen-right = east.
    highp float c = cos(yaw);
    highp float s = sin(yaw);
    highp vec2 q = vec2(d.x * c - d.y * s, d.x * s + d.y * c);

    // A paper dart: nose forward, two swept-back wings, a notch cut into the tail. The notch is
    // what stops it reading as a plain triangle -- and a triangle this size reads as a generic
    // marker rather than as a heading.
    const highp float kNose  = 1.0;
    const highp float kBack  = 0.85;
    const highp float kNotch = 0.25;

    highp float w = max(halfWidth, 0.001);
    highp float ax = abs(q.x);

    highp float halfAt = w * (kNose - q.y) / (kNose + kBack);
    highp float notchAt = -kNotch - ax * (kBack - kNotch) / w;

    if (q.y > kNose || q.y < -kBack || ax > halfAt || q.y < notchAt) {
        discard;
    }

    fragColor = color;
}
