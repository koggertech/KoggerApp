#version 140
in vec3 vertice;
//flat in highp int verticeId;
uniform float max_y;
uniform float min_y;
uniform int selectedPrimitiveFirstIndex;
uniform int selectedPrimitiveLastIndex;

vec3 getColor(float v,float vmin, float vmax)
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
    //if(verticeId >= selectedPrimitiveFirstIndex && verticeId <= selectedPrimitiveLastIndex){
    //    gl_FragColor = vec4(1.0f, 0.0f, 1.0f, 1.0f);
    //}else{
        float norm_y = (vertice.y - min_y) / (abs(max_y - min_y));
        vec4 color = vec4(getColor(norm_y, 0.0f, 1.0f), 1.0f);
        gl_FragColor = color;
    //}
};
