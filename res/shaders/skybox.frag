#version 410

// Cubemap texture
uniform samplerCube cubemap;

// Incoming 3D texture coordinate
layout (location = 0) in vec3 tex_coord;

// Outgoing colour
layout (location = 1) out vec4 colour;
layout (location = 2) out vec4 normal;

void main()
{
	colour = texture(cubemap, tex_coord);
	normal = vec4(0.0);
}