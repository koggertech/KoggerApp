#version 140

uniform vec4 color;
uniform float yaw;
uniform float baseScale;

void main()
{
    vec2 coord = gl_PointCoord * 2.0 - 1.0;

    float c = cos(yaw);
    float s = sin(yaw);
    vec2 p = vec2(coord.x * c - coord.y * s, coord.x * s + coord.y * c);

    float baseY = max(baseScale - 0.2, -0.2);
    float tipY = 1.0;
    float baseHalfWidth = baseScale * 0.95;

    bool inTriangle = (p.y >= baseY && p.y <= tipY);
    if (inTriangle) {
        float t = (tipY - p.y) / (tipY - baseY);
        float halfWidth = baseHalfWidth * t;
        inTriangle = abs(p.x) <= halfWidth;
    }

    if (!inTriangle) {
        discard;
    }

    gl_FragColor = color;
}
