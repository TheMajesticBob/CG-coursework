#version 440

struct material {
  vec4 emissive;
  vec4 diffuse_reflection;
  vec4 specular_reflection;
  float shininess;
};

// MVP transformation matrix
uniform mat4 MVP;
// M transformation matrix
uniform mat4 M;
// N transformation matrix
uniform mat3 N;
// Material
uniform material mat;

// Incoming position
layout(location = 0) in vec3 position;
// Incoming normal
layout(location = 2) in vec3 normal;
// Incoming texture coordinate
layout(location = 10) in vec2 tex_coord;
// Incoming texture weight
layout(location = 11) in vec4 tex_weight;

// Outgoing vertex position
layout(location = 0) out vec4 vertex_position;
// Transformed normal
layout(location = 1) out vec4 transformed_normal;
// Outgoing tex_coord
layout(location = 2) out vec2 vertex_tex_coord;
// Outgoing tex_weight
layout(location = 3) out vec4 vertex_tex_weight;
// Outgoing diffuse
layout (location = 4) out vec4 diffuse_out;
// Outgoing specular
layout (location = 5) out vec4 specular_out;

void main() {
	// Calculate screen position
	gl_Position = MVP * vec4(position, 1.0);
	// Calculate vertex world position
	vertex_position = (M * vec4(position, 1.0));
	// Transform normal
	transformed_normal = vec4( N * normal, mat.shininess);
	// Pass through tex_coord
	vertex_tex_coord = tex_coord;
	// Pass through tex_weight
	vertex_tex_weight = tex_weight;
	diffuse_out = mat.diffuse_reflection;
	specular_out = mat.specular_reflection;

}