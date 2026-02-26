#version 330 core
#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

uniform vec4 textColor;
uniform sampler2D tex;
in vec2 v_texcoord;
out vec4 color;

void main()
{
    float alpha = texture(tex, v_texcoord).r;
    color = vec4(textColor.rgb, textColor.a * alpha);
};
