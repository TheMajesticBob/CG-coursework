#version 440 

uniform sampler2D tex;
uniform vec4 colour;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;
layout(location = 3) in vec4 depth;

layout(location = 0) out vec4 position_out;
layout(location = 1) out vec4 diffuse_out;
layout(location = 2) out vec4 normal_out;
layout(location = 3) out vec4 mat_diffuse_out;
layout(location = 4) out vec4 mat_specular_out;
layout(location = 5) out vec4 mat_emissive_out;

void main()
{
	diffuse_out	 = vec4( 0.0 ); 
	mat_emissive_out = texture(tex, tex_coord); // * colour;
	mat_emissive_out.rgb *= colour.rgb; // = 1.0;
}