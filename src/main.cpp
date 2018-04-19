#include <glm\glm.hpp>
#include <graphics_framework.h>
// Custom framework
#include "InputHandler.h"
#include "RenderedObject.h"
#include "Player.h"
#include "Firefly.h"
// Deferred shading
#include "GBuffer.h"
// Additional effects
#include "ParticleSystem.h"
#include "Skybox.h"
#include "Terrain.h"

// Geometry
map<string, RenderedObject*> gameObjects;

// Shaders
effect deferredShading;
effect deferredRendering;

/// Lighting
effect stencilPass; // Used to detect light "collisions"
effect dirLightPass; // Directional light shader
effect pointLightPass; // Point light shader

mesh pLight; // Mesh used to render point lights

vector<point_light> pointLights(4);
directional_light ambientLight;

/// Textures
map<string, texture> textures;
map<string, texture> normals;

Player* player;
chase_camera playerCamera;
CameraController camController;

// Terrain
Terrain m_Terrain;
vec3 terrainSize;

/// Custom effects
ParticleSystem smokePS;
Firefly firefliesPS;
float windSpeed;
vec3 windDirection;

Skybox skyBox;

/// Post-process
effect outline; // Outline shader
vec2 screenSize;
geometry screen_quad;

// Input vars
float flatValue = -3.0f;
float blendValue = 0.88f; // 100.0f;

void SetupLights();
void SetupGeometry();

// Deferred shading methods
void DSGeometryPass();
void DSPointLightPass(point_light* light);
void DSStencilPass(point_light* light);
void DSDirectionalLightPass();

// Post-process methods
void RenderFrameOnScreen();
void RenderOutline();

float CalcPointLightSphere(point_light* light);

int depthOnly = 0;
// Random stuff
const int MAX_FIREFLIES = 10;

// Deferred render vars
GBuffer gBuffer;
GLuint outlineTexture, outlineBuffer;

