#pragma once
#include "CameraController.h"

using namespace graphics_framework;
using namespace glm;
using namespace std;

class Skybox
{
public:
	Skybox();
	~Skybox();

	void Init();

	void SetPosition(vec3 pos);
	void Render(mat4 VP);
private:
	mesh m_SkyBox;
	effect e_Skybox;
	cubemap m_Cubemap;
};