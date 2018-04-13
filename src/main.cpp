#include <glm\glm.hpp>
#include <graphics_framework.h>
#include "InputHandler.h"
#include "CameraController.h"
#include "RenderedObject.h"
#include "Player.h"
#include "GBuffer.h"

using namespace std;
using namespace graphics_framework;
using namespace glm;

// Geometry
map<string, RenderedObject*> gameObjects;

// Shaders
effect specular;
effect outline;

effect stencilPass;
effect dirLightPass;
effect pointLightPass;

effect deferredShading;
effect deferredRendering;

// Lights
mesh pLight;
vector<point_light> pointLights(4);
vector<spot_light> spotLights(2);
directional_light ambientLight;

// Textures
map<string, texture> textures;
map<string, texture> normals;

// Etc
shadow_map shadow;

Player* player;
chase_camera playerCamera;
CameraController camController;

// Post-process
vec2 screenSize;
frame_buffer frame;
geometry screen_quad;

// Input vars
double mouse_x = 0.0, mouse_y = 0.0;

void SetupLights();
void SetupGeometry();

void DSGeometryPass();
void DSPointLightPass(int lightIndex);
void DSStencilPass(int lightIndex);
void DSDirectionalLightPass();
void RenderFrameOnScreen();
void RenderOutline();

float CalcPointLightSphere(point_light light);

int depthOnly = 0;

// Deferred render vars
GBuffer gBuffer; // unsigned int gBuffer
unsigned int gPosition, gNormal, gAlbedo, gDepth;
unsigned int gPositionTexture, gNormalTexture, gAlbedoTexture, gDepthTexture;

bool initialise() {
	// Set input mode - hide the cursor
	glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// Capture initial mouse position
	glfwGetCursorPos(renderer::get_window(), &mouse_x, &mouse_y);

	// Set custom input callbacks
	glfwSetInputMode(renderer::get_window(), GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSetKeyCallback(renderer::get_window(), InputHandler::KeyboardHandler);
	glfwSetMouseButtonCallback(renderer::get_window(), InputHandler::MouseButtonHandler);
	glfwSetCursorPosCallback(renderer::get_window(), InputHandler::MousePosHandler);

	camController.Initialize();
	return true;
}

bool load_content() {
	screenSize = vec2( renderer::get_screen_width(), renderer::get_screen_height() );

	// Create frame buffer - use screen width and height
	frame = frame_buffer(screenSize.x, screenSize.y);
	// Create screen quad
	vector<vec3> positions{ vec3(-1.0f, -1.0f, 0.0f), vec3(1.0f, -1.0f, 0.0f), vec3(-1.0f, 1.0f, 0.0f),
		vec3(1.0f, 1.0f, 0.0f) };
	vector<vec2> tex_coords{ vec2(0.0, 0.0), vec2(1.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 1.0f) };
	screen_quad.add_buffer(positions, BUFFER_INDEXES::POSITION_BUFFER);
	screen_quad.add_buffer(tex_coords, BUFFER_INDEXES::TEXTURE_COORDS_0);
	screen_quad.set_type(GL_TRIANGLE_STRIP);

	shadow = shadow_map(screenSize.x, screenSize.y);

	// Load up textures
	textures["empty"] = texture("res/textures/empty_texture.png");
	textures["dirt"] = texture("res/textures/dirt.jpg");
	textures["brick"] = texture("res/textures/brick.jpg");
	textures["metal"] = texture("res/textures/metal.png");

	normals["empty"] = texture("res/textures/normals/empty_normal.png");
	normals["dirt"] = texture("res/textures/normals/dirt_normal.jpg");
	normals["brick"] = texture("res/textures/normals/brick_normal.jpg");
	normals["metal"] = texture("res/textures/normals/metal_normal.png");

	// Create some geometry
	SetupGeometry();

	// Create a camera for the player
	playerCamera.set_pos_offset(vec3(0.0f, 4.0f, 10.0f));
	playerCamera.set_springiness(0.5f);
	playerCamera.set_projection(quarter_pi<float>(), renderer::get_screen_aspect(), 0.1f, 1000.0f);
	playerCamera.move(gameObjects["turretBase"]->get_mesh()->get_transform().position, eulerAngles(gameObjects["turretBase"]->get_mesh()->get_transform().orientation));
	camController.AddNewCamera("playerCamera", &playerCamera);

	// Create lights
	SetupLights();

	// Load in shaders
	specular.add_shader("res/shaders/shader.vert", GL_VERTEX_SHADER);
	// Name of fragment shaders required
	vector<string> frag_shaders{ "res/shaders/shader.frag", "res/shaders/part_direction.frag",
		  "res/shaders/part_point.frag", "res/shaders/part_spot.frag", "res/shaders/part_shadow.frag" };
	specular.add_shader(frag_shaders, GL_FRAGMENT_SHADER);
	specular.add_shader("res/shaders/part_normal_map.frag", GL_FRAGMENT_SHADER);
	// Build effect
	specular.build();

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

	auto player = (Player*)gameObjects["player"];
	player->Initialize();

	gBuffer = GBuffer();
	gBuffer.Init(screenSize.x, screenSize.y);

	return true;
}


bool update(float delta_time) {
	InputHandler::Update(delta_time);

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

			//gameObjects["turret"]->get_mesh()->get_transform().rotate(vec3(0.0f, dx*delta_time, 0.0f));

			playerCamera.rotate(vec3(-dy, -dx, 0.0f));
			playerCamera.set_target_pos(gameObjects["turretBase"]->get_world_position());
			playerCamera.set_target_rotation(eulerAngles(gameObjects["turretBase"]->get_world_orientation()));

			player->RotateTurret(-dx);
		}

		currentCam->update(delta_time);
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

	player->update(delta_time);

	// Update the shadow map light_position from the spot light
	shadow.light_position = spotLights[0].get_position();
	// do the same for light_dir property
	shadow.light_dir = spotLights[0].get_direction();

	glfwPollEvents();

  return true;
}

