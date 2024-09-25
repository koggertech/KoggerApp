#version 330 core

in vec2 vTexCoord;
in float vHeight;

uniform sampler2D indexedTexture;
uniform sampler1D colorTable;

out vec4 fragColor;

void main()
{
    float index = texture(indexedTexture, vTexCoord).r;
    fragColor = texture(colorTable, index);
}
