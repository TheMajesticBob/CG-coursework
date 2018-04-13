#version 440
struct point_light
{
	vec4 light_colour;
	vec3 position;
	float constant;
	float linear;
	float quadratic;
};

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

   	FragColor = vec4(Color, 1.0) * calculate_point(WorldPos, Normal);
}

vec2 CalcTexCoord()
{
   return gl_FragCoord.xy / gScreenSize;
}

vec4 calculate_point(in point_light point, in material mat, in vec3 position, in vec3 normal, in vec3 view_dir, in vec4 tex_colour)
{
	// *********************************
	// Get distance between point light and vertex
	float dist = distance(position, point.position);
	// Calculate attenuation factor
	float attentuation = 1 / (point.constant + point.linear * dist + point.quadratic * dist * dist );
	// Calculate light colour
	vec4 light_colour = point.light_colour * attentuation;
	//Set colour alpha to 1.0
	light_colour.a = 1.0;
	// Calculate light dir
	vec3 light_dir = normalize( point.position - position );
	// *********************************
	// Now use standard phong shading but using calculated light colour and direction
	// - note no ambient
	vec4 diffuse = (mat.diffuse_reflection * light_colour) * max(dot(normal, light_dir), 0);
	vec3 half_vector = normalize(light_dir + view_dir);
	vec4 specular = (mat.specular_reflection * light_colour) * pow(max(dot(normal, half_vector), 0), mat.shininess);
	vec4 primary = mat.emissive + diffuse;
	vec4 colour = primary * tex_colour + specular;
	colour.a = 1.0;
	return colour;
}