// Random numbers
default_random_engine engine(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
uniform_real_distribution<float> dist;

bool initialise() {
	// Set input mode - hide the cursor
	glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set custom input callbacks
	glfwSetInputMode(renderer::get_window(), GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSetKeyCallback(renderer::get_window(), InputHandler::KeyboardHandler);
	glfwSetMouseButtonCallback(renderer::get_window(), InputHandler::MouseButtonHandler);
	glfwSetCursorPosCallback(renderer::get_window(), InputHandler::MousePosHandler);

	camController.Initialize();
	return true;
}

bool load_content() {
	// Save screen size for other shaders
	screenSize = vec2( renderer::get_screen_width(), renderer::get_screen_height() );

	// Create screen quad
	vector<vec3> positions{ vec3(-1.0f, -1.0f, 0.0f), vec3(1.0f, -1.0f, 0.0f), vec3(-1.0f, 1.0f, 0.0f),
		vec3(1.0f, 1.0f, 0.0f) };
	vector<vec2> tex_coords{ vec2(0.0, 0.0), vec2(1.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 1.0f) };
	screen_quad.add_buffer(positions, BUFFER_INDEXES::POSITION_BUFFER);
	screen_quad.add_buffer(tex_coords, BUFFER_INDEXES::TEXTURE_COORDS_0);
	screen_quad.set_type(GL_TRIANGLE_STRIP);

	// Initialise GBuffer
	gBuffer = GBuffer();
	gBuffer.Init(screenSize.x, screenSize.y);

	// Load up textures
	textures["empty"] = texture("res/textures/empty_texture.png");
	textures["dirt"] = texture("res/textures/dirt.jpg");
	textures["brick"] = texture("res/textures/brick.jpg");
	textures["metal"] = texture("res/textures/metal.png");
	textures["tree"] = texture("res/textures/tree.png");
	textures["grass"] = texture("res/textures/grass.png");
	textures["stone"] = texture("res/textures/stone.jpg");
	textures["stonygrass"] = texture("res/textures/stonygrass.jpg");

	// Load up normal textures
	normals["empty"] = texture("res/textures/normals/empty_normal.png");
	normals["dirt"] = texture("res/textures/normals/dirt_normal.jpg");
	normals["brick"] = texture("res/textures/normals/brick_normal.jpg");
	normals["metal"] = texture("res/textures/normals/metal_normal.png");

	// Create cell shading texture
	vector<vec4> colour_data{ 
		// 4 pixels for darkest colours
		vec4(0.12f, 0.12f, 0.12f, 1.0f), vec4(0.12f, 0.12f, 0.12f, 1.0f),vec4(0.12f, 0.12f, 0.12f, 1.0f), vec4(0.12f, 0.12f, 0.12f, 1.0f), 
		// 6 pixels for quarter-lit
		vec4(0.25f, 0.25f, 0.25f, 1.0f), vec4(0.25f, 0.25f, 0.25f, 1.0f), vec4(0.25f, 0.25f, 0.25f, 1.0f), vec4(0.25f, 0.25f, 0.25f, 1.0f), vec4(0.25f, 0.25f, 0.25f, 1.0f), vec4(0.25f, 0.25f, 0.25f, 1.0f), 
		// 8 pixels for half lit
		vec4(0.5f, 0.5f, 0.5f, 1.0f), vec4(0.5f, 0.5f, 0.5f, 1.0f), vec4(0.5f, 0.5f, 0.5f, 1.0f), vec4(0.5f, 0.5f, 0.5f, 1.0f), vec4(0.5f, 0.5f, 0.5f, 1.0f), vec4(0.5f, 0.5f, 0.5f, 1.0f), vec4(0.5f, 0.5f, 0.5f, 1.0f), vec4(0.5f, 0.5f, 0.5f, 1.0f),
		// 16 pixels for fully lit
		vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f),
		vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f)
	};
	textures["CS_DATA"] = texture(colour_data, colour_data.size(), 1, false, false);

	// Create skybox
	skyBox.Init();

	// Create terrain
	// Load heightmap and terrain textures
	texture height_map("res/textures/heightmap.png");
	texture terrainTex[]{ textures["grass"],textures["grass"],textures["stonygrass"],textures["stone"] };

	terrainSize = vec3(20, 20, 20);

	m_Terrain.Init();
	m_Terrain.LoadTerrain(height_map, terrainSize.x, terrainSize.y, 2.0f);
	m_Terrain.SetTextures(terrainTex);

	mesh* terrMesh = m_Terrain.GetMesh();
	terrMesh->get_transform().scale = vec3(terrainSize.z);
	terrMesh->get_material().set_diffuse(vec4(0.5f, 0.5f, 0.5f, 1.0f));
	terrMesh->get_material().set_specular(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	terrMesh->get_material().set_shininess(20.0f);
	terrMesh->get_material().set_emissive(vec4(0.0f, 0.0f, 0.0f, 1.0f));

	// Create some geometry
	SetupGeometry();

	// Create lights
	SetupLights();

	// Load up shader files
	deferredShading.add_shader("res/shaders/deferred_shading.vert", GL_VERTEX_SHADER);
	deferredShading.add_shader("res/shaders/deferred_shading.frag", GL_FRAGMENT_SHADER);
	deferredShading.add_shader("res/shaders/part_normal_map.frag", GL_FRAGMENT_SHADER);
	deferredShading.build();

	deferredRendering.add_shader("res/shaders/simple_texture.vert", GL_VERTEX_SHADER);
	deferredRendering.add_shader("res/shaders/deferred_rendering.frag", GL_FRAGMENT_SHADER);
	deferredRendering.build();

	pointLightPass.add_shader("res/shaders/simple_texture.vert", GL_VERTEX_SHADER);
	pointLightPass.add_shader("res/shaders/point_light_pass.frag", GL_FRAGMENT_SHADER);
	pointLightPass.build();

	dirLightPass.add_shader("res/shaders/simple_texture.vert", GL_VERTEX_SHADER);
	dirLightPass.add_shader("res/shaders/dir_light_pass.frag", GL_FRAGMENT_SHADER);
	dirLightPass.build();

	stencilPass.add_shader("res/shaders/simple_texture.vert", GL_VERTEX_SHADER);
	stencilPass.build();

	outline.add_shader("res/shaders/simple_texture.vert", GL_VERTEX_SHADER);
	outline.add_shader("res/shaders/outline.frag", GL_FRAGMENT_SHADER);
	outline.build();

	// Set up smoke particles
	smokePS = ParticleSystem(32);
	smokePS.Init(vec3(8.0f, 16.0f, 6.0f));
	smokePS.SetParticleColour(vec4(1.0f, 0.3f, 0.0f, 1.0f), vec4(0.9f));
	smokePS.SetSpawnRate(0.05f, 0.15f);
	smokePS.SetParticleLifetime(1.0f, 5.0f);
	smokePS.SetInitialParticleSize(0.2f, 1.0f);
	smokePS.SetParticleSpeed(1.0f, 4.0f);
	smokePS.SetParticleDirection(vec3(0.0f, 1.0f, 0.0f));
	smokePS.SetPosition(vec3(0.0f, 8.0f, 0.0f));

	smokePS.SetEmitting(true);

	// Set up fireflies
	firefliesPS = Firefly(vec3(32.0f, 20.0f, 32.0f), texture("res/textures/firefly.png"));
	firefliesPS.Init(10);

	windSpeed = 0.02f;
	windDirection = vec3(1.0f, 0.0f, 0.0f);

	// Set up the player
	auto player = (Player*)gameObjects["player"];
	player->Initialize();

	// Create a camera for the player
	playerCamera.set_pos_offset(vec3(0.0f, 4.0f, 10.0f));
	playerCamera.set_springiness(0.5f);
	playerCamera.set_projection(quarter_pi<float>(), renderer::get_screen_aspect(), 0.1f, 1000.0f);
	playerCamera.move(gameObjects["turretBase"]->get_mesh()->get_transform().position, eulerAngles(gameObjects["turretBase"]->get_mesh()->get_transform().orientation));
	camController.AddNewCamera("playerCamera", &playerCamera);

	// Set up a framebuffer for the outline effect
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &outlineBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outlineBuffer);

	glGenTextures(1, &outlineTexture);
	glBindTexture(GL_TEXTURE_2D, outlineTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenSize.x, screenSize.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outlineTexture, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		printf("MAIN FB error, status: 0x%x\n", Status);
		return false;
	}

	// Reset buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}


bool update(float delta_time) {
	// Update input
	InputHandler::Update(delta_time);

	// Update smoke particles
	smokePS.UpdateDelta(delta_time);
	firefliesPS.update(delta_time);

	camera* currentCam = camController.GetActiveCamera();

	// Update the camera
	if (currentCam != nullptr)
	{
		if (currentCam == camController.GetCameraByName("freeCam"))
		{
			camController.HandleFreeCameraRotation();
		}
		if (currentCam == camController.GetCameraByName("playerCamera"))
		{
			double dx, dy;
			InputHandler::GetRealMouseDelta(&dx, &dy);

			playerCamera.rotate(vec3(-dy, -dx, 0.0f));
			playerCamera.set_target_pos(gameObjects["turretBase"]->get_world_position());
			playerCamera.set_target_rotation(eulerAngles(gameObjects["turretBase"]->get_world_orientation()));

			player->RotateTurret(-dx);
		}

		currentCam->update(delta_time);
	}

	if (glfwGetKey(renderer::get_window(), 'Z')) {
		flatValue -= .2f * delta_time;
		printf("%f\n\r", flatValue);
	}
	if (glfwGetKey(renderer::get_window(), 'X')) {
		flatValue += .2f * delta_time;
		printf("%f\n\r", flatValue);
	}

	if (glfwGetKey(renderer::get_window(), 'C')) {
		blendValue -= 0.05f;
		printf("%f\n\r", blendValue);
	}
	if (glfwGetKey(renderer::get_window(), 'V')) {
		blendValue += 0.05f;
		printf("%f\n\r", blendValue);
	}

	if (glfwGetKey(renderer::get_window(), 'Q')) {
		depthOnly = 1;
	}
	else if (glfwGetKey(renderer::get_window(), 'E')) {
		depthOnly = 2;
	}
	else {
		depthOnly = 0;
	}

	//windDirection = lerp(windDirection, vec3((dist(engine) * 2.0f) - 1.0f, (dist(engine) * 2.0f) - 1.0f, (dist(engine) * 2.0f) - 1.0f), delta_time);
	//windSpeed = clamp( lerp( windSpeed, windSpeed + ( dist(engine) * 2.0f ) - 1.0f * dist(engine), delta_time ), -0.3f, 0.3f );
	//smokePS.SetWind(vec4(windDirection, windSpeed));

	skyBox.SetPosition(currentCam->get_position());
	player->update(delta_time);

	glfwPollEvents();

  return true;
}

bool render() {
	smokePS.SyncData();
	firefliesPS.SyncData();

	gBuffer.StartFrame();

	DSGeometryPass();

	// Light passes
	glEnable(GL_STENCIL_TEST);

	for (int i = 0; i < pointLights.size(); ++i)
	{
		// Point lights get a stencil test 
		DSStencilPass(&pointLights[i]);
		DSPointLightPass(&pointLights[i]);
	}

	GLuint FireflyPosBuffer = firefliesPS.GetPositionBuffer();
	int particleCount = firefliesPS.GetParticleCount();
	// Bind buffer to read/write data
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, FireflyPosBuffer);
	// Map the buffer to position array
	// vec4 *positions = reinterpret_cast<vec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4) * particleCount, GL_MAP_READ_BIT));

	for (int i = 0; i < particleCount; ++i)
	{
		//printf("Particles pos [%f,%f,%f]\n\r", positions[i].x, positions[i].y, positions[i].z);
		//DSStencilPass(firefliesPS.GetLight());
		//DSPointLightPass(firefliesPS.GetLight());
	}
	// Unmap buffer
	// glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	// Unbind buffer
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glDisable(GL_STENCIL_TEST);
	
	DSDirectionalLightPass();

	RenderFrameOnScreen();

	RenderOutline();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return true;
}

