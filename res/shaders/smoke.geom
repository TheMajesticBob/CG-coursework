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

layout(points) in;
layout(triangle_strip, max_vertices = 16) out;

// Triangles table texture
uniform isampler2D triTableTex;
// Global iso level
uniform float isolevel;
// Marching cubes vertices decal
uniform vec3 vertDecals[8];
// MVP matrix
uniform mat4 MVP;
// Particle vars
uniform vec4 startColour;
uniform vec4 endColour;
uniform vec2 initialLifetime;
uniform int gMetaballCount;

layout(std430, binding = 0) buffer PositionBuffer { vec4 positions[]; };
layout(std430, binding = 1) buffer VelocityBuffer { vec4 velocities[]; };

//Get vertex i position within current marching cube
vec3 cubePos( in int i )
{
	return gl_in[0].gl_Position.xyz + vertDecals[i];
}

//Get vertex i value within current marching cube
float cubeVal( in int i, out vec3 nor, out vec4 col )
{
	float value = 0.0;
	nor = vec3(0.0);
	col = vec4(0.0);
	for( int j = 0; j < gMetaballCount; ++j )
	{
		float dist = distance( cubePos(i), positions[j].xyz );
		value += positions[j].w * positions[j].w / ( dist * dist );

		vec3 n = 2.0 * (cubePos(i)-positions[j].xyz);
		nor += n * positions[j].w * positions[j].w / ( dist * dist * dist * dist );
		col += clamp( value, -1.0, 0.0 ) * -mix( startColour, endColour, positions[j].y / 1.0f );
	}

	col /= value;
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
	vec4 outputColour = vec4(0.0);

	for( int i = 0; i < 8; ++i )
	{
		vec4 tempColour;
		cubeVals[i] = cubeVal(i, cubeNormals[i], tempColour);
		outputColour += tempColour;
	}

	// outputColour = normalize(outputColour);
	
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
			// Generate triangle vertices
			// Fill position varying attribute for fragment shader
			// Fill gl_Position attribute for vertex raster space position
			vec4 pos[3];
			vec3 nor[3];
			for( int j = 0; j < 3; ++j )
			{
				position = vec4(vertlist[triTableValue(cubeIndex, i+j)], 1);
				gl_Position = MVP * position;
				normal = vertNorms[triTableValue(cubeIndex, i+j)];
				colour = outputColour; // mix( startColour, endColour, height / bounds.y );
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