#version 300 es

precision mediump float;

in vec2 vTexCoord;
in float vHeight;

uniform sampler2D texture1;

out vec4 fragColor;

void main()
{
    fragColor = texture(texture1, vTexCoord);
}