void main() {
  // Create application
  app application("Graphics Coursework");
  // Set load content, update and render methods
  application.set_load_content(load_content);
  application.set_initialise(initialise);
  application.set_update(update);
  application.set_render(render);
  // Run application
  application.run();
}

void DSGeometryPass()
{
	gBuffer.BindForGeometryPass();

	auto VP = camController.GetCurrentVPMatrix();

	glDepthMask(GL_TRUE);
	// Clear the render targets
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Make sure to enable depth test
	glEnable(GL_DEPTH_TEST);

	m_Terrain.Render(VP);

	// Bind effect
	renderer::bind(deferredShading);

	// Render meshes
	for (auto &e : gameObjects)
	{
		auto m = e.second;
		// Create MVP matrix
		mat4 M = m->get_transform_matrix();
		mat4 MVP = VP * M;

		// Set MVP matrix uniform
		glUniformMatrix4fv(deferredShading.get_uniform_location("MVP"), // Location of uniform
			1,                               // Number of values - 1 mat4
			GL_FALSE,                        // Transpose the matrix?
			value_ptr(MVP));                 // Pointer to matrix data
		glUniformMatrix4fv(deferredShading.get_uniform_location("M"), 1, GL_FALSE, value_ptr(M));
		// Set N matrix uniform - remember - 3x3 matrix
		glUniformMatrix3fv(deferredShading.get_uniform_location("N"), 1, GL_FALSE, value_ptr(m->get_normal_matrix()));

		renderer::bind(m->get_mesh()->get_material(), "mat");

		// Bind texture
		if (m->get_texture() != nullptr)
		{
			renderer::bind(*m->get_texture(), 0);
		}
		else {
			renderer::bind(textures["empty"], 0);
		}
		// Set tex uniform
		glUniform1i(deferredShading.get_uniform_location("tex_diffuse"), 0);
		if (m->get_normal() != nullptr)
		{
			// Set normal
			renderer::bind(*m->get_normal(), 1);
		}
		else {
			renderer::bind(normals["empty"], 1);
		}
		// Set tex uniform
		glUniform1i(deferredShading.get_uniform_location("normal_map"), 1);
		// Render mesh
		renderer::render(*m->get_mesh());
	}

	smokePS.Render(&camController);

	// firefliesPS.Render(camController.GetCurrentViewMatrix(), camController.GetCurrentProjectionMatrix());

	glDepthMask(GL_FALSE);
}

