#ifndef MarchingCubes_h
#define MarchingCubes_h
#include <glm\glm.hpp>

using namespace glm;

class GridCell{
public:
	float val[8];
	vec3 pos[8];
};

void RenderMarchCube(float *data, ivec3 size, ivec3 gridsize, float isolevel);

int Polygonise(GridCell &grid, float isolevel, ivec3 *triangles);

#endif