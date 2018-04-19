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
uniform sampler2D tMatEmissive;
uniform sampler2D tMatDiffuse;
uniform sampler2D tMatSpecular;

uniform point_light gPointLight;
uniform vec3 gEyeWorldPos;
uniform vec2 gScreenSize;

uniform float sphereRadius;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

layout(location = 0) out vec4 colour;

vec2 CalcTexCoord()
{
   return gl_FragCoord.xy / gScreenSize;
}

float stepmix(float edge0, float edge1, float E, float x)
{
    float T = clamp(0.5 * (x - edge0 + E) / E, 0.0, 1.0);
    return mix(edge0, edge1, T);
}


void main()
{
   	vec2 TexCoord = CalcTexCoord();
   	vec3 WorldPos = texture(tPosition, TexCoord).xyz;
   	vec4 Color = texture(tAlbedo, TexCoord);
   	vec4 Normal = texture(tNormals, TexCoord);
	vec4 Emissive = texture(tMatEmissive, TexCoord);
	vec4 Diffuse = texture(tMatDiffuse, TexCoord);
	vec4 Specular = texture(tMatSpecular, TexCoord);
	vec3 normal = normalize(Normal.rgb);
	
	const float A = 0.1;
    const float B = 0.3;
    const float C = 0.6;
    const float D = 1.0;
	float E;
	
	// Calculate light dir
	vec3 light_dir = WorldPos - gPointLight.position;
	// Calculate distance
	float dist = length(light_dir);
	// Normalize light direction
	light_dir = -normalize(light_dir);
	// Calculate attenuation factor
	float attentuation = 1 / (gPointLight.constant + gPointLight.linear * dist + gPointLight.quadratic * dist * dist );
	// Calculate light colour
	vec4 light_colour = gPointLight.light_colour * attentuation;
	// Now use standard phong shading but using calculated light colour and direction
	float k;
	// Calculate diffuse component
	k = max( dot( normal, light_dir ), 0 );
	E = fwidth(k);
	// Cell shade
	if (k > A - E && k < A + E) k = stepmix(A, B, E, k);
    else if (k > B - E && k < B + E) k = stepmix(B, C, E, k);
    else if (k > C - E && k < C + E) k = stepmix(C, D, E, k);
    else if (k < A) k = 0.0;
    else if (k < B) k = B;
    else if (k < C) k = C;
    else k = D;

	vec4 diffuse = k * Diffuse * light_colour;
	// Calculate view direction
	vec3 view_dir = normalize( gEyeWorldPos - WorldPos );
	// Calculate half vector
	vec3 half_vector = normalize( view_dir + light_dir );
	// Calculate specular component
	k = pow( max( dot( normal, half_vector ), 0 ), Normal.a );
	// Cell shade
	E = fwidth(k);
	if( k > 0.5 - E && k < 0.5 + E )
	{
		k += smoothstep	(0.5 - E, 0.5 + E, k);
	} else 
	{
		k = step(0.5, k);
	}

	vec4 specular = k * ( Specular * light_colour );
	vec4 primary = Emissive + diffuse;
	// Calculate final colour - remember alpha
	primary.a = 1;
	specular.a = 1;
	colour = Color * primary + specular;
}