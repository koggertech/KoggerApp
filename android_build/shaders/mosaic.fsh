#version 300 es

precision mediump float;

in vec2 vTexCoord;
in float vHeight;

uniform sampler2D indexedTexture;
uniform sampler2D colorTable;

out vec4 fragColor;

void main()
{
    float index = texture(indexedTexture, vTexCoord).r;

    if (index == 0.0) {
        discard;
    }
    else {
        fragColor = texture(colorTable, vec2(index, 0.5));
    }
}
