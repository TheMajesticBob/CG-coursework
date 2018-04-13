#version 440

uniform sampler2D tPosition;
uniform sampler2D tAlbedo; 
uniform sampler2D tNormals;
uniform sampler2D tMatDiffuse;
uniform sampler2D tMatSpecular;

uniform int depthOnly;
uniform float exposure;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

// Outgoing colour
layout(location = 0) out vec4 colour;

void main()
{
	vec4 image = texture2D( tAlbedo, tex_coord );
		
    const float gamma = 2.2;
    vec3 hdrColor = image.rgb;
  
    // reinhard tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

    // gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
  
    colour = vec4(mapped, 1.0);
}
