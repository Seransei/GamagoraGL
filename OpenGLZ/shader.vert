#version 450

in vec3 position;
in vec2 uv;

out vec2 uv_out;
out vec4 color_out;

uniform mat4 lookAt;
uniform mat4 perspective;
uniform mat4 transformMatrix;
uniform vec4 color;

void main()
{
    gl_Position = 	 
		perspective *
		lookAt * 
		transformMatrix * 
		vec4(position * 2 - 1, 1.0);

	uv_out = uv;
	color_out = color;
}