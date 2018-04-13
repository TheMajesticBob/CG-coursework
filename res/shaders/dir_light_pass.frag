#version 440

uniform sampler2D tDiffuse; 
uniform sampler2D tPosition;
uniform sampler2D tNormals;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

void main()
{
   	vec2 TexCoord = CalcTexCoord();
   	vec3 WorldPos = texture(tPosition, TexCoord).xyz;
   	vec3 Color = texture(tDiffuse, TexCoord).xyz;
   	vec3 Normal = texture(tNormals, TexCoord).xyz;
   	Normal = normalize(Normal);

   	FragColor = vec4(Color, 1.0) * CalcPointLight(WorldPos, Normal);
}

vec2 CalcTexCoord()
{
   return gl_FragCoord.xy / gScreenSize;
}