#pragma once
#include <glm/glm.hpp>
#include <graphics_framework.h>

using namespace std;
using namespace graphics_framework;
using namespace glm;

class Terrain
{
public:

	Terrain();
	~Terrain();
	void Init();
	void SetTextures(texture textures[4]);
	void LoadTerrain(const texture &heightMap, unsigned int width, unsigned int depth, float height_scale);
	void Render(mat4 VP);
	mesh* GetMesh();

private:
	geometry geom;
	effect terrainEffect;
	texture tex[4];
	mesh terr;
};