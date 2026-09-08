#version 140

in  vec3 vertice;
in  vec3 vertNormal;
out vec4 fragColor;

uniform sampler2D paletteSampler;
uniform float  depthMin;
uniform float  levelStep;
uniform int    levelCount;
uniform bool  linePass;
uniform vec3  lineColor;
uniform vec3  casingColor;
uniform float casingWidth;
uniform float casingTint;
uniform float lineWidth;
uniform vec3  lightDir;
uniform float shadowAmbient;
uniform float shadowIntensity;
uniform float highlightIntensity;
uniform bool shadowsEnabled;

vec3 safeNormalize(vec3 v, vec3 fallback)
{
    float len2 = dot(v, v);
    if (!(len2 > 1e-8)) {
        return fallback;
    }
    return v * inversesqrt(len2);
}

vec3 applyDirectionalShade(vec3 baseColor)
{
    if (!shadowsEnabled) {
        return baseColor;
    }

    vec3 n = safeNormalize(vertNormal, vec3(0.0, 0.0, 1.0));
    if (n.z < 0.0) {
        n = -n;
    }
    vec3 l = safeNormalize(lightDir, vec3(0.0, 0.0, 1.0));
    float ndl = dot(n, l);
    float ndlRef = clamp(l.z, 0.0, 1.0);
    float invRef = 1.0 / max(ndlRef, 1e-4);
    float invRefToOne = 1.0 / max(1.0 - ndlRef, 1e-4);
    float shadowTerm = smoothstep(0.0, 1.0, clamp((ndlRef - ndl) * invRef, 0.0, 1.0));
    float highlightTerm = smoothstep(0.0, 1.0, clamp((ndl - ndlRef) * invRefToOne, 0.0, 1.0));
    float baseShade = shadowAmbient + (1.0 - shadowAmbient) * (1.0 - shadowIntensity * shadowTerm);
    float highlightGain = (1.0 - shadowAmbient) * highlightIntensity * highlightTerm;
    float shade = clamp(baseShade + highlightGain, 0.0, 1.5);
    return baseColor * shade;
}

void main()
{
    if (linePass) {
        fragColor = vec4(applyDirectionalShade(lineColor), 1.0);
        return;
    }

    float relDepth = vertice.z - depthMin;
    float levelIdx = relDepth / levelStep;
    float stepIdx = floor(levelIdx);
    float denom = max(float(levelCount), 1.0);
    float norm = clamp(levelIdx / denom, 0.0, 1.0);
    vec3 color = texture2D(paletteSampler, vec2(norm, 0.5)).rgb;

    float inRange = step(0.0, stepIdx) * step(stepIdx, float(levelCount - 1));
    float frac = fract(levelIdx);
    float edgeDist = min(frac, 1.0 - frac);
    const float gradEps = 1e-5;
    float grad = fwidth(levelIdx);
    float pixDist = edgeDist / max(grad, gradEps);
    float lineGate = inRange * step(gradEps, grad);
    float coreMask = (1.0 - smoothstep(0.0, lineWidth, pixDist)) * lineGate;
    float casingMask = (1.0 - smoothstep(0.0, lineWidth + casingWidth, pixDist)) * lineGate;

    vec3 casingInk = mix(casingColor, color, casingTint);

    vec3 outColor = mix(color, casingInk, casingMask);
    outColor = mix(outColor, lineColor, coreMask);
    outColor = applyDirectionalShade(outColor);
    fragColor = vec4(outColor, 1.0);
}