bool render() {

	gBuffer.StartFrame();

	DSGeometryPass();

	// Light passes
	/*
	glEnable(GL_STENCIL_TEST);

	for (int i = 0; i < pointLights.size(); ++i)
	{
		// Point lights get a stencil test 
		DSStencilPass(i);
		DSPointLightPass(i);
	}

	glDisable(GL_STENCIL_TEST);
	*/
	DSDirectionalLightPass();
	
	RenderFrameOnScreen();

	// RenderOutline();

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

	glDepthMask(GL_TRUE);
	// Clear the render targets
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glEnable(GL_DEPTH_TEST);

	camera* currentCam = camController.GetActiveCamera();
	mat4 V = currentCam->get_view();
	mat4 P = currentCam->get_projection();
	auto VP = P * V;
	mat4 LightProjectionMat = perspective<float>(90.f, renderer::get_screen_aspect(), 0.1f, 1000.f);

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

	if (0) // Debug lights
	{
		for (int i = 0; i < pointLights.size(); ++i)
		{
			pLight.get_transform().position = pointLights[i].get_position();
			pLight.get_transform().scale = vec3(CalcPointLightSphere(pointLights[i]));

			auto M = pLight.get_transform().get_transform_matrix();
			mat4 MVP = VP * M;

			renderer::bind(textures["empty"], 0);
			renderer::bind(textures["empty"], 1);

			// Set MVP matrix uniform
			glUniformMatrix4fv(deferredShading.get_uniform_location("MVP"), // Location of uniform
				1,                               // Number of values - 1 mat4
				GL_FALSE,                        // Transpose the matrix?
				value_ptr(MVP));                 // Pointer to matrix data
			glUniformMatrix4fv(deferredShading.get_uniform_location("M"), 1, GL_FALSE, value_ptr(M));
			// Set N matrix uniform - remember - 3x3 matrix
			glUniformMatrix3fv(deferredShading.get_uniform_location("N"), 1, GL_FALSE, value_ptr(pLight.get_transform().get_normal_matrix()));
			// Set empty textures
			glUniform1i(deferredShading.get_uniform_location("tex_diffuse"), 0);
			glUniform1i(deferredShading.get_uniform_location("normal_map"), 1);

			renderer::render(pLight);
		}
	}

	glDepthMask(GL_FALSE);
}

float CalcPointLightSphere(point_light light)
{
	vec4 colour = light.get_light_colour();
	float MaxChannel = fmax(fmax(colour.x, colour.y), colour.z);

	float linear_attenuation = light.get_linear_attenuation();
	float quad_attenuation = light.get_quadratic_attenuation();
	
	float ret = (-linear_attenuation + sqrtf(linear_attenuation * linear_attenuation -
		4 * quad_attenuation * (quad_attenuation - 256 * MaxChannel * light.get_constant_attenuation())))
		/ (2 * quad_attenuation);
	return ret / 3.0f;
}

