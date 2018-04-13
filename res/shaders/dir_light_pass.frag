#version 440
struct directional_light
{
	vec4 ambient_intensity;
	vec4 light_colour;
	vec3 light_dir;
};

uniform sampler2D tDiffuse; 
uniform sampler2D tPosition;
uniform sampler2D tNormals;

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

vec4 CalcLightInternal(directional_light Light,
					   vec3 LightDirection,
					   vec3 WorldPos,
					   vec3 Normal)
{
    vec4 AmbientColor = Light.light_colour * Light.ambient_intensity;
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

vec4 CalcDirectionalLight(vec3 WorldPos, vec3 Normal)
{
    return CalcLightInternal(gDirectionalLight,
							 gDirectionalLight.light_dir,
							 WorldPos,
							 Normal);
}

void main()
{
   	vec2 TexCoord = CalcTexCoord();
   	vec3 WorldPos = texture(tPosition, TexCoord).xyz;
   	vec3 Color = texture(tDiffuse, TexCoord).xyz;
   	vec3 Normal = texture(tNormals, TexCoord).xyz;
   	Normal = normalize(Normal);

   	colour = vec4(Color, 1.0) * CalcDirectionalLight(WorldPos, Normal);
}