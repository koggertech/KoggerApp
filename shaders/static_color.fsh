#version 140

uniform vec4 color;
uniform bool isPoint;
uniform bool isTriangle;

void main()
{
    if (isPoint) {
        vec2 coord = gl_PointCoord * 2.0 - 1.0;
        if (dot(coord, coord) > 1.0) {
            discard;
        }
    }
    if (isTriangle) {
        vec2 coord = gl_PointCoord * 2.0 - 1.0;
        coord.y += 2.0;

        if (coord.y < 0.0 || coord.y > 2.0 || abs(coord.x) > (1.0 - coord.y / 2.0)) {
            discard;
        }
    }
    gl_FragColor = color;
}