void DSStencilPass(int lightIndex)
{
	camera* cam = camController.GetActiveCamera();
	auto V = cam->get_view();
	auto P = cam->get_projection();

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

	pLight.get_transform().position = pointLights[lightIndex].get_position();
	pLight.get_transform().scale = vec3(CalcPointLightSphere(pointLights[lightIndex]));

	auto M = pLight.get_transform().get_transform_matrix();
	auto MVP = P * V * M;

	renderer::bind(pointLights[lightIndex], "gPointLight");

	glUniformMatrix4fv(stencilPass.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
	renderer::render(pLight);
}

void DSPointLightPass(int lightIndex)
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
	auto V = cam->get_view();
	auto P = cam->get_projection();

	pLight.get_transform().position = pointLights[lightIndex].get_position();
	pLight.get_transform().scale = vec3(CalcPointLightSphere(pointLights[lightIndex]));

	auto M = pLight.get_transform().get_transform_matrix();
	auto MVP = P * V * M;

	renderer::bind(pointLights[lightIndex], "gPointLight");
	glUniform3fv(pointLightPass.get_uniform_location("gEyeWorldPos"), 1, value_ptr(cam->get_position()));
	glUniform2fv(pointLightPass.get_uniform_location("gScreenSize"), 1, value_ptr(screenSize));

	glUniform1i(pointLightPass.get_uniform_location("tPosition"), 0);
	glUniform1i(pointLightPass.get_uniform_location("tAlbedo"), 1);
	glUniform1i(pointLightPass.get_uniform_location("tNormals"), 2);

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

	glUniformMatrix4fv(dirLightPass.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(mat4(1.0f)));
	renderer::render(screen_quad);

	glDisable(GL_BLEND);
}

void RenderFrameOnScreen()
{
	gBuffer.BindForFinalPass();

	renderer::bind(deferredRendering);

	glUniform1i(deferredRendering.get_uniform_location("tPosition"), 0);
	glUniform1i(deferredRendering.get_uniform_location("tAlbedo"), 1);
	glUniform1i(deferredRendering.get_uniform_location("tNormals"), 2);
	glUniform1i(deferredRendering.get_uniform_location("tDiffuse"), 3);

	glUniform1i(deferredRendering.get_uniform_location("depthOnly"), depthOnly);
	glUniformMatrix4fv(deferredRendering.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(mat4(1.0f)));

	renderer::render(screen_quad);
}

void RenderOutline()
{
	renderer::set_render_target();
	renderer::bind(outline);

	// MVP is now the identity matrix
	glUniformMatrix4fv(outline.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(mat4(1.0)));
	// Bind texture from frame buffer
	renderer::bind(frame.get_frame(), 0);

	glUniform1i(outline.get_uniform_location("tPosition"), 0);
	glUniform1i(outline.get_uniform_location("tAlbedo"), 1);
	glUniform1i(outline.get_uniform_location("tNormals"), 2);
	// Bind depth texture
	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, gDepthTexture);
	// Set the tex uniform
	glUniform1i(outline.get_uniform_location("depth"), 1);
	// Set the screen_size uniform
	glUniform2fv(outline.get_uniform_location("screen_size"), 1, value_ptr(vec2(renderer::get_screen_width(), renderer::get_screen_height())));
	// Set the T uniform
	glUniform1f(outline.get_uniform_location("blend_value"), 50.0f);
	glUniform1f(outline.get_uniform_location("flattening_value"), -10.0f);
	glUniform1f(outline.get_uniform_location("near_distance"), 6000.0f);
	glUniform1f(outline.get_uniform_location("far_distance"), 90000.0f);
	// Set outline colour
	glUniform4fv(outline.get_uniform_location("outline_colour"), 1, value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
	// Debug
	glUniform1i(outline.get_uniform_location("depth_only"), depthOnly);
	// Render the screen quad
	renderer::render(screen_quad);
}

