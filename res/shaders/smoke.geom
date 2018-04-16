#version 440 core
#extension GL_EXT_geometry_shader4 : enable
#extension GL_EXT_gpu_shader4 : enable

out VertexData
{
	vec4 colour;
	vec2 tex_coord;
};

// MVP matrix
uniform mat4 MVP;
uniform mat4 MV;
uniform mat4 P;

uniform vec3 bounds;

uniform int gMetaballCount;

layout(points) in;
layout(triangle_strip, max_vertices = 16) out;

layout(location = 0) in float height[];
//layout(location = 1) in vec4 force[];

// Volume data field texture
uniform sampler3D dataFieldTex;
// Edge table texture
uniform isampler2D edgeTableTex;
// Triangles table texture
uniform isampler2D triTableTex;
// Global iso level
uniform float isolevel;
// Marching cubes vertices decal
uniform vec3 vertDecals[8];
// Vertices position for fragment shader
//layout(location = 0) in vec4 position;

layout(std430, binding = 0) buffer PositionBuffer { vec4 positions[]; };

//Get vertex i position within current marching cube
vec3 cubePos( in int i )
{
	return gl_in[0].gl_Position.xyz + vertDecals[i];
}

//Get vertex i value within current marching cube
float cubeVal( in int i )
{
	float value = 0.0;
	for( int j = 0; j < gMetaballCount; ++j )
	{
		float dist = distance( cubePos(i), positions[j].xyz );
		value += positions[j].w * positions[j].w / ( dist * dist );
	}

	// float dist = distance( cubePos(i), vec3(0.0) ); // +
	// float dist2 = distance( cubePos(i), vec3(1.0) );
	// return 1 / (dist2 * dist2) + 1 / (dist * dist);
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
	for( int i = 0; i < 8; ++i )
	{
		cubeVals[i] = cubeVal(i);
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

	//Find the vertices where the surface intersects the cube
	vertlist[0] = vertexInterp(isolevel, cubePos(0), cubeVals[0], cubePos(1), cubeVals[1]);
	vertlist[1] = vertexInterp(isolevel, cubePos(1), cubeVals[1], cubePos(2), cubeVals[2]);
	vertlist[2] = vertexInterp(isolevel, cubePos(2), cubeVals[2], cubePos(3), cubeVals[3]);
	vertlist[3] = vertexInterp(isolevel, cubePos(3), cubeVals[3], cubePos(0), cubeVals[0]);
	vertlist[4] = vertexInterp(isolevel, cubePos(4), cubeVals[4], cubePos(5), cubeVals[5]);
	vertlist[5] = vertexInterp(isolevel, cubePos(5), cubeVals[5], cubePos(6), cubeVals[6]);
	vertlist[6] = vertexInterp(isolevel, cubePos(6), cubeVals[6], cubePos(7), cubeVals[7]);
	vertlist[7] = vertexInterp(isolevel, cubePos(7), cubeVals[7], cubePos(4), cubeVals[4]);
	vertlist[8] = vertexInterp(isolevel, cubePos(0), cubeVals[0], cubePos(4), cubeVals[4]);
	vertlist[9] = vertexInterp(isolevel, cubePos(1), cubeVals[1], cubePos(5), cubeVals[5]);
	vertlist[10] = vertexInterp(isolevel, cubePos(2), cubeVals[2], cubePos(6), cubeVals[6]);
	vertlist[11] = vertexInterp(isolevel, cubePos(3), cubeVals[3], cubePos(7), cubeVals[7]);

	// Create the triangle
	//colour = vec4(cos(isolevel*5.0-0.5), sin(isolevel*5.0-0.5), 0.5, 1.0);

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

			gl_Position = MVP * vec4( vertlist[ triTableValue(cubeIndex, i) ], 1);
			colour = mix( vec4(0.0,0.0,0.0,1.0), vec4(1.0,0.0,0.0,1.0), height / bounds.y );
			EmitVertex();

			//Generate second vertex of triangle//
			
			gl_Position = MVP * vec4(vertlist[triTableValue(cubeIndex, i+1)], 1);
			EmitVertex();

			//Generate last vertex of triangle//
			
			gl_Position = MVP * vec4(vertlist[triTableValue(cubeIndex, i+2)], 1);
			EmitVertex();
			
			//End triangle strip at firts triangle
			EndPrimitive();
		}
		else
		{
			break;
		}
		//i = i + 3; //Comment it for testing the strange bug
	}
}
//*/