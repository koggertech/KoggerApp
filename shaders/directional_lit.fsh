#version 140

uniform vec4 color;
in float vBaseShade;
in float vHighlight;

void main()
{
    vec3 shaded = color.rgb * vBaseShade;
    vec3 highlighted = mix(shaded, vec3(1.0), vHighlight);
    gl_FragColor = vec4(highlighted, color.a);
}
