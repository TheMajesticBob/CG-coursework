#include "RenderedObject.h"

RenderedObject::RenderedObject()
{
}

RenderedObject::~RenderedObject()
{
}

texture * RenderedObject::get_texture()
{
	return my_tex;
}

texture * RenderedObject::get_normal()
{
	return my_normal;
}

void RenderedObject::set_parent(RenderedObject *p)
{
	parent = p;
}

void RenderedObject::set_effect(effect * e)
{
	my_eff = e;
}

void RenderedObject::set_texture(texture * t)
{
	my_tex = t;
}

void RenderedObject::set_normal(texture * t)
{
	my_normal = t;
}

void RenderedObject::set_mesh(mesh * m)
{
	my_mesh = mesh(*m);
}

mat4 RenderedObject::get_transform_matrix()
{
	// Simple recursive matrix multiplication
	if (parent != nullptr)
	{
		return parent->get_transform_matrix() * my_mesh.get_transform().get_transform_matrix();
	}

	return my_mesh.get_transform().get_transform_matrix();
}

mat3 RenderedObject::get_normal_matrix()
{
	if (parent != nullptr)
	{
		return mat3(get_transform_matrix()) * my_mesh.get_transform().get_normal_matrix();
	}

	return my_mesh.get_transform().get_normal_matrix();
}

vec3 RenderedObject::get_world_position()
{
	// To get world position just multiply local position by parent's transform matrix
	if (parent != nullptr)
	{
		return parent->get_transform_matrix() * vec4(my_mesh.get_transform().position, 1.0f);
	}

	return my_mesh.get_transform().position;
}

quat RenderedObject::get_world_orientation()
{
	// Get 3x3 matrix (drop translation as it's not needed)
	mat3 mat = get_transform_matrix();

	// Get scale
	vec3 scale = get_local_scale();

	// Divide the matrix by scale vector to get rotation matrix
	mat3 rotMat;
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			rotMat[i][j] = mat[i][j] / scale[i];
		}
	}

	// Return a quaternion
	return toQuat(rotMat);
}

vec3 RenderedObject::get_local_scale()
{
	// If there is a parent multiply scale by parent's scale to receive local scale
	if (parent != nullptr)
	{
		return my_mesh.get_transform().scale * parent->get_local_scale();
	}

	// Return own scale
	return my_mesh.get_transform().scale;
}

void RenderedObject::set_local_scale(vec3 scale)
{
	// If there is a parent divide target scale by parent's scale
	vec3 targetScale = scale;
	if (parent != nullptr)
	{
		targetScale = scale / parent->get_local_scale();
	}
	my_mesh.get_transform().scale = targetScale;
}

void RenderedObject::update(float delta_time)
{

}

mesh* RenderedObject::get_mesh()
{
	return &my_mesh;
}