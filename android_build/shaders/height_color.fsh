#version 300 es

#ifdef GL_KHR_blend_equation_advanced
#extension GL_KHR_blend_equation_advanced : enable
#endif

#ifndef GL_FRAGMENT_PRECISION_HIGH
#define highp mediump
#endif

in highp vec3 vertice;
uniform highp float max_z;
uniform highp float min_z;
out highp vec4 fragColor;

highp vec3 getColor(highp float v) {
    highp float vmin = 0.0;
    highp float vmax = 1.0;

    if (v < vmin)
        v = vmin;
    if (v > vmax)
        v = vmax;

    highp float dv = vmax - vmin;
    highp vec3 c = vec3(1.0, 1.0, 1.0);

    if (v < (vmin + 0.25 * dv)) {
       c.r = 0.0;
       c.g = 4.0 * (v - vmin) / dv;
    }
    else if (v < (vmin + 0.5 * dv)) {
       c.r = 0.0;
       c.b = 1.0 + 4.0 * (vmin + 0.25 * dv - v) / dv;
    }
    else if (v < (vmin + 0.75 * dv)) {
       c.r = 4.0 * (v - vmin - 0.5 * dv) / dv;
       c.b = 0.0;
    }
    else {
       c.g = 1.0 + 4.0 * (vmin + 0.75 * dv - v) / dv;
       c.b = 0.0;
    }

    return c;
}

void main() {
    highp float norm_z = (vertice.z - min_z) / abs(max_z - min_z);
    highp vec4 color = vec4(getColor(norm_z) * 0.95, 1.0);
    fragColor = color;
}
