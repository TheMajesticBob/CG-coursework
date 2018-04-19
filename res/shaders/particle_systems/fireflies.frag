#version 440 core
#extension GL_EXT_gpu_shader4 : enable

uniform sampler2D tex;
uniform vec4 colour;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

layout(location = 0) out vec4 position_out;
layout(location = 1) out vec4 diffuse_out;
layout(location = 2) out vec4 normal_out;
layout(location = 3) out vec4 mat_diffuse_out;
layout(location = 4) out vec4 mat_specular_out;
layout(location = 5) out vec4 mat_emissive_out;

void main()
{
	diffuse_out	 = vec4(1.0); //vec4( texture(tex, tex_coord).rgb, 1.0 ) * colour;
	//mat_emissive_out = diffuse_out;
}