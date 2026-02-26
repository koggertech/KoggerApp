#version 300 es

precision highp float;

uniform vec4 color;
in float vBaseShade;
in float vHighlight;
out vec4 fragColor;

void main()
{
    vec3 shaded = color.rgb * vBaseShade;
    vec3 highlighted = mix(shaded, vec3(1.0), vHighlight);
    fragColor = vec4(highlighted, color.a);
}
