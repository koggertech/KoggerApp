#version 140

in vec3 position;
in vec3 normal;

uniform mat4 matrix;

out vec3 vertice;
out vec3 vertNormal;

void main()
{
    if (isnan(position.x)) {
        gl_Position = vec4(0.0);
        return;
    }

    vertice = position;
    vertNormal = normal;
    gl_Position = matrix * vec4(position, 1.0);
}
