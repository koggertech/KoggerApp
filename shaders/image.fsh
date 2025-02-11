#version 120
varying vec2 vTexCoord;

uniform sampler2D imageTexture;

varying vec4 fragColor;

void main()
{
    gl_FragColor = texture2D(imageTexture, vTexCoord);
}
