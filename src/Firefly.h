#pragma once
#include "RenderedObject.h"

class Firefly : public RenderedObject
{
public:
	Firefly();
	Firefly(vec3 bounds, texture tex);
	~Firefly();

	void Init(int maxParticles);

	void Render(mat4 V, mat4 P);
	void SetBounds(vec3 position, vec3 bounds);
	void SetSpeed(vec2 speed);
	void SetColour(vec4 colour);
	void SetRange(float range);

	void SyncData();
	void update(float deltaTime) override;

	point_light* GetLight();

	int GetParticleCount();
	GLuint GetPositionBuffer();

private:

	effect computeShader;
	effect fireflyShader;
	GLuint G_Position_buffer;
	GLuint G_Velocity_buffer;
	GLuint G_Targets_buffer;

	GLuint vao;

	texture myTexture;
	int maxParticles;
	bool isInitialised;
	float size;
	vec4 colour;
	vec3 bounds;
	vec2 speed;

	mat4 positionMatrix;
	
	point_light light;
};