#pragma once
#include "glm\glm.hpp"
#include "CameraController.h"
#include "MarchingCubes.h"
#include "graphics_framework.h"

const float RESOLUTION = 4.0f;
const unsigned int MAX_PARTICLES = 48;

using namespace graphics_framework;
using namespace std;
using namespace std::chrono;
using namespace glm;

class ParticleSystem
{
public:
	ParticleSystem();
	ParticleSystem(int maxParticles);
	~ParticleSystem();

	void Init(vec3 cubeSize);
	void SetEmitting(bool isEmitting);
	void SetPosition(vec3 position);
	void Render(CameraController* camControl);
	void UpdateDelta(float deltaTime);
	void SyncData();

	void SetParticleSize(float min, float max);
	void SetParticleSpeed(float min, float max);
	void SetParticleDirection(vec3 dir);
	void SetWind(vec4 wind);

private:
	void SetupComputingShader();
	void SetupParticleShader();

	bool isInitialised = false;
	bool isEmitting = false;

	float sizeMin, sizeMax;
	float speedMin, speedMax;
	vec3 particleDirection = vec3(0.0f);
	
	vec4 windDirection;

	uint maxParticles;
	GLuint G_Position_buffer, G_Velocity_buffer;
	GLuint triTableTexture;
	GLuint vao;

	effect computeShader, smokeShader;

	mesh boxMesh;

	vec3 cubeSize;
	vec3 cubeStep = vec3(1.0f / RESOLUTION);

	vector<vec4> smokePositions;
	vector<vec4> smokeVelocitys;
};