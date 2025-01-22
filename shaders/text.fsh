#version 330 core
#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

uniform vec4 textColor;
uniform sampler2D tex;
varying vec2 v_texcoord;
out vec4 color;

float width = 0.51;
float edge = 0.02;

void main()
{
    float distance = 1.0 - texture(tex,v_texcoord).r;
    float alpha = 1.0 - smoothstep(width, width + edge, distance);
    color = vec4(vec3(textColor.r,textColor.g,textColor.b), alpha);
};