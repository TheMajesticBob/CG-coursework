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

uniform point_light gPointLight;
uniform vec3 gEyeWorldPos;
uniform vec2 gScreenSize;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

layout(location = 0) out vec4 colour;

vec4 CalcLightInternal(point_light Light,
					   vec3 LightDirection,
					   vec3 WorldPos,
					   vec3 Normal)
{
    vec4 AmbientColor = Light.light_colour;
    float DiffuseFactor = dot(Normal, -LightDirection);

    vec4 DiffuseColor  = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);

    if (DiffuseFactor > 0.0) {
        DiffuseColor = Light.light_colour * DiffuseFactor;

        vec3 VertexToEye = normalize(gEyeWorldPos - WorldPos);
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));
        float SpecularFactor = dot(VertexToEye, LightReflect);        
        if (SpecularFactor > 0.0) {
            SpecularFactor = pow(SpecularFactor, 2.0);
            SpecularColor = Light.light_colour * SpecularFactor;
        }
    }

    return (AmbientColor + DiffuseColor + SpecularColor);
}

vec4 CalcPointLight(vec3 WorldPos, vec3 Normal)
{
    vec3 LightDirection = WorldPos - gPointLight.position;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);

    vec4 Color = CalcLightInternal(gPointLight, LightDirection, WorldPos, Normal);

    float Attenuation =  gPointLight.constant +
                         gPointLight.linear * Distance +
                         gPointLight.quadratic * Distance * Distance;

    Attenuation = max(1.0, Attenuation);

    return Color / Attenuation;
}

void main()
{
   	vec2 TexCoord = tex_coord;
   	vec3 WorldPos = texture(tPosition, TexCoord).xyz;
   	vec3 Color = texture(tAlbedo, TexCoord).xyz;
   	vec3 Normal = texture(tNormals, TexCoord).xyz;

   	// Normal = normalize(Normal);

   	colour = vec4(Color, 1.0); // * CalcPointLight(WorldPos, Normal);
}