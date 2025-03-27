#version 300 es
precision mediump float;

in vec2 vTexCoord;

uniform sampler2D imageTexture;

out vec4 fragColor;

void main()
{
    fragColor = texture(imageTexture, vTexCoord);
}
