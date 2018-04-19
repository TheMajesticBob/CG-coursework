#include "Firefly.h"

Firefly::Firefly()
{
}

Firefly::Firefly(vec3 bounds, texture* tex)
{ 
	my_tex = tex;
	this->bounds = bounds;

	light = point_light();
	SetColour(vec4(dist(rand), dist(rand), dist(rand), 1.0f));

	SetRange(7.0f);
	SetBounds(vec3(0.0f), bounds);
	SetSpeed(vec2(2.0f, 10.0f));
}

Firefly::~Firefly()
{
	glDeleteBuffers(1, &G_Position_buffer);
	glDeleteBuffers(1, &G_Velocity_buffer);
	glDeleteBuffers(1, &G_Targets_buffer);
}

void Firefly::Init(int maxParticles)
{
	this->maxParticles = maxParticles;

	computeShader.add_shader("res/shaders/particle_systems/fireflies.comp", GL_COMPUTE_SHADER);
	computeShader.build();

	fireflyShader.add_shader("res/shaders/particle_systems/fireflies.vert", GL_VERTEX_SHADER);
	fireflyShader.add_shader("res/shaders/particle_systems/fireflies.geom", GL_GEOMETRY_SHADER);
	fireflyShader.add_shader("res/shaders/particle_systems/fireflies.frag", GL_FRAGMENT_SHADER);
	fireflyShader.build();

	vector<vec4> positions;
	vector<vec4> velocitys;
	vector<vec4> targets;

	// Initilise particles
	for (unsigned int i = 0; i < maxParticles; ++i) {

		positions.push_back((vec4((dist(rand) - 0.5f) * bounds.x, (dist(rand) - 0.5f) * bounds.y / 16.0f + 1.0f, (dist(rand) - 0.5f) * bounds.z, 1.0f) * 8.0f));
		velocitys.push_back(vec4((dist(rand) - 0.5f), (dist(rand) - 0.5f), (dist(rand) - 0.5f), dist(rand)*4.0f));
		targets.push_back(vec4(0.0f, 1.0f, 0.0f, 0.0f));
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
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * maxParticles, &positions[0], GL_DYNAMIC_DRAW);

	// Generate Velocity Data buffer
	glGenBuffers(1, &G_Velocity_buffer);
	// Bind as GL_SHADER_STORAGE_BUFFER
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, G_Velocity_buffer);
	// Send Data to GPU, use GL_DYNAMIC_DRAW
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * maxParticles, &velocitys[0], GL_DYNAMIC_DRAW);

	// Generate Velocity Data buffer
	glGenBuffers(1, &G_Targets_buffer);
	// Bind as GL_SHADER_STORAGE_BUFFER
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, G_Targets_buffer);
	// Send Data to GPU, use GL_DYNAMIC_DRAW
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * maxParticles, &targets[0], GL_DYNAMIC_DRAW);

	// *********************************
	//Unbind
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Firefly::update(float deltaTime)
{
	renderer::bind(computeShader);
	glUniform1f(computeShader.get_uniform_location("delta_time"), deltaTime);
	glUniform2fv(computeShader.get_uniform_location("speed"), 1, value_ptr(speed));
	glUniform3fv(computeShader.get_uniform_location("boundsPos"), 1, value_ptr(position));
	glUniform3fv(computeShader.get_uniform_location("bounds"), 1, value_ptr(bounds));
}

void Firefly::SyncData()
{
	glBindVertexArray(vao);
	// Bind Compute Shader
	renderer::bind(computeShader);
	// Bind data as SSBO
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, G_Position_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, G_Velocity_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, G_Targets_buffer);
	// Dispatch
	glDispatchCompute(maxParticles, 1, 1);
	// Sync, wait for completion
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Firefly::Render(mat4 V, mat4 P, GLuint depthTexture)
{
	glBindVertexArray(vao);
	// Bind Compute Shader
	renderer::bind(computeShader);
	// Bind data as SSBO
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, G_Position_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, G_Velocity_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, G_Targets_buffer);
	// Dispatch
	glDispatchCompute(maxParticles, 1, 1);
	// Sync, wait for completion
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glEnable(GL_BLEND); 
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA);
	// Bind render effect
	renderer::bind(fireflyShader);
	// Create MVP matrix
	mat4 M(1.0f);
	auto MVP = V * M;
	// Set the colour uniform
	renderer::bind(*my_tex, 1);
	glUniform1i(fireflyShader.get_uniform_location("tex"), 1);
	// Bind depth texture
	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, depthTexture);

	glUniform2fv(fireflyShader.get_uniform_location("speed"), 1, value_ptr(speed));
	glUniform4fv(fireflyShader.get_uniform_location("colour"), 1, value_ptr(colour));
	glUniform1f(fireflyShader.get_uniform_location("point_size"), size);
	// Set MVP matrix uniform
	glUniformMatrix4fv(fireflyShader.get_uniform_location("MV"), 1, GL_FALSE, value_ptr(MVP));
	glUniformMatrix4fv(fireflyShader.get_uniform_location("P"), 1, GL_FALSE, value_ptr(P));

	// Bind position buffer as GL_ARRAY_BUFFER
	glBindBuffer(GL_ARRAY_BUFFER, G_Position_buffer);
	// Setup vertex format
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void *)0);
	// Render
	glDrawArrays(GL_POINTS, 0, maxParticles);
	// Tidy up
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glUseProgram(0);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void Firefly::SetBounds(vec3 position, vec3 bounds)
{
	this->position = position;
	this->positionMatrix = translate(mat4(1.0f), position);
	this->bounds = bounds;
}

void Firefly::SetSpeed(vec2 speed)
{
	this->speed = speed;

	renderer::bind(computeShader);
	glUniform2fv(computeShader.get_uniform_location("speed"), 1, value_ptr(speed));
}

void Firefly::SetColour(vec4 colour)
{
	this->colour = colour;
	light.set_light_colour(colour);
}

void Firefly::SetRange(float range)
{
	size = range / 20.0f;
	light.set_range(range);
}

point_light* Firefly::GetLight(vec3 pos)
{
	light.set_position(pos);
	light.set_range(size*20.0f);
	return &light;
}

int Firefly::GetParticleCount()
{
	return maxParticles;
}

GLuint Firefly::GetPositionBuffer()
{
	return G_Position_buffer;
}