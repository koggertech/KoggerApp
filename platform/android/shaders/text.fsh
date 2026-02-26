#version 300 es
precision mediump float;

uniform vec4 textColor;
uniform sampler2D tex;
in vec2 v_texcoord;
out vec4 color;

void main()
{
    float alpha = texture(tex, v_texcoord).r;
    color = vec4(textColor.rgb, textColor.a * alpha);
}
