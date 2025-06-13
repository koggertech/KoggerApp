#version 140

in vec3 position;

uniform mat4 matrix;

varying vec3 vertice;

void main()
{
    if (isnan(position.x)) {
        gl_Position = vec4(0.0);
        return;
    }

    vertice = position;
    gl_Position = matrix * vec4(position, 1.0);
}
