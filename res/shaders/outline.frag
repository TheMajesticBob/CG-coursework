#version 430 core

// Incoming texture containing frame information
uniform sampler2D tAlbedo;
uniform sampler2D tNormals;
uniform sampler2D depth;

uniform vec4 outline_colour;
uniform vec2 screen_size;

uniform float blend_value;
uniform float near_distance;
uniform float far_distance;

uniform float flattening_value;

// Debug
uniform int depth_only;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

// Outgoing colour
layout(location = 0) out vec4 colour;

vec2 CalcTexCoord()
{
   return gl_FragCoord.xy / screen_size;
}

void main() {
	// Calculate texel size and coordinate offset for edge detection
	vec2 texelSize = 1.0 / screen_size;
	vec2 blockSize = clamp( ceil( screen_size / 256.0 ), 1.0, 1.5 ) * texelSize;

	vec2 coord = CalcTexCoord();

	// Get neighbour coordinates
	vec2[4] uvs = vec2[]( 
					vec2( blockSize.r, 0.0 ) + tex_coord,
					vec2( 0.0, blockSize.g ) + tex_coord,
					vec2( -blockSize.r, 0.0 ) + tex_coord,
					vec2( 0.0, -blockSize.g ) + tex_coord);

	// Sample main depth texture
	vec4 mainDepth = texture(depth, tex_coord) / 4.0;
	vec4 mainNormal = texture(tNormals, tex_coord);

	// Sample depth texture with offset UVs and subtract them from main depth
	vec4[4] depths = vec4[](
					mainDepth - texture(depth, uvs[0]),
					mainDepth - texture(depth, uvs[1]),
					mainDepth - texture(depth, uvs[2]),
					mainDepth - texture(depth, uvs[3]));

	vec4[4] normals = vec4[](
					mainNormal - texture(tNormals, uvs[0]),
					mainNormal - texture(tNormals, uvs[1]),
					mainNormal - texture(tNormals, uvs[2]),
					mainNormal - texture(tNormals, uvs[3]));
	
	// Add everything together

	vec4 depthSum = vec4(0.0);
	vec4 normalSum = vec4(0.0);

	for( int i = 0; i < 4; ++i )
	{
		depthSum += depths[i];
		normalSum += normals[i];
	}

	vec4 edgeDepth = clamp( max(depthSum*2.0,normalSum) / flattening_value, 0.0, 1.0 ) * blend_value;

	float outlineValue = clamp( pow(length(edgeDepth), 2.0), 0.0, 1.0 );

	if( depth_only == 1 )
	{
		colour = vec4( vec3( outlineValue ), 1.0 );
	} else if( depth_only == 2 )
	{
		colour = edgeDepth;
	} else {
		colour = mix( texture( tAlbedo, tex_coord ), outline_colour, outlineValue );
		colour.a = 1.0;
	}
}