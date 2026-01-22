#version 140

uniform vec4 color;
uniform float yaw;

void main()
{
    vec2 coord = gl_PointCoord * 2.0 - 1.0;

    float c = cos(yaw);
    float s = sin(yaw);
    vec2 p = vec2(coord.x * c - coord.y * s, coord.x * s + coord.y * c);

    float headBaseY = 0.2;
    float tipY = 0.9;
    float headHalfWidth = 0.6;

    bool inHead = (p.y >= headBaseY && p.y <= tipY);
    if (inHead) {
        float t = (tipY - p.y) / (tipY - headBaseY);
        float halfWidth = headHalfWidth * t;
        inHead = abs(p.x) <= halfWidth;
    }

    bool inTail = (p.y >= -0.6 && p.y <= headBaseY && abs(p.x) <= 0.15);

    if (!(inHead || inTail)) {
        discard;
    }

    gl_FragColor = color;
}
