#include "Skybox.h"

Skybox::Skybox() { }

Skybox::~Skybox() { }

void Skybox::Init()
{
	// Create box geometry for skybox
	m_SkyBox = mesh(geometry_builder::create_box());
	// Scale box by 100
	m_SkyBox.get_transform().scale = vec3(100.0f);

	// Load the cubemap
	array<string, 6> filenames = { "res/textures/night_skybox/night_front.png", "res/textures/night_skybox/night_back.png",
		"res/textures/night_skybox/night_up.png", "res/textures/night_skybox/night_down.png",
		"res/textures/night_skybox/night_right.png", "res/textures/night_skybox/night_left.png" };
	// Create cube_map
	m_Cubemap = cubemap(filenames);

	// Load in skybox effect
	e_Skybox.add_shader("res/shaders/skybox.vert", GL_VERTEX_SHADER);
	e_Skybox.add_shader("res/shaders/skybox.frag", GL_FRAGMENT_SHADER);

	// Build effect
	e_Skybox.build();
}

void Skybox::SetPosition(vec3 pos)
{
	m_SkyBox.get_transform().position = pos;
}

void Skybox::Render(mat4 VP)
{
	// Disable depth test,depth mask,face culling
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);

	// Bind skybox effect
	renderer::bind(e_Skybox);
	// Calculate MVP for the skybox
	auto M = m_SkyBox.get_transform().get_transform_matrix();
	auto MVP = VP * M;
	// Set MVP matrix uniform
	glUniformMatrix4fv(e_Skybox.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
	// Set cubemap uniform
	renderer::bind(m_Cubemap, 0);
	glUniform1i(e_Skybox.get_uniform_location("cubemap"), 0);
	// Render skybox
	renderer::render(m_SkyBox);
	// Enable depth test,depth mask,face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);
}