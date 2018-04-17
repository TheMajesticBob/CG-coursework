#version 440 core
#extension GL_EXT_gpu_shader4 : enable

uniform sampler2D tex;

// Triangles table texture
uniform sampler3D dataFieldTex;

in VertexData {
	vec4 position;
	vec3 normal;
	vec4 colour;
	vec2 tex_coord;
};

// They all were vec4!!
layout(location = 0) out vec4 position_out;
layout(location = 1) out vec4 diffuse_out;
layout(location = 2) out vec4 normal_out;
layout(location = 3) out vec4 mat_diffuse_out;
layout(location = 4) out vec4 mat_specular_out;

void main()
{
	diffuse_out	 = colour;
	position_out = position;
	normal_out	 = vec4(normal,1.0); //calc_normal(normal, tangent, binormal, normal_map, tex_coord);
	mat_diffuse_out	 = vec4(0.7,0.7,0.7,1.0); // diffuse;
	mat_specular_out = vec4(0.0); //specular;
}