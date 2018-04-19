#version 410

// Cubemap texture
uniform samplerCube cubemap;

// Incoming 3D texture coordinate
layout (location = 0) in vec3 tex_coord;
layout (location = 0) in vec3 normal_in;

// Outgoing colour
layout(location = 0) out vec4 position_out;
layout(location = 1) out vec4 diffuse_out;
layout(location = 2) out vec4 normal_out;
layout(location = 3) out vec4 mat_diffuse_out;
layout(location = 4) out vec4 mat_specular_out;

void main()
{
	position_out = tex_coord;
	diffuse_out = texture(cubemap, tex_coord);
	normal_out = vec4(normal_in,0.0);
	mat_diffuse_out = vec4(0.0);
	mat_specular_out = vec4(0.0);
}