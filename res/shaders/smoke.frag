#version 440 core
#extension GL_EXT_gpu_shader4 : enable

uniform sampler2D tex;

in VertexData {
	vec4 position;
	vec3 normal;
	vec4 colour;
	vec2 tex_coord;
};

layout(location = 0) out vec4 position_out;
layout(location = 1) out vec4 diffuse_out;
layout(location = 2) out vec4 normal_out;
layout(location = 3) out vec4 mat_diffuse_out;
layout(location = 4) out vec4 mat_specular_out;

void main()
{
	diffuse_out	 = colour;
	position_out = position;
	normal_out	 = vec4(normal,1.0);
	mat_diffuse_out	 = vec4(0.9,0.9,0.9, 0.95);
	mat_specular_out = vec4(0.1, 0.1, 0.1, 1.0);
}