#version 140

in vec3 position;
in vec3 normal;

uniform mat4 matrix;
uniform vec3 lightDir;
uniform float ambient;
uniform float intensity;
uniform float highlightIntensity;

out float vBaseShade;
out float vHighlight;

vec3 safeNormalize(vec3 v, vec3 fallback)
{
    float len2 = dot(v, v);
    if (len2 <= 1e-8) {
        return fallback;
    }
    return v * inversesqrt(len2);
}

void main()
{
    vec3 n = safeNormalize(normal, vec3(0.0, 0.0, 1.0));
    vec3 l = safeNormalize(lightDir, vec3(0.0, 0.0, 1.0));
    float ndl = dot(n, l);
    float ndlRef = clamp(l.z, 0.0, 1.0);
    float invRef = 1.0 / max(ndlRef, 1e-4);
    float invRefToOne = 1.0 / max(1.0 - ndlRef, 1e-4);
    float shadowTerm = smoothstep(0.0, 1.0, clamp((ndl - ndlRef) * invRefToOne, 0.0, 1.0));
    float highlightTerm = smoothstep(0.0, 1.0, clamp((ndlRef - ndl) * invRef, 0.0, 1.0));
    float baseShade = ambient + (1.0 - ambient) * (1.0 - intensity * shadowTerm);
    vBaseShade = clamp(baseShade, 0.0, 1.0);
    vHighlight = clamp((1.0 - ambient) * highlightIntensity * highlightTerm, 0.0, 1.0);
    gl_Position = matrix * vec4(position, 1.0);
}
