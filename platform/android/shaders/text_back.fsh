#version 300 es
precision mediump float;

uniform vec4 color;
uniform float radius;

in vec2 v_local;
in vec2 v_half;

out vec4 fragColor;

void main()
{
    float r = min(radius, min(v_half.x, v_half.y));
    vec2 q = abs(v_local) - v_half + r;
    float d = min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - r;
    float alpha = 1.0 - smoothstep(-0.5, 0.5, d);
    if (alpha <= 0.0) {
        discard;
    }
    fragColor = vec4(color.rgb, color.a * alpha);
}
