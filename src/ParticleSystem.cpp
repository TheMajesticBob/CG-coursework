#include "ParticleSystem.h"

extern int triTable[256][16];

ParticleSystem::ParticleSystem()
{
	G_Position_buffer = 0;
	G_Velocity_buffer = 0;

	windDirection = vec4(normalize(vec3(0.5f, 0.0f, 0.5f)), 1.0f);
}

ParticleSystem::ParticleSystem(int maxParticles)
{
	ParticleSystem::ParticleSystem();

	this->maxParticles = maxParticles;

	computeShader.add_shader("res/shaders/smoke.comp", GL_COMPUTE_SHADER);
	computeShader.build();

	smokeShader.add_shader("res/shaders/smoke.vert", GL_VERTEX_SHADER);
	smokeShader.add_shader("res/shaders/smoke.geom", GL_GEOMETRY_SHADER);
	smokeShader.add_shader("res/shaders/smoke.frag", GL_FRAGMENT_SHADER);
	smokeShader.build();

	SetPosition(vec3(0.0f));
}

ParticleSystem::~ParticleSystem()
{
}

void ParticleSystem::Init(vec3 cubeSize)
{
	this->cubeSize = cubeSize;

	SetupComputingShader();
	SetupParticleShader();
}

void ParticleSystem::SetEmitting(bool isEmitting)
{
	if (!isInitialised)
	{
		SetupParticleShader();
	}

	renderer::bind(computeShader);
	glUniform1i(computeShader.get_uniform_location("isEmitting"), (int)isEmitting);
}

void ParticleSystem::UpdateDelta(float deltaTime)
{
	float systemTime = (float)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	renderer::bind(computeShader);
	glUniform1f(computeShader.get_uniform_location("delta_time"), deltaTime);
	glUniform1f(computeShader.get_uniform_location("time"), systemTime);
	glUniform4fv(computeShader.get_uniform_location("windDirection"), 1, value_ptr(windDirection));
	glUniform3fv(computeShader.get_uniform_location("max_dims"), 1, value_ptr(cubeSize));
}

void ParticleSystem::Render(CameraController* camControl)
{
	if (!isInitialised)
	{
		return;
	}

	auto MVP = camControl->GetCurrentVPMatrix() * boxMesh.get_transform().get_transform_matrix();

	renderer::bind(smokeShader);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// glDepthMask(GL_FALSE);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, triTableTexture);
	glUniform1i(smokeShader.get_uniform_location("triTableTex"), 3);
	glUniform1f(smokeShader.get_uniform_location("isolevel"), 5.0f);

	glUniformMatrix4fv(smokeShader.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));

	// Bind position buffer as GL_SHADER_STORAGE_BUFFER
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, G_Position_buffer);
	// Render
	renderer::render(boxMesh);
	// Tidy up
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glDisable(GL_BLEND);
}

void ParticleSystem::SyncData()
{
	if (!isInitialised)
	{
		return;
	}

	// Bind Compute Shader
	renderer::bind(computeShader);
	// Bind data as SSBO
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, G_Position_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, G_Velocity_buffer);
	// Dispatch
	glDispatchCompute(MAX_PARTICLES, 1, 1);
	// Sync, wait for completion
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ParticleSystem::SetParticleSize(float min, float max)
{
	renderer::bind(computeShader);
	glUniform1f(computeShader.get_uniform_location("sizeMin"), min);
	glUniform1f(computeShader.get_uniform_location("sizeMax"), max);
}

void ParticleSystem::SetParticleSpeed(float min, float max)
{
	renderer::bind(computeShader);
	glUniform1f(computeShader.get_uniform_location("speedMin"), min);
	glUniform1f(computeShader.get_uniform_location("speedMax"), max);
}

void ParticleSystem::SetParticleDirection(vec3 dir)
{
	particleDirection = dir;
	renderer::bind(computeShader);
	glUniform3fv(computeShader.get_uniform_location("initialDirection"), 1, value_ptr(dir));
}

