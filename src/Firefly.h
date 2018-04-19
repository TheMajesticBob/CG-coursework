#pragma once
#include "RenderedObject.h"

class Firefly : public RenderedObject
{
public:
	Firefly();
	Firefly(vec3 bounds, texture* tex);
	~Firefly();

	void Init(int maxParticles);

	void Render(mat4 V, mat4 P, GLuint depthTexture);
	void SetBounds(vec3 position, vec3 bounds);
	void SetSpeed(vec2 speed);
	void SetColour(vec4 colour);
	void SetRange(float range);

	void SyncData();
	void update(float deltaTime) override;

	point_light* GetLight(vec3 pos);

	int GetParticleCount();

	GLuint GetPositionBuffer();

private:
	default_random_engine rand;
	uniform_real_distribution<float> dist;

	effect computeShader;
	effect fireflyShader;
	GLuint G_Position_buffer;
	GLuint G_Velocity_buffer;
	GLuint G_Colours_buffer;
	GLuint G_Targets_buffer;

	GLuint vao;

	int maxParticles;
	bool isInitialised;
	float size;
	vec4 colour;
	vec3 bounds;
	vec2 speed;

	vec3 position;
	mat4 positionMatrix;
	
	point_light light;
};