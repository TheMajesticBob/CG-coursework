#version 440 core
#extension GL_EXT_gpu_shader4 : enable

uniform sampler2D tex;

// Triangles table texture
uniform sampler3D dataFieldTex;

in VertexData {
  vec4 colour;
  vec2 tex_coord;
};

layout(location = 0) out vec4 colour_out;

void main() { 
	colour_out = colour; // vec4(0,0,0,1); // vec4( vec3( texture(dataFieldTex, vec3( 0, 0, 0 )).r ), 1.0 ); // vec4( vec3( texelFetch2D( triTableTex, ivec2( 1, 1 ), 0 ).r ), 1.0 );
}