#version 430 core

// Incoming texture containing frame information
uniform sampler2D tex;

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

void main() {
	// Calculate texel size and coordinate offset for edge detection
	vec2 texelSize = 1.0 / screen_size;
	vec2 blockSize = clamp( ceil( screen_size / 256.0 ), 1.0, 2.0 ) * texelSize;

	// Get neighbour coordinates
	vec2[4] uvs = vec2[]( 
					vec2( blockSize.r, 0.0 ) + tex_coord,
					vec2( 0.0, blockSize.g ) + tex_coord,
					vec2( -blockSize.r, 0.0 ) + tex_coord,
					vec2( 0.0, -blockSize.g ) + tex_coord);

	// Sample main depth texture
	vec4 mainDepth = texture(depth, tex_coord);

	// Sample depth texture with offset UVs and subtract them from main depth
	vec4[4] depths = vec4[](
					mainDepth - texture(depth, uvs[0]),
					mainDepth - texture(depth, uvs[1]),
					mainDepth - texture(depth, uvs[2]),
					mainDepth - texture(depth, uvs[3]));

	// Add everything together

	vec4 edgeDepth = clamp( (depths[0] + depths[1] + depths[2] + depths[3]) / flattening_value, 0.0, 1.0 ) * blend_value;

	// Calculate falloff (to not render outlines for objects too close or far from the camera)
	float near = clamp( 1.0 - (mainDepth.r / near_distance ), 0.25, 1.0 );
	float far = clamp( 1.0 - (mainDepth.r / far_distance ), 0.0, 1.0 );

	float falloffValue = mix( near, far, blend_value );

	float outlineValue = clamp( edgeDepth.r * falloffValue, 0.0, 1.0 );

	if( depth_only == 1 )
	{
		colour = vec4( vec3( outlineValue ), 1.0 );
	} else if( depth_only == 2 )
	{
		colour = vec4( vec3( falloffValue ), 1.0 );
	} else {
		colour = mix( texture( tex, tex_coord ), outline_colour, outlineValue );
		colour.a = 1.0;
	}
}