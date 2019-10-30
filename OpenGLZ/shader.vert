#version 450

in vec3 position;
in vec3 color;

out vec3 color_out;

uniform float scale;

void main()
{
    gl_Position = vec4(position * scale, 1.0);

	color_out = color;
}