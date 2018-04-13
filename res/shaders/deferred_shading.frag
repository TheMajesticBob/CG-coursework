#version 440

uniform sampler2D tex_diffuse;
uniform sampler2D normal_map;

// Incoming position
layout(location = 0) in vec4 position;
// Incoming texture coordinate
layout(location = 1) in vec2 tex_coord;
// Incoming normal
layout(location = 2) in vec3 normal;
// Incoming tangent
layout(location = 3) in vec3 tangent;
// Incoming binormal
layout(location = 4) in vec3 binormal;
// Incoming diffuse
layout(location = 5) in vec3 diffuse;
// Incoming specular
layout(location = 6) in vec3 specular;

// They all were vec4!!
layout(location = 0) out vec3 position_out;
layout(location = 1) out vec3 diffuse_out;
layout(location = 2) out vec3 normal_out;
layout(location = 3) out vec3 mat_diffuse_out;
layout(location = 4) out vec3 mat_specular_out;

vec3 calc_normal(in vec3 normal, in vec3 tangent, in vec3 binormal, in sampler2D normal_map, in vec2 tex_coord);

void main()
{
	diffuse_out	 = texture2D(tex_diffuse,tex_coord).xyz;
	position_out = position.xyz;
	normal_out	 = calc_normal(normal, tangent, binormal, normal_map, tex_coord);
	mat_diffuse_out	 = diffuse;
	mat_specular_out = specular;
}