void SetupLights()
{
	pLight = mesh(geometry_builder::create_sphere());

	// Directional light
	ambientLight.set_ambient_intensity(vec4(0.2f));
	ambientLight.set_light_colour(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	ambientLight.set_direction(normalize(vec3(0.2f, -1.0f, 0.35f)));

	// Point lights
	pointLights[0].set_position(vec3(-15.0f, 4.0f, -15.0f));
	pointLights[0].set_light_colour(vec4(1.0f, 0.2f, 0.3f, 1.0f));
	pointLights[0].set_range(7.0f);

	pointLights[1].set_position(vec3(-15.0f, 4.0f, 10.0f));
	pointLights[1].set_light_colour(vec4(0.1f, 0.2f, 0.7f, 1.0f));
	pointLights[1].set_range(5.0f + rand() % 10);

	pointLights[2].set_position(vec3(5.0f, 10.0f, -7.0f));
	pointLights[2].set_light_colour(vec4(0.3f, 1.0f, 0.3f, 1.0f));
	pointLights[2].set_range(5.0f + rand() % 10);

	pointLights[3].set_position(vec3(2.0f, 4.0f, 20.0f));
	pointLights[3].set_light_colour(vec4(1.0f, 0.2f, 1.0f, 1.0f));
	pointLights[3].set_range(5.0f + rand() % 10);

	// Spot lights
	spotLights[0].set_position(vec3(0.0f, 10.0f, 10.0f));
	spotLights[0].set_light_colour(vec4(1.0f, 1.0f, 0.0f, 1.0f));
	spotLights[0].set_direction(normalize(vec3(-2.0f, -5.0f, -4.0f)));
	spotLights[0].set_range(25.0f);
	spotLights[0].set_power(0.5f); 

	spotLights[1].set_position(vec3(15.0f, 7.0f, -5.0f));
	spotLights[1].set_light_colour(vec4(0.1f, 0.6f, 0.9f, 1.0f));
	spotLights[1].set_direction(normalize(vec3(-0.4f, -5.0f, 2.0f)));
	spotLights[1].set_range(35.0f);
	spotLights[1].set_power(0.5f);

}

void SetupGeometry()
{// Ground
	mesh floorMesh = mesh(geometry_builder::create_plane());
	floorMesh.get_material().set_emissive(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	floorMesh.get_material().set_diffuse(vec4(1.0f, 1.0f, 0.0f, 1.0f));
	floorMesh.get_material().set_specular(vec4(.0f, .0f, .0f, 1.0f));
	floorMesh.get_material().set_shininess(150.0f);

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

	mesh sphereMesh = mesh(geometry_builder::create_sphere());
	sphereMesh.get_material().set_emissive(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	sphereMesh.get_material().set_specular(vec4(0.0f, 1.0f, 1.0f, 1.0f));
	sphereMesh.get_material().set_shininess(10.0f);
	sphereMesh.get_material().set_diffuse(vec4(0.2f, 0.2f, 0.7f, 1.0f));

	// Create game objects
	// Ground object
	gameObjects["floor"] = new RenderedObject(floorMesh, &specular, &textures["dirt"], &normals["dirt"]);
	gameObjects["floor"]->get_mesh()->get_transform().position = vec3(0.0f, 0.0f, 0.0f);

	// Player object
	player = new Player(boxMesh, &specular, &textures["metal"], &normals["metal"]);

	gameObjects["player"] = player;
	gameObjects["player"]->get_mesh()->get_transform().position = vec3(-15.0f, 1.0f, -15.0f);
	gameObjects["player"]->get_mesh()->get_transform().scale = vec3(5.0f, 2.0f, 10.0f);

	gameObjects["turretBase"] = new RenderedObject(boxMesh, &specular, &textures["metal"], &normals["metal"]);
	gameObjects["turretBase"]->get_mesh()->get_transform().position = vec3(0.0f, 0.55f, 0.0f);
	gameObjects["turretBase"]->set_parent(gameObjects["player"]);
	gameObjects["turretBase"]->set_local_scale(vec3(3.5f, 1.0f, 3.5f));

	gameObjects["turret"] = new RenderedObject(cylinderMesh, &specular, &textures["metal"], &normals["metal"]);
	gameObjects["turret"]->get_mesh()->get_transform().position = vec3(0.0f, 0.75f, 0.0f);
	gameObjects["turret"]->set_parent(gameObjects["turretBase"]);
	gameObjects["turret"]->set_local_scale(vec3(2.0f, 1.0f, 2.0f));

	player->SetTurretObject(gameObjects["turret"]);

	gameObjects["barrel"] = new RenderedObject(cylinderMesh, &specular, &textures["metal"], &normals["metal"]);
	gameObjects["barrel"]->get_mesh()->get_transform().position = vec3(0.0f, 0.0f, -2.0f);
	gameObjects["barrel"]->get_mesh()->get_transform().orientation = toQuat(orientate3(vec3(half_pi<float>(), 0.0f, 0.0f)));
	gameObjects["barrel"]->set_parent(gameObjects["turret"]);
	gameObjects["barrel"]->set_local_scale(vec3(1.0f, 3.0f, 1.0f));

	// Some other objects
	gameObjects["wall"] = new RenderedObject(boxMesh, &specular, &textures["brick"], &normals["brick"]);
	gameObjects["wall"]->get_mesh()->get_transform().position = vec3(1.0f, 1.0f, 0.0f);
	gameObjects["wall"]->set_local_scale(vec3(3.0f, 3.0f, 1.0f));

	gameObjects["wall2"] = new RenderedObject(boxMesh, &specular, &textures["brick"], &normals["brick"]);
	gameObjects["wall2"]->get_mesh()->get_transform().position = vec3(4.0f, 1.0f, 0.0f);
	gameObjects["wall2"]->set_local_scale(vec3(3.0f, 3.0f, 1.0f));

	gameObjects["wall3"] = new RenderedObject(boxMesh, &specular, &textures["brick"], &normals["brick"]);
	gameObjects["wall3"]->get_mesh()->get_transform().position = vec3(7.0f, 1.0f, 0.0f);
	gameObjects["wall3"]->set_local_scale(vec3(3.0f, 3.0f, 1.0f));

	gameObjects["sphere"] = new RenderedObject(sphereMesh, &specular, &textures["metal"], &normals["metal"]);
	gameObjects["sphere"]->get_mesh()->get_transform().position = vec3(-10.0f, 2.0f, 0.0f);
}