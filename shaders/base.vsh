#version 140
in vec3 position;
uniform mat4 matrix;
uniform float width;
out vec3 vertice;
flat out highp int verticeId;

void main()
{
    vertice = position;
    //verticeId = gl_VertexID;
    gl_PointSize = width;
    gl_Position = matrix * vec4(position, 1.0);
}
