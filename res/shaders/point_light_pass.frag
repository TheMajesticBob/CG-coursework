#version 440
struct point_light
{
	vec4 light_colour;
	vec3 position;
	float constant;
	float linear;
	float quadratic;
};

uniform sampler2D tPosition;
uniform sampler2D tAlbedo; 
uniform sampler2D tNormals;
uniform sampler2D tMatDiffuse;
uniform sampler2D tMatSpecular;

uniform point_light gPointLight;
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
	vec3 normal = normalize(Normal.rgb);

	float dist = distance(WorldPos, gPointLight.position);
	// Calculate attenuation factor
	float attentuation = 1 / (gPointLight.constant + gPointLight.linear * dist + gPointLight.quadratic * dist * dist );
	// Calculate light colour
	vec4 light_colour = gPointLight.light_colour * attentuation;
	// Calculate light dir
	vec3 light_dir = normalize( gPointLight.position - WorldPos );
	// Now use standard phong shading but using calculated light colour and direction
	// - note no ambient
	float k;
	// Calculate diffuse component
	k = max( dot( normal, light_dir ), 0 );
	vec4 diffuse = k * Diffuse * gPointLight.light_colour;
	// Calculate view direction
	vec3 view_dir = normalize( gEyeWorldPos - WorldPos );
	// Calculate half vector
	vec3 half_vector = normalize( view_dir + light_dir );
	// Calculate specular component
	k = pow( max( dot( normal, half_vector ), 0 ), Normal.a );
	vec4 specular = k * (Specular * light_colour );
	// Calculate final colour - remember alpha
	diffuse.a = 1;
	specular.a = 1;
	colour = Color * diffuse + specular;
	/*
	// Calculate view direction
	vec3 view_dir = normalize( gEyeWorldPos - WorldPos );
	// Get distance between point light and vertex
	float dist = distance(WorldPos, gPointLight.position);
	// Calculate attenuation factor
	float attentuation = 1 / (gPointLight.constant + gPointLight.linear * dist + gPointLight.quadratic * dist * dist );
	// Calculate light colour
	vec4 light_colour = gPointLight.light_colour * attentuation;
	//Set colour alpha to 1.0
	light_colour.a = 1.0;
	// Calculate light dir
	vec3 light_dir = normalize( gPointLight.position - WorldPos );
	vec3 half_vector = normalize(light_dir + view_dir);

	float intensity = max(dot(Normal, light_dir), 0);
	float specIntensity = max(dot(Normal, half_vector), 0);

	// Now use standard phong shading but using calculated light colour and direction
	// - note no ambient
	vec4 diffuse = (vec4(Diffuse.rgb,1.0) * light_colour) * intensity;
	vec4 specular = (Specular * light_colour) * pow(specIntensity, Diffuse.a);
	colour = diffuse * Color + specular;
	colour.a = 1.0;
	*/
}