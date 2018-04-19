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
uniform sampler2D tMatEmissive;
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
   	vec4 Normal = texture(tNormals, TexCoord);
	vec4 Diffuse = texture(tMatDiffuse, TexCoord);
	vec4 Specular = texture(tMatSpecular, TexCoord);
	vec4 Emissive = texture(tMatEmissive, TexCoord);
	vec3 normal = normalize(Normal.rgb);
	
	const float A = 0.1;
    const float B = 0.3;
    const float C = 0.6;
    const float D = 1.0;

	float k;
	vec4 ambient = Diffuse * gDirectionalLight.ambient_intensity;
	// Calculate diffuse component
	k = max( dot( normal, gDirectionalLight.light_dir ), 0 );
    if (k < A) k = 0.0;
    else if (k < B) k = B;
    else if (k < C) k = C;
    else k = D;

	vec4 diffuse = k * Diffuse * gDirectionalLight.light_colour;
	// Calculate view direction
	vec3 view_dir = normalize( gEyeWorldPos - WorldPos );
	// Calculate half vector
	vec3 half_vector = normalize( view_dir + gDirectionalLight.light_dir );
	// Calculate specular component
	k = pow( max( dot( normal, half_vector ), 0 ), Normal.a );
	k = step(0.5, k);
	vec4 specular = k * (Specular * gDirectionalLight.light_colour );
	// Calculate primary colour component
	vec4 primary = Emissive + ambient + diffuse;
	// Calculate final colour - remember alpha
	primary.a = 1;
	specular.a = 1;
	colour = Color * primary + specular;
}