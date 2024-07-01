#version 140

in vec3 vertice;
uniform float max_z;
uniform float min_z;
uniform int selectedPrimitiveFirstIndex;
uniform int selectedPrimitiveLastIndex;

vec3 getColor(float v, float vmin, float vmax)
{
    vec3 c = vec3(1.0f, 1.0f, 1.0f);
    float dv;

    if (v < vmin)
        v = vmin;
    if (v > vmax)
        v = vmax;

    dv = vmax - vmin;

    if (v < (vmin + 0.25f * dv)) {
       c.r = 0.0f;
       c.g = 4.0f * (v - vmin) / dv;
    } else if (v < (vmin + 0.5f * dv)) {
       c.r = 0.0f;
       c.b = 1.0f + 4.0f * (vmin + 0.25f * dv - v) / dv;
    } else if (v < (vmin + 0.75f * dv)) {
       c.r = 4.0f * (v - vmin - 0.5f * dv) / dv;
       c.b = 0.0f;
    } else {
       c.g = 1.0f + 4.0f * (vmin + 0.75f * dv - v) / dv;
       c.b = 0.0f;
    }

    return c;
}

void main()
{
    float norm_z = (vertice.z - min_z) / (abs(max_z - min_z));
    vec4 color = vec4(getColor(norm_z, 0.0f, 1.0f) * 0.95f, 1.0f);
    gl_FragColor = color;
}
