#version 450

in vec3 position;
in vec3 color;

out vec3 color_out;

uniform mat4 lookAt;

uniform mat4 perspective;

uniform mat4 transformMatrix;

void main()
{
    gl_Position = 	 
		perspective *
		lookAt * 
		transformMatrix * 
		vec4(position, 1.0);

	color_out = color;
}