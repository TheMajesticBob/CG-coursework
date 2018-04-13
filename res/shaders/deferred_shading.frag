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

// They all were vec4!!
layout(location = 0) out vec4 position_out;
layout(location = 1) out vec4 diffuse_out;
layout(location = 2) out vec4 normal_out;
layout(location = 3) out vec4 tex_coord_out;

vec3 calc_normal(in vec3 normal, in vec3 tangent, in vec3 binormal, in sampler2D normal_map, in vec2 tex_coord);

void main()
{
	diffuse_out	 = texture2D(tex_diffuse,tex_coord);
	position_out = position;
	normal_out	 = vec4(calc_normal(normal, tangent, binormal, normal_map, tex_coord), 1.0);
    tex_coord_out = vec4(tex_coord, 0.0, 0.0); 
}
