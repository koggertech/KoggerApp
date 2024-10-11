#version 140

uniform vec4 color;
uniform bool isPoint;

void main()
{
    if (isPoint) {
        vec2 coord = gl_PointCoord * 2.0 - 1.0;
        if (dot(coord, coord) > 1.0) {
            discard;
        }
    }
    gl_FragColor = color;
}
