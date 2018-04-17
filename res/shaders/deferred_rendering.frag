#version 440

uniform sampler2D tPosition;
uniform sampler2D tAlbedo; 
uniform sampler2D tNormals;
uniform sampler2D tMatDiffuse;
uniform sampler2D tMatSpecular;
uniform sampler1D tCellShading;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

// Outgoing colour
layout(location = 0) out vec4 colour;

void main()
{
	vec4 image = texture2D( tAlbedo, tex_coord );
	float lum = 0.2126 * image.r + 0.7152f * image.g + 0.0722f * image.b;

	colour = image; // * texture(tCellShading, clamp( lum, 0.0, 1.0 ));
}
