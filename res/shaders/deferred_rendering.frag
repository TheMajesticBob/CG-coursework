#version 440

uniform sampler2D tDiffuse; 
uniform sampler2D tPosition;
uniform sampler2D tNormals;
uniform sampler2D tTexCoord;
uniform vec3 cameraPosition;

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
	
	vec3 light = vec3(50,100,50);
	vec3 light_dir = normalize(light - position.xyz);
	vec4 light_colour = vec4(1.0, 1.0, 1.0, 1.0);
	vec4 ambient_intensity = vec4(1.2);
	
	vec3 view_dir = normalize(cameraPosition-position.xyz);

	vec4 tex_colour = vec4( image.rgb, 1.0 );
	
	// Calculate ambient component
	vec4 ambient = tex_colour * ambient_intensity;
	// Calculate diffuse component :  (diffuse reflection * light_colour) *  max(dot(normal, light direction), 0)
	vec4 diffuse = tex_colour * light_colour * max( dot( normal.rgb, light_dir ), 0.0 );
	// Calculate normalized half vector 
	vec3 half_vector = normalize( view_dir + light_dir );
	// Calculate specular component : (specular reflection * light_colour) * (max(dot(normal, half vector), 0))^mat.shininess
	vec4 specular = ( image.a * light_colour ) * pow( max( dot( normal.xyz, half_vector ), 0.0 ), 100 );
 // *********************************
	// Calculate colour to return
	if( depthOnly == 1 )
	{
		colour = vec4( image.r );
	} else {
		colour = texCoord; // ((ambient + diffuse) * tex_colour) + specular;
	}
	
    const float gamma = 2.2;
    vec3 hdrColor = colour.rgb;
  
    // reinhard tone mapping
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));

    // gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
  
    // colour = vec4(mapped, 1.0);
}
