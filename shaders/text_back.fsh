#version 120

uniform vec4 color;
varying vec4 fragColor;

void main()
{
    gl_FragColor = color;
}
