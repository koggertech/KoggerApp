#version 330 core

in vec2 vTexCoord;
in float vHeight;

uniform sampler2D texture;

out vec4 fragColor;

void main()
{
    vec4 texColor = texture(texture, vTexCoord);


    float normalizedHeight = (vHeight + 50.0) / 100.0;
    vec3 color = mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), normalizedHeight);

    fragColor = vec4(color, 1.0);
    // когда есть текстура то texColor *
}
