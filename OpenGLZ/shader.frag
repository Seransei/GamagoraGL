#version 450

in vec2 uv_out;

out vec4 color;

uniform sampler2D text;

void main()
{	
    color = texture(text, uv_out);
}