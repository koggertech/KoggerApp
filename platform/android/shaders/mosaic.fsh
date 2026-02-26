#version 300 es

precision highp float;

in vec2 vTexCoord;
in float vHeight;
in vec3 vNormal;

uniform sampler2D indexedTexture;
uniform sampler2D colorTable;
uniform vec3 lightDir;
uniform float shadowAmbient;
uniform float shadowIntensity;
uniform float highlightIntensity;
uniform bool shadowsEnabled;

out vec4 fragColor;

vec3 safeNormalize(vec3 v, vec3 fallback)
{
    float len2 = dot(v, v);
    if (!(len2 > 1e-8)) {
        return fallback;
    }
    return v * inversesqrt(len2);
}

void main()
{
    float index = texture(indexedTexture, vTexCoord).r;

    if (index == 0.0) {
        discard;
    }
    else {
        vec4 baseSample = texture(colorTable, vec2(clamp(index, 0.0, 1.0), 0.5));
        vec3 baseColor = baseSample.rgb;

        if (!shadowsEnabled) {
            fragColor = baseSample;
            return;
        }

        vec3 n = safeNormalize(vNormal, vec3(0.0, 0.0, 1.0));
        if (n.z < 0.0) {
            n = -n;
        }
        vec3 l = safeNormalize(lightDir, vec3(0.0, 0.0, 1.0));
        float ndl = dot(n, l);
        float ndlRef = clamp(l.z, 0.0, 1.0);
        float invRef = 1.0 / max(ndlRef, 1e-4);
        float invRefToOne = 1.0 / max(1.0 - ndlRef, 1e-4);
        float shadowTerm = smoothstep(0.0, 1.0, clamp((ndl - ndlRef) * invRefToOne, 0.0, 1.0));
        float highlightTerm = smoothstep(0.0, 1.0, clamp((ndlRef - ndl) * invRef, 0.0, 1.0));
        float baseShade = shadowAmbient + (1.0 - shadowAmbient) * (1.0 - shadowIntensity * shadowTerm);
        float highlightGain = (1.0 - shadowAmbient) * highlightIntensity * highlightTerm;
        float shade = clamp(baseShade + highlightGain, 0.0, 1.5);

        fragColor = vec4(baseColor * shade, baseSample.a);
    }
}
