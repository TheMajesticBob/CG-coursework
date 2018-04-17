#pragma once
#include "glm\glm.hpp"
#include "MarchingCubes.h"
#include "graphics_framework.h"

const float RESOLUTION = 4.0f;

using namespace graphics_framework;
using namespace std;
using namespace std::chrono;
using namespace glm;

class ParticleSystem
{
public:
	ParticleSystem(int maxParticles);
	~ParticleSystem();

	void Init(vec3 cubeSize);

private:
	int maxParticles;
	GLuint G_Position_buffer, G_Velocity_buffer;
	GLuint triTableTexture;
	GLuint vao;

	effect computeShader, smokeShader;

	geometry cube;
	vec3 cubeSize;
	vec3 cubeStep = vec3(1.0f / RESOLUTION);

	vector<vec4> smokePositions;
	vector<vec4> smokeVelocitys;
};