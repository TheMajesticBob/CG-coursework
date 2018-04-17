#version 440 core
#extension GL_EXT_geometry_shader4 : enable
#extension GL_EXT_gpu_shader4 : enable

out VertexData
{
	vec4 position;
	vec3 normal;
	vec4 colour;
	vec2 tex_coord;
};

// MVP matrix
uniform mat4 MVP;

uniform vec3 bounds;

uniform int gMetaballCount;

layout(points) in;
layout(triangle_strip, max_vertices = 16) out;

layout(location = 0) in float height[];

// Triangles table texture
uniform isampler2D triTableTex;
// Global iso level
uniform float isolevel;
// Marching cubes vertices decal
uniform vec3 vertDecals[8];

layout(std430, binding = 0) buffer PositionBuffer { vec4 positions[]; };

//Get vertex i position within current marching cube
vec3 cubePos( in int i )
{
	return gl_in[0].gl_Position.xyz + vertDecals[i];
}

//Get vertex i value within current marching cube
float cubeVal( in int i, out vec3 nor )
{
	float value = 0.0;
	nor = vec3(0.0);
	for( int j = 0; j < gMetaballCount; ++j )
	{
		float dist = distance( cubePos(i), positions[j].xyz );
		value += positions[j].w * positions[j].w / ( dist * dist );

		vec3 n = 2.0 * (cubePos(i)-positions[j].xyz);
		nor += n * positions[j].w * positions[j].w / ( dist * dist * dist * dist );
	}

	nor = normalize(nor);
	return value;
}

//Get triangle table value
int triTableValue( in int i, in int j )
{
	return texelFetch2D( triTableTex, ivec2( j, i ), 0 ).r;
}

//Compute interpolated vertex along an edge
vec3 vertexInterp( in float isolevel, in vec3 v0, in float l0, in vec3 v1, in float l1 )
{
	return mix( v0, v1, ( isolevel - l0 ) / ( l1 - l0 ) );
}

//Geometry Shader entry point
void main(void) 
{  
	float cubeVals[8];
	vec3 cubeNormals[8];

	for( int i = 0; i < 8; ++i )
	{
		cubeVals[i] = cubeVal(i, cubeNormals[i]);
	}
	
	int cubeIndex = 0;
	//Determine the index into the edge table which tells us which vertices are inside of the surface
	for( int i = 0; i < 8; ++i )
	{
		// cubeIndex += (int(cubeVals[i] < isolevel) << i);
	}
	cubeIndex = int(cubeVals[0] < isolevel);
	cubeIndex += int(cubeVals[1] < isolevel)*2;
	cubeIndex += int(cubeVals[2] < isolevel)*4;
	cubeIndex += int(cubeVals[3] < isolevel)*8;
	cubeIndex += int(cubeVals[4] < isolevel)*16;
	cubeIndex += int(cubeVals[5] < isolevel)*32;
	cubeIndex += int(cubeVals[6] < isolevel)*64;
	cubeIndex += int(cubeVals[7] < isolevel)*128;

	//Cube is entirely in/out of the surface
	if( cubeIndex == 0 || cubeIndex == 255 )
		return;

	vec3 vertlist[12];
	vec3 vertNorms[12];
	ivec2 vertPairs[12] = ivec2[12]( ivec2(0,1), ivec2(1,2), ivec2(2,3), ivec2(3,0), 
									ivec2(4,5), ivec2(5,6), ivec2(6,7), ivec2(7,4),
									ivec2(0,4), ivec2(1,5), ivec2(2,6), ivec2(3,7));

	//Find the vertices where the surface intersects the cube
	for( int i = 0; i < 12; ++i )
	{
		int a = vertPairs[i].x;
		int b = vertPairs[i].y;
		vertlist[i] = vertexInterp(isolevel, cubePos(a), cubeVals[a], cubePos(b), cubeVals[b]);
		vertNorms[i] = vertexInterp(isolevel, cubeNormals[a], cubeVals[a], cubeNormals[b], cubeVals[b]);
	}


	int i=0;
	// Strange bug with this way, uncomment to test
	for (i=0; triTableValue(cubeIndex, i)!=-1; i+=3) {
	//while(true)	{
		if( triTableValue(cubeIndex, i) != -1 )
		{
			// Generate first vertex of triangle//
			// Fill position varying attribute for fragment shader
			// Fill gl_Position attribute for vertex raster space position
			  // fire temperature
			  float temp = clamp(2.0 / (2.0 / (height[0])), 0.0, 1.0);
			  // scale between white and red
			  vec4 fire_colour = mix(vec4(1., .98, .42, 1.), vec4(0.88, .35, 0., 1.), temp);
			  // and then between red and black
			  fire_colour = mix(fire_colour, vec4(0), height[0] - 1.0);
			  // fade smoke out near top
			  //fire_colour.a = clamp((2.0 - height[0]) / 3.0, 0.0, 1.0);
			  colour = fire_colour;

			  float height = vec4( vertlist[ triTableValue(cubeIndex, i) ], 1).y;

			vec4 pos[3];
			vec3 nor[3];
			for( int j = 0; j < 3; ++j )
			{
				gl_Position = MVP * vec4(vertlist[triTableValue(cubeIndex, i+j)], 1);
				position = gl_Position;
				normal = vertNorms[triTableValue(cubeIndex, i+j)];
				colour = mix( vec4(0.5,0.6,0.6,1.0), vec4(1.0), height / bounds.y );
				colour.a = 0.8;
				EmitVertex();
			}

			//End triangle strip at first triangle
			EndPrimitive();
		}
		else
		{
			break;
		}
		//i = i + 3; //Comment it for testing the strange bug
	}
}