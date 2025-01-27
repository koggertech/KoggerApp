#version 140

uniform vec4 color;
uniform bool isPoint;
uniform bool isTriangle;

void main()
{
    vec2 coord = gl_PointCoord * 2.0 - 1.0;

    if (isPoint) {
        if (dot(coord, coord) > 1.0) {
            discard;
        }
    }

    if (isTriangle) {
        // top half circle
        vec2 circleCenter = vec2(0.0, -0.75);
        float radius = 0.25;
        bool insideCircle = dot(coord - circleCenter, coord - circleCenter) <= radius * radius;

        // bot triangle
        float baseY = 0.0;
        float tipY = -0.75;
        float halfWidth = 0.25;
        bool insideTriangle = (coord.y >= tipY && coord.y <= baseY && abs(coord.x) <= (1.0 - (coord.y - tipY) / (baseY - tipY)) * halfWidth);

        if (!(insideCircle || insideTriangle)) {
            discard;
        }
    }
    gl_FragColor = color;
}
