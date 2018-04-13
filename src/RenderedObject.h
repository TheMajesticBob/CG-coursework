#pragma once
#include <glm\glm.hpp>
#include <graphics_framework.h>

using namespace std;
using namespace graphics_framework;
using namespace glm;

class RenderedObject
{
public:
	RenderedObject();
	RenderedObject(mesh m, texture *t = nullptr, texture *n = nullptr) : my_mesh(m), my_tex(t), my_normal(n) {}
	~RenderedObject();

	virtual void update(float delta_time);

	mesh* get_mesh();
	mat4 get_transform_matrix();
	mat3 get_normal_matrix();
	texture* get_texture();
	texture* get_normal();

	vec3 get_world_position();
	quat get_world_orientation();

	void set_parent(RenderedObject *parent);

	void set_effect(effect *e);
	void set_texture(texture *t);
	void set_normal(texture *t);
	void set_mesh(mesh *m);

	// Transform operations
	vec3 get_local_scale();
	void set_local_scale(vec3 scale);

protected:
	mesh my_mesh;
	texture *my_tex, *my_normal;
	effect *my_eff;
	RenderedObject *parent = nullptr;

};

