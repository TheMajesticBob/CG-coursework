#version 440
struct material {
  vec4 emissive;
  vec4 diffuse_reflection;
  vec4 specular_reflection;
  float shininess;
};

// Model transformation matrix (World matrix)
uniform mat4 M;
// Transformation matrix
uniform mat4 MVP;
// Normal matrix
uniform mat3 N;
// Material
uniform material mat;

// Incoming position
layout (location = 0) in vec3 position;
// Incoming normal
layout (location = 2) in vec3 normal;
// Incoming binormal
layout(location = 3) in vec3 binormal;
// Incoming tangent
layout(location = 4) in vec3 tangent;
// Incoming texture coordinate
layout (location = 10) in vec2 tex_coord_in;

// Outgoing position
layout (location = 0) out vec4 vertex_position;
// Outgoing texture coordinate
layout (location = 1) out vec2 tex_coord_out;
// Outgoing transformed normal
layout (location = 2) out vec4 transformed_normal;
// Outgoing tangent
layout (location = 3) out vec3 tangent_out;
// Outgoing binormal
layout (location = 4) out vec3 binormal_out;
// Outgoing diffuse
layout (location = 5) out vec4 diffuse_out;
// Outgoing specular
layout (location = 6) out vec4 specular_out;

void main( void )
{
	gl_Position		= MVP * vec4( position, 1.0 );
	vertex_position = M * vec4( position, 1.0 );

	// Transform normal
	transformed_normal = vec4( N * normal, mat.shininess);
	// Transform tangent
	tangent_out = N * tangent;
	// Transform binormal
	binormal_out = N * binormal;

	tex_coord_out = tex_coord_in;

	diffuse_out = mat.diffuse_reflection;
	specular_out = mat.specular_reflection;
}