void ParticleSystem::SetWind(vec4 wind)
{
	windDirection = wind;
	renderer::bind(computeShader);
	glUniform4fv(computeShader.get_uniform_location("windDirection"), 1, value_ptr(wind));
}

void ParticleSystem::SetupComputingShader()
{
	default_random_engine rand(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
	uniform_real_distribution<float> dist;
	
	// Initilise particles
	for (unsigned int i = 0; i < MAX_PARTICLES; ++i) {
		vec4 newPos = vec4( ( cubeSize.x / 2.0f ) * dist(rand) - (cubeSize.x / 4.0f), 3.0f * dist(rand), (cubeSize.z / 2.0f) * dist(rand) - (cubeSize.z / 4.0f), dist(rand) * 1.2f + 0.4f);
		
		smokeVelocitys.push_back(vec4(0.0f, 0.2f + 8.0f * dist(rand), 0.0f, 0.0f));

		smokePositions.push_back(newPos);
		printf("Particle [%f] pos [%f,%f,%f]\n\r", smokeVelocitys[i].y, newPos.x, newPos.y, newPos.z);
	}

	// Set up initial uniforms
	glUniform1i(computeShader.get_uniform_location("isEmitting"), (int)isEmitting);

	// A useless vao, but we need it bound or we get errors.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
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
	// Unbind
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	isInitialised = true;
}

void ParticleSystem::SetupParticleShader()
{
	// Generate vertex grid for marching cubes
	vector<vec3> pos;
	for (float k = -cubeSize.z / 2.0f; k <= cubeSize.z / 2.0f; k += cubeStep.z)
	{
		for (float j = -cubeSize.y / 2.0f; j <= cubeSize.y / 2.0f; j += cubeStep.y)
		{
			for (float i = -cubeSize.x / 2.0f; i <= cubeSize.x / 2.0f; i += cubeStep.x) {
				pos.push_back(vec3(i, j, k));
			}
		}
	}
	// Add to the geometry
	geometry cube;
	cube.add_buffer(pos, BUFFER_INDEXES::POSITION_BUFFER);
	cube.set_type(GL_POINTS);

	boxMesh = mesh(cube);

	GLuint program = smokeShader.get_program();

	//Bind program object for parameters setting
	glUseProgramObjectARB(program);

	// This texture stores the vertex index list for generating the triangles of each configurations. (cf. MarchingCubes.cpp)
	glGenTextures(1, &triTableTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, triTableTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16I, 16, 256, 0, GL_RED_INTEGER, GL_INT, &triTable);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	////Samplers assignment///
	glUniform1iARB(glGetUniformLocationARB(program, "triTableTex"), 0);
	//Initial isolevel
	glUniform1fARB(glGetUniformLocationARB(program, "isolevel"), 5.0f);
	// Set boundaries
	glUniform3fvARB(glGetUniformLocationARB(program, "bounds"), 1, value_ptr(cubeSize));

	cubeStep /= 2.0f;
	//Decal for each vertex in a marching cube
	glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[0]"), -cubeStep.x, -cubeStep.y, -cubeStep.z);
	glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[1]"), cubeStep.x, -cubeStep.y, -cubeStep.z);
	glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[2]"), cubeStep.x, cubeStep.y, -cubeStep.z);
	glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[3]"), -cubeStep.x, cubeStep.y, -cubeStep.z);
	glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[4]"), -cubeStep.x, -cubeStep.y, cubeStep.z);
	glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[5]"), cubeStep.x, -cubeStep.y, cubeStep.z);
	glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[6]"), cubeStep.x, cubeStep.y, cubeStep.z);
	glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[7]"), -cubeStep.x, cubeStep.y, cubeStep.z);

	glUniform1i(glGetUniformLocationARB(program, "gMetaballCount"), MAX_PARTICLES);
	// Unbind
	glUseProgramObjectARB(0);
}

void ParticleSystem::SetPosition(vec3 position)
{
	boxMesh.get_transform().position = position;
}