#version 440

uniform sampler2D tPosition;
uniform sampler2D tDiffuse; 
uniform sampler2D tNormals;
uniform sampler2D tTexCoord;

uniform int depthOnly;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

// Outgoing colour
layout(location = 0) out vec4 colour;

void main()
{
	vec4 image = texture2D( tDiffuse, tex_coord );
	vec4 position = texture2D( tPosition, tex_coord );
	vec4 normal = normalize(texture2D( tNormals, tex_coord ));
	vec4 texCoord = texture2D(tTexCoord, tex_coord);

	// Calculate colour to return
	if( depthOnly == 1 )
	{
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
