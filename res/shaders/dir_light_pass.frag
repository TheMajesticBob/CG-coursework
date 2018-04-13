#version 440
struct directional_light
{
	vec4 ambient_intensity;
	vec4 light_colour;
	vec3 light_dir;
};

uniform sampler2D tPosition;
uniform sampler2D tAlbedo; 
uniform sampler2D tNormals;
uniform sampler2D tMatDiffuse;
uniform sampler2D tMatSpecular;

uniform directional_light gDirectionalLight;
uniform vec3 gEyeWorldPos;
uniform vec2 gScreenSize;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

layout(location = 0) out vec4 colour;

vec2 CalcTexCoord()
{
   return gl_FragCoord.xy / gScreenSize;
}

void main()
{
	vec2 TexCoord = CalcTexCoord();
   	vec3 WorldPos = texture(tPosition, TexCoord).xyz;
   	vec4 Color = texture(tAlbedo, TexCoord);
   	vec3 Normal = texture(tNormals, TexCoord).xyz;
	vec4 Diffuse = texture(tMatDiffuse, TexCoord);
	vec4 Specular = texture(tMatSpecular, TexCoord);
	Normal = normalize(Normal);

	// Calculate view direction
	vec3 view_dir = normalize( gEyeWorldPos - WorldPos );
	// Calculate ambient component
	vec4 ambient = vec4(Diffuse.rgb,1.0) * gDirectionalLight.ambient_intensity;
	// Calculate diffuse component 
	vec4 diffuse = ( vec4(Diffuse.rgb,1.0) * gDirectionalLight.light_colour) * max( dot( Normal, gDirectionalLight.light_dir ), 0.0 );
	// Calculate normalized half vector 
	vec3 half_vector = normalize( view_dir + gDirectionalLight.light_dir );
	// Calculate specular component
	vec4 specular = ( Specular * gDirectionalLight.light_colour ) * pow( max( dot( Normal, half_vector ), 0.0 ), Diffuse.a );
	// Calculate colour to return
	colour = ((ambient + diffuse) * Color) + specular;
	colour.a = 1.0;
}