#version 440 core
uniform sampler2D tAlbedo;

layout (location = 0) in vec2 tex_coord;

layout (location = 0) out vec4 colour;

void main()
{
	colour = texture(tAlbedo, tex_coord);
	colour.a = 1.0;
}