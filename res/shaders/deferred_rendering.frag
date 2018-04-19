#version 440

uniform sampler2D tPosition;
uniform sampler2D tAlbedo; 
uniform sampler2D tNormals;
uniform sampler2D tMatDiffuse;
uniform sampler2D tMatSpecular;
uniform sampler2D tDepth;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

// Outgoing colour
layout(location = 0) out vec4 colour;

void main()
{
	float a = 1 - step(1.0, texture(tDepth, tex_coord).r);
	if( a == 0 )
	{
		discard;
	}
	colour = texture( tAlbedo, tex_coord );
}
