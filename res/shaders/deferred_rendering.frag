#version 440

uniform sampler2D tPosition;
uniform sampler2D tAlbedo; 
uniform sampler2D tNormals;
uniform sampler2D tMatDiffuse;
uniform sampler2D tMatSpecular;

uniform int depthOnly;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

// Outgoing colour
layout(location = 0) out vec4 colour;

void main()
{
	vec4 image = texture2D( tAlbedo, tex_coord );
	vec4 position = texture2D( tPosition, tex_coord );
	vec4 normal = texture2D( tNormals, tex_coord );
	vec4 diffuse = texture2D( tMatDiffuse, tex_coord );
	vec4 specular = texture2D( tMatSpecular, tex_coord );

	// Calculate colour to return
	if( depthOnly == 1 )
	{
		colour = position;
	} else if( depthOnly == 2 ){
		colour = normal;
	} else {
		colour = image;
	}
	
    const float gamma = 2.2;
    vec3 hdrColor = colour.rgb;
  
    // reinhard tone mapping
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));

    // gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
  
    // colour = vec4(mapped, 1.0);
}
