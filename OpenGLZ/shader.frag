#version 450

in vec3 color_out;

out vec4 color;

void main()
{
    color = vec4(color_out, 1.0);
}