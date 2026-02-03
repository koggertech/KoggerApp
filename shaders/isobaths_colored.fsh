#version 140

in  vec3 vertice;
out vec4 fragColor;

uniform sampler2D paletteSampler;
uniform float  depthMin;
uniform float  levelStep;
uniform int    levelCount;
uniform bool  linePass;
uniform vec3  lineColor;
uniform float lineWidth;

void main()
{
    if (linePass) {
        fragColor = vec4(lineColor, 1.0);
        return;
    }

    float relDepth = vertice.z - depthMin;
    float levelIdx = relDepth / levelStep;
    float stepIdx = floor(levelIdx);
    float clampedIdx = clamp(stepIdx, 0.0, float(levelCount - 1));
    float denom = max(float(levelCount - 1), 1.0);
    float norm = clampedIdx / denom;
    vec3 color = texture2D(paletteSampler, vec2(norm, 0.5)).rgb;

    float inRange = step(0.0, stepIdx) * step(stepIdx, float(levelCount - 1));
    float frac = fract(levelIdx);
    float edgeDist = min(frac, 1.0 - frac);
    float width = max(fwidth(levelIdx) * lineWidth, 1e-6);
    float lineMask = (1.0 - smoothstep(0.0, width, edgeDist)) * inRange;

    vec3 outColor = mix(color, lineColor, lineMask);
    fragColor = vec4(outColor, 1.0);
}
