#version 300 es
precision mediump float;

uniform vec4 textColor;
uniform sampler2D tex;
in vec2 v_texcoord;
out vec4 color;

const float width = 0.5;
const float edge = 0.5;

void main()
{
    float distance = 1.0 - texture(tex, v_texcoord).r;
    float alpha = 1.0 - smoothstep(width, width + edge, distance);
    color = vec4(textColor.rgb, alpha);
}