float CalcPointLightSphere(point_light* light)
{
	vec4 colour = light->get_light_colour();
	float MaxChannel = fmax(fmax(colour.x, colour.y), colour.z);

	float linear_attenuation = light->get_linear_attenuation();
	float quad_attenuation = light->get_quadratic_attenuation();
	
	float ret = (-linear_attenuation + sqrtf(linear_attenuation * linear_attenuation -
		4 * quad_attenuation * (quad_attenuation - 256 * MaxChannel * light->get_constant_attenuation())))
		/ (2 * quad_attenuation);
	return ret;
}

void DSStencilPass(point_light* light)
{
	auto VP = camController.GetCurrentVPMatrix();

	gBuffer.BindForStencilPass();

	// Set up needed bits
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glClear(GL_STENCIL_BUFFER_BIT);

	// Set stencil to always succeed, checking only with depth buffer
	glStencilFunc(GL_ALWAYS, 0, 0);

	// Back faces increase stencil value, front faces reduce it
	glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

	renderer::bind(stencilPass);
	pLight.get_transform().position = light->get_position();
	pLight.get_transform().scale = vec3(CalcPointLightSphere(light));

	auto M = pLight.get_transform().get_transform_matrix();
	auto MVP = VP * M;

	renderer::bind(*light, "gPointLight");
	
	glUniformMatrix4fv(stencilPass.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
	renderer::render(pLight);
}

void DSPointLightPass(point_light* light)
{
	gBuffer.BindForLightPass();

	glStencilFunc(GL_NOTEQUAL, 0, 0xFF);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	renderer::bind(pointLightPass);
	camera* cam = camController.GetActiveCamera();
	auto VP = camController.GetCurrentVPMatrix();

	float sphereSize = CalcPointLightSphere(light);

	pLight.get_transform().position = light->get_position();
	pLight.get_transform().scale = vec3(sphereSize);

	auto M = pLight.get_transform().get_transform_matrix();
	auto MVP = VP * M;

	renderer::bind(*light, "gPointLight");
	glUniform3fv(pointLightPass.get_uniform_location("gEyeWorldPos"), 1, value_ptr(cam->get_position()));
	glUniform2fv(pointLightPass.get_uniform_location("gScreenSize"), 1, value_ptr(screenSize));
	glUniform1f(pointLightPass.get_uniform_location("sphereSize"), sphereSize);

	glUniform1i(pointLightPass.get_uniform_location("tPosition"), 0);
	glUniform1i(pointLightPass.get_uniform_location("tAlbedo"), 1);
	glUniform1i(pointLightPass.get_uniform_location("tNormals"), 2);
	glUniform1i(pointLightPass.get_uniform_location("tMatDiffuse"), 3);
	glUniform1i(pointLightPass.get_uniform_location("tMatSpecular"), 4);
	glUniform1i(pointLightPass.get_uniform_location("tMatEmissive"), 5);

	glUniformMatrix4fv(pointLightPass.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
	renderer::render(pLight);

	glCullFace(GL_BACK);
	glDisable(GL_BLEND);
}

void DSDirectionalLightPass()
{
	gBuffer.BindForLightPass();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	
	glBlendFunc(GL_ONE, GL_ONE);

	renderer::bind(dirLightPass);

	renderer::bind(ambientLight, "gDirectionalLight");
	glUniform3fv(dirLightPass.get_uniform_location("gEyeWorldPos"), 1, value_ptr(camController.GetActiveCamera()->get_position()));
	glUniform2fv(dirLightPass.get_uniform_location("gScreenSize"), 1, value_ptr(screenSize));

	glUniform1i(dirLightPass.get_uniform_location("tPosition"), 0);
	glUniform1i(dirLightPass.get_uniform_location("tAlbedo"), 1);
	glUniform1i(dirLightPass.get_uniform_location("tNormals"), 2);
	glUniform1i(dirLightPass.get_uniform_location("tMatDiffuse"), 3);
	glUniform1i(dirLightPass.get_uniform_location("tMatSpecular"), 4);
	glUniform1i(dirLightPass.get_uniform_location("tMatEmissive"), 5);

	glUniformMatrix4fv(dirLightPass.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(mat4(1.0f)));
	renderer::render(screen_quad);

	glDisable(GL_BLEND);
}

void RenderFrameOnScreen()
{
	gBuffer.BindForFinalPass();
	
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outlineBuffer);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	skyBox.Render(camController.GetCurrentVPMatrix());

	renderer::bind(deferredRendering);

	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthTexture());
	glUniform1i(deferredRendering.get_uniform_location("tDepth"), 3);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, gBuffer.GetFinalTexture());
	glUniform1i(deferredRendering.get_uniform_location("tAlbedo"), 1);

	glUniformMatrix4fv(deferredRendering.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(mat4(1.0f)));

	renderer::render(screen_quad);
}

