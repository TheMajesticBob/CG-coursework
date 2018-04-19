#include "ParticleSystem.h"
#include "time.h"

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

	this->currentParticles = 0;
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

	SetupParticleShader();
}

void ParticleSystem::SetEmitting(bool isEmitting)
{
	if (!isInitialised)
	{
		SetupComputingShader();
	}

	renderer::bind(computeShader);
	glUniform1i(computeShader.get_uniform_location("isEmitting"), (int)isEmitting);
	this->isEmitting = isEmitting;
}

void ParticleSystem::UpdateDelta(float deltaTime)
{
	clock_t t = clock();
	renderer::bind(computeShader);
	glUniform1f(computeShader.get_uniform_location("delta_time"), deltaTime);
	glUniform1f(computeShader.get_uniform_location("time"), float(t));
	glUniform4fv(computeShader.get_uniform_location("windDirection"), 1, value_ptr(windDirection));

	if (isEmitting)
	{
		if (currentParticles < maxParticles)
		{
			lastSpawned -= deltaTime;
			if (lastSpawned <= 0.0f)
			{
				vec4 pos, vel;
				SpawnParticle(&pos, &vel);

				// Bind buffer to read/write data
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, G_Position_buffer);
				// Map the buffer to position array
				vec4 *positions = reinterpret_cast<vec4*>(
					glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4) * maxParticles, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)
				);
				
				// printf("Particle 0: [%f,%f,%f,%f]\n", positions[0].x, positions[0].y, positions[0].z, positions[0].w);
				// Assign a new particle
				positions[currentParticles] = pos;
				// Unmap buffer
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

				// Same as the above but for velocities
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, G_Velocity_buffer);
				vec4 *velocities = reinterpret_cast<vec4*>(
					glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4) * maxParticles, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)
					);
				velocities[currentParticles] = vel;
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

				// Unbind buffer
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

				currentParticles++;
				lastSpawned = ( dist(rand) + 1.0f ) / 2.0f * ( spawnRate.y - spawnRate.x ) + spawnRate.x;

				renderer::bind(smokeShader);
				glUniform1i(smokeShader.get_uniform_location("gMetaballCount"), currentParticles);
			}
		}
	}
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
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, G_Position_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, G_Velocity_buffer);
	// Render
	renderer::render(boxMesh);
	// Tidy up
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

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
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
}

void ParticleSystem::SetParticleLifetime(float min, float max)
{
	initialLifetime = vec2(min, max);
	renderer::bind(computeShader);
	glUniform2fv(computeShader.get_uniform_location("initialLifetime"), 1, value_ptr(initialLifetime));


	renderer::bind(smokeShader);
	glUniform2fv(smokeShader.get_uniform_location("initialLifetime"), 1, value_ptr(initialLifetime));
}

void ParticleSystem::SetParticleColour(vec4 start, vec4 end)
{
	startColour = start;
	endColour = end;

	renderer::bind(smokeShader);
	glUniform4fv(smokeShader.get_uniform_location("startColour"), 1, value_ptr(start));
	glUniform4fv(smokeShader.get_uniform_location("endColour"), 1, value_ptr(end));
}

void ParticleSystem::SetInitialParticleSize(float min, float max)
{
	initialSize = vec2(min, max);
	renderer::bind(computeShader);
	glUniform2fv(computeShader.get_uniform_location("initialSize"), 1, value_ptr(initialSize));
}

void ParticleSystem::SetParticleSpeed(float min, float max)
{
	initialSpeed = vec2(min, max);
	renderer::bind(computeShader);
	glUniform2fv(computeShader.get_uniform_location("initialSpeed"), 1, value_ptr(initialSpeed));
}

void ParticleSystem::SetSpawnRate(float min, float max)
{
	spawnRate = vec2(min, max);
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

void ParticleSystem::SpawnParticle(vec4* pos, vec4* vel)
{
	float size = (dist(rand) + 1.0f) / 2.0f * (initialSize.y - initialSize.x) + initialSize.x;
	vec3 position = vec3((cubeSize.x / 2.0f) * dist(rand) - (cubeSize.x / 4.0f), -(cubeSize.y / 2.0f) + size * 2.0f, (cubeSize.z / 2.0f) * dist(rand) - (cubeSize.z / 4.0f));
	float speed = (dist(rand) + 1.0f) / 2.0f * (initialSpeed.y - initialSpeed.x) + initialSpeed.x;
	float lifetime = (dist(rand) + 1.0f) / 2.0f * (initialLifetime.y - initialLifetime.x) + initialLifetime.x;
	vec3 velocity = vec3(particleDirection) * speed;

	*vel = vec4(velocity, lifetime);
	*pos = vec4(position, size);
}

void ParticleSystem::SetupComputingShader()
{
	// Set up initial uniforms
	glUniform1i(computeShader.get_uniform_location("isEmitting"), (int)isEmitting);
	glUniform3fv(computeShader.get_uniform_location("max_dims"), 1, value_ptr(cubeSize/2.0f));
	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	//Generate Position Data buffer
	glGenBuffers(1, &G_Position_buffer);
	// Generate Velocity Data buffer
	glGenBuffers(1, &G_Velocity_buffer);
	// Bind as GL_SHADER_STORAGE_BUFFER
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, G_Position_buffer);
	// Send Data to GPU, use GL_DYNAMIC_DRAW
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * maxParticles, NULL, GL_DYNAMIC_DRAW);
	// Bind as GL_SHADER_STORAGE_BUFFER
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, G_Velocity_buffer);
	// Send Data to GPU, use GL_DYNAMIC_DRAW
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * maxParticles, NULL, GL_DYNAMIC_DRAW);
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

	glUniform1i(glGetUniformLocationARB(program, "gMetaballCount"), 0);
	// Unbind
	glUseProgramObjectARB(0);
}

void ParticleSystem::SetPosition(vec3 position)
{
	boxMesh.get_transform().position = position;
}