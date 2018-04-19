#version 410

// MVP transformation matrix
uniform mat4 MVP;

// Incoming position
layout (location = 0) in vec3 position;
// Incoming normal
layout (location = 2) in vec3 normal;

// Outgoing 3D texture coordinate
layout (location = 0) out vec3 tex_coord;
layout (location = 1) out vec3 normal_out;

void main()
{
	// Calculate screen space position
	gl_Position = MVP * vec4(position, 1.0);

	tex_coord = position;
	normal_out = normal;
}