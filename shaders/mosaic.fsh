#version 120

varying vec2 vTexCoord;
varying float vHeight;

uniform sampler2D indexedTexture;
uniform sampler1D colorTable;

varying vec4 fragColor;

void main()
{
    float index = texture2D(indexedTexture, vTexCoord).r;

    if (index == 0.0) {
        discard;
    }
    else {
        gl_FragColor = texture1D(colorTable, index);
    }
}
