#version 450

in vec2 uv_out;
in vec4 color_in;

out vec4 color;

uniform sampler2D text;

void main()
{	
//  color = color_in;
	color = texture(text, uv_out);
}