#version 140

in  vec3 vertice;
out vec4 fragColor;

uniform sampler2D paletteSampler;
uniform float  depthMin;
uniform float  levelStep;
uniform int    levelCount;
uniform bool  linePass;
uniform vec3  lineColor;

void main()
{
    if (linePass) {
        fragColor = vec4(lineColor, 1.0);
        return;
    }

    float relDepth = vertice.z - depthMin;
    float stepIdx = floor(relDepth / levelStep);
    float clampedIdx = clamp(stepIdx, 0.0, float(levelCount - 1));
    float norm = clampedIdx / float(levelCount - 1);
    vec3 color = texture2D(paletteSampler, vec2(norm, 0.5)).rgb;
    fragColor = vec4(color, 1.0);
}
