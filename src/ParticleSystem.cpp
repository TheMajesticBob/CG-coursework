#include "ParticleSystem.h"

ParticleSystem::ParticleSystem(int maxParticles)
{
	this->maxParticles = maxParticles;

	computeShader.add_shader("res/shaders/smoke.comp", GL_COMPUTE_SHADER);
	computeShader.build();

	smokeShader.add_shader("res/shaders/smoke.vert", GL_VERTEX_SHADER);
	smokeShader.add_shader("res/shaders/smoke.geom", GL_GEOMETRY_SHADER);
	smokeShader.add_shader("res/shaders/smoke.frag", GL_FRAGMENT_SHADER);
	smokeShader.build();
}

ParticleSystem::~ParticleSystem()
{
}

void ParticleSystem::Init(vec3 cubeSize)
{
	default_random_engine rand(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
	uniform_real_distribution<float> dist;

	// Initilise particles
	for (unsigned int i = 0; i < maxParticles; ++i) {
		smokePositions.push_back(vec4((cubeSize.x / 2.0f) * dist(rand) - cubeSize.x / 4.0f, 8.0f * dist(rand), (cubeSize.z / 2.0f) * dist(rand) - cubeSize.z / 4.0f, dist(rand) + 0.1f);
		smokeVelocitys.push_back(vec4(0.0f, 0.4f + (2.0f * dist(rand)), 0.0f, 0.0f));
	}

	// a useless vao, but we need it bound or we get errors.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	// *********************************
	//Generate Position Data buffer
	glGenBuffers(1, &G_Position_buffer);
	// Bind as GL_SHADER_STORAGE_BUFFER
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, G_Position_buffer);
	// Send Data to GPU, use GL_DYNAMIC_DRAW
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * smokePositions.size(), &smokePositions[0], GL_DYNAMIC_DRAW);

	// Generate Velocity Data buffer
	glGenBuffers(1, &G_Velocity_buffer);
	// Bind as GL_SHADER_STORAGE_BUFFER
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, G_Velocity_buffer);
	// Send Data to GPU, use GL_DYNAMIC_DRAW
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * smokeVelocitys.size(), &smokeVelocitys[0], GL_DYNAMIC_DRAW);

	// *********************************
	//Unbind
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