void RenderOutline()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	renderer::bind(outline);

	// MVP is now the identity matrix
	glUniformMatrix4fv(outline.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(mat4(1.0)));

	// Bind depth texture
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, outlineTexture);
	// Bind depth texture
	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthTexture());

	// Set the tex uniform
	glUniform1i(outline.get_uniform_location("tPosition"), 0);
	glUniform1i(outline.get_uniform_location("tAlbedo"), 1);
	glUniform1i(outline.get_uniform_location("tNormals"), 2);
	glUniform1i(outline.get_uniform_location("tDepth"), 3);
	// Set the tex uniform
	// Set the screen_size uniform
	glUniform2fv(outline.get_uniform_location("screen_size"), 1, value_ptr(vec2(renderer::get_screen_width(), renderer::get_screen_height())));
	// Set the T uniform
	glUniform1f(outline.get_uniform_location("blend_value"), blendValue);
	glUniform1f(outline.get_uniform_location("flattening_value"), flatValue);
	glUniform1f(outline.get_uniform_location("near_distance"), 0.2f);
	glUniform1f(outline.get_uniform_location("far_distance"), 1.0f);
	// Set outline colour
	glUniform4fv(outline.get_uniform_location("outline_colour"), 1, value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
	// Debug
	glUniform1i(outline.get_uniform_location("depth_only"), depthOnly);
	// Render the screen quad
	renderer::render(screen_quad);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SetupLights()
{
	pLight = mesh(geometry_builder::create_sphere());

	// Directional light
	ambientLight.set_ambient_intensity(vec4(0.05f));
	ambientLight.set_light_colour(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	ambientLight.set_direction(normalize(vec3(1.0f, 1.0f, -1.0f)));
	
	// Point lights
	pointLights[0].set_position(vec3(-15.0f, 4.0f, -15.0f));
	pointLights[0].set_light_colour(vec4(2.0f, 6.0f, 3.3f, 1.0f));
	pointLights[0].set_range(3.0f);

	pointLights[1].set_position(vec3(-15.0f, 4.0f, 10.0f));
	pointLights[1].set_light_colour(vec4(2.3f, 1.0f, 4.2f, 1.0f));
	pointLights[1].set_range(dist(engine) * 5.0f + 2.0f);

	pointLights[2].set_position(vec3(5.0f, 10.0f, -7.0f));
	pointLights[2].set_light_colour(vec4(0.24f, 0.20f, 0.31f, 1.0f));
	pointLights[2].set_range(4.0f + dist(engine) * 3.0f);

	pointLights[3].set_position(vec3(2.0f, 4.0f, 20.0f));
	pointLights[3].set_light_colour(vec4(0.0f, 0.21f, 0.19f, 1.0f));
	pointLights[3].set_range(5.0f + dist(engine) * 4.0f);
	
}

void SetupGeometry()
{
	mesh boxMesh = mesh(geometry_builder::create_box());
	boxMesh.get_material().set_emissive(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	boxMesh.get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	boxMesh.get_material().set_shininess(10.0f);
	boxMesh.get_material().set_diffuse(vec4(0.5f, 0.5f, 1.0f, 1.0f));

	mesh cylinderMesh = mesh(geometry_builder::create_cylinder());
	cylinderMesh.get_material().set_emissive(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	cylinderMesh.get_material().set_specular(vec4(1.0f, 0.0f, 0.0f, 1.0f));
	cylinderMesh.get_material().set_shininess(10.0f);
	cylinderMesh.get_material().set_diffuse(vec4(0.0f, 0.0f, 1.0f, 1.0f));

	uniform_real_distribution<float> treeDist(-terrainSize.x, terrainSize.x);

	string rocks[] = { "rock_small", "rock_medium", "rock_flat", "rock_big" };
	for(string s : rocks)
	{
		mesh rockMesh = mesh(geometry("res/models/" + s + ".obj"));
		rockMesh.get_material().set_shininess(100.0f);
		rockMesh.get_material().set_specular(vec4(100.0f, 92.0f, 94.0f, 255.0f) / vec4(255.0f));
		rockMesh.get_material().set_diffuse(vec4(1.0f));
		for (int i = 0; i < 150; ++i)
		{
			string rockName = s + to_string(i);

			vec3 randomPos = vec3(treeDist(engine), 0, treeDist(engine)) * terrainSize.x / 2.0f;

			gameObjects[rockName] = new RenderedObject(rockMesh, &textures["stone"]);
			gameObjects[rockName]->get_mesh()->get_transform().position = randomPos;
			gameObjects[rockName]->get_mesh()->get_transform().scale = vec3((dist(engine) + 1.2f) + 0.55f);
			float rot = treeDist(engine) * 60.0f;
			gameObjects[rockName]->get_mesh()->get_transform().rotate(vec3(0.0f, rot, 0.0f));
		}
	}

	mesh treeMesh = mesh(geometry("res/models/tree.obj"));
	treeMesh.get_material().set_shininess(100.0f);
	treeMesh.get_material().set_specular(vec4(54.0f, 92.0f, 61.0f, 255.0f) / vec4(255.0f));
	treeMesh.get_material().set_diffuse(vec4(1.0f));

	// Create game objects
	// Generate some trees
	for (int i = 0; i < 600; ++i)
	{
		string treeName = "tree" + to_string(i);
		vec3 randomPos = vec3(treeDist(engine), 0, treeDist(engine)) * terrainSize.x / 2.0f;

		gameObjects[treeName] = new RenderedObject(treeMesh, &textures["tree"]);
		gameObjects[treeName]->get_mesh()->get_transform().position = randomPos; // vec3(40.0f, 0.0f, -12.0f);
		gameObjects[treeName]->get_mesh()->get_transform().scale = vec3((dist(engine)+1.2f) + 0.55f);
		float rot = treeDist(engine) * 60.0f;
		gameObjects[treeName]->get_mesh()->get_transform().rotate(vec3(0.0f, rot, 0.0f));
		
	}
	// Player object
	player = new Player(boxMesh, &textures["metal"], &normals["metal"]);

	gameObjects["player"] = player;
	gameObjects["player"]->get_mesh()->get_transform().position = vec3(-15.0f, 1.0f, -15.0f);
	gameObjects["player"]->get_mesh()->get_transform().scale = vec3(5.0f, 2.0f, 10.0f);

	gameObjects["turretBase"] = new RenderedObject(boxMesh, &textures["metal"], &normals["metal"]);
	gameObjects["turretBase"]->get_mesh()->get_transform().position = vec3(0.0f, 0.55f, 0.0f);
	gameObjects["turretBase"]->set_parent(gameObjects["player"]);
	gameObjects["turretBase"]->set_local_scale(vec3(3.5f, 1.0f, 3.5f));

	gameObjects["turret"] = new RenderedObject(cylinderMesh, &textures["metal"], &normals["metal"]);
	gameObjects["turret"]->get_mesh()->get_transform().position = vec3(0.0f, 0.75f, 0.0f);
	gameObjects["turret"]->set_parent(gameObjects["turretBase"]);
	gameObjects["turret"]->set_local_scale(vec3(2.0f, 1.0f, 2.0f));

	player->SetTurretObject(gameObjects["turret"]);

	gameObjects["barrel"] = new RenderedObject(cylinderMesh, &textures["metal"], &normals["metal"]);
	gameObjects["barrel"]->get_mesh()->get_transform().position = vec3(0.0f, 0.0f, -2.0f);
	gameObjects["barrel"]->get_mesh()->get_transform().orientation = toQuat(orientate3(vec3(half_pi<float>(), 0.0f, 0.0f)));
	gameObjects["barrel"]->set_parent(gameObjects["turret"]);
	gameObjects["barrel"]->set_local_scale(vec3(1.0f, 3.0f, 1.0f));

	mesh sphereMesh = mesh(geometry_builder::create_sphere());
	sphereMesh.get_material().set_emissive(vec4(1.0f, 0.0f, 0.0f, 1.0f));
	sphereMesh.get_material().set_specular(vec4(0.0f, 1.0f, 0.0f, 1.0f));
	sphereMesh.get_material().set_shininess(2.0f);
	sphereMesh.get_material().set_diffuse(vec4(0.0f, 0.0f, 1.0f, 1.0f));
}