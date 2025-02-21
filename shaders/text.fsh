#version 120
#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

uniform vec4 textColor;
uniform sampler2D tex;
varying vec2 v_texcoord;
varying vec4 color;

float width = 0.5;
float edge = 0.5;

void main()
{
    float distance = 1.0 - texture2D(tex,v_texcoord).r;
    float alpha = 1.0 - smoothstep(width, width + edge, distance);
    gl_FragColor = vec4(textColor.rgb, alpha);
};
