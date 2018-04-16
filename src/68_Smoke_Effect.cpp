#include <glm\glm.hpp>
#include <graphics_framework.h>
#include "MarchingCubes.h"

extern int edgeTable[256];
extern int triTable[256][16];

using namespace std;
using namespace std::chrono;
using namespace graphics_framework;
using namespace glm;

// Maximum number of particles
const unsigned int MAX_PARTICLES = 20;
const unsigned int RESOLUTION = 8;

vec4 positions[RESOLUTION * RESOLUTION * RESOLUTION];
vec4 velocitys[MAX_PARTICLES];

GLuint G_Position_buffer, G_Velocity_buffer;

effect eff;
//effect compute_eff;
arc_ball_camera cam;
double cursor_x = 0.0;
double cursor_y = 0.0;
GLuint vao;
texture tex;

geometry cube;
vec3 cubeSize = vec3(8);
vec3 cubeStep = vec3(1.0f / RESOLUTION);

int currentMetaballs = 20;
GLuint programObject;

bool load_content() {
  default_random_engine rand(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
  uniform_real_distribution<float> dist;

  tex = texture("textures/smoke.png");

  // Initilise particles
  for (unsigned int i = 0; i < MAX_PARTICLES; ++i) {
    positions[i] = vec4(((2.0f * dist(rand)) - 1.0f) / 10.0f, 5.0 * dist(rand), 0.0f, 0.0f);
    velocitys[i] = vec4(0.0f, 0.1f + dist(rand), 0.0f, 0.0f);
  }

  // Load in shaders
  //compute_eff.add_shader("67_Compute_Shader/particle.comp", GL_COMPUTE_SHADER);
  //compute_eff.build();

  GLuint edgeTableTexture;
  GLuint triTableTexture;
  GLuint dataTexture;

  cube.set_type(GL_POINTS);
  vector<vec3> pos;
  for (float k = -1; k < 1.0f; k += cubeStep.z)
  {
	  for (float j = -1; j < 1.0f; j += cubeStep.y)
	  {
		  for (float i = -1; i < 1.0f; i += cubeStep.x) {
			  pos.push_back(vec3(i, j, k));
		  }
	  }
  }
  // Colours
  vector<vec4> colours;
  for (int i = 0; i < pos.size(); ++i)
  {
	  colours.push_back(vec4(1.0f, 1.0f, 1.0f, 1.0f));
  }
  // Add to the geometry
  cube.add_buffer(pos, BUFFER_INDEXES::POSITION_BUFFER);
  cube.add_buffer(colours, BUFFER_INDEXES::COLOUR_BUFFER);

  // Load in shaders
  eff.add_shader("68_Smoke_Effect/smoke.vert", GL_VERTEX_SHADER);
  eff.add_shader("68_Smoke_Effect/smoke.frag", GL_FRAGMENT_SHADER);
  eff.add_shader("68_Smoke_Effect/smoke.geom", GL_GEOMETRY_SHADER);
  eff.build();

  GLuint program = eff.get_program();
  programObject = program;

  // Relink program
  glLinkProgram(program);

  // a useless vao, but we need it bound or we get errors.
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  //Test link success
  GLint ok = false;
  glGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &ok);
  if (!ok) {
	  int maxLength = 4096;
	  char *infoLog = new char[maxLength];
	  glGetInfoLogARB(program, maxLength, &maxLength, infoLog);
	  std::cout << "Link error: " << infoLog << "\n";
	  delete[]infoLog;
  }

  //Program validation
  glValidateProgramARB(program);
  ok = false;
  glGetObjectParameterivARB(program, GL_OBJECT_VALIDATE_STATUS_ARB, &ok);
  if (!ok) {
	  int maxLength = 4096;
	  char *infoLog = new char[maxLength];
	  glGetInfoLogARB(program, maxLength, &maxLength, infoLog);
	  std::cout << "Validation error: " << infoLog << "\n";
	  delete[]infoLog;
  }

  //Bind program object for parameters setting
  glUseProgramObjectARB(program);

  //Edge Table texture//
  //This texture store the 256 different configurations of a marching cube.
  //This is a table accessed with a bitfield of the 8 cube edges states
  //(edge cut by isosurface or totally in or out).
  //(cf. MarchingCubes.cpp)
  glGenTextures(1, &edgeTableTexture);
  glActiveTexture(GL_TEXTURE1);
  //glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, edgeTableTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  //We create an integer texture with new GL_EXT_texture_integer formats
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA16I_EXT, 256, 1, 0,
	  GL_ALPHA_INTEGER_EXT, GL_INT, &edgeTable);
  
  //Triangle Table texture//
  //This texture store the vertex index list for
  //generating the triangles of each configurations.
  //(cf. MarchingCubes.cpp)
  glGenTextures(1, &triTableTexture);
  glActiveTexture(GL_TEXTURE2);
  //glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, triTableTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA16I_EXT, 16, 256, 0,
	  GL_ALPHA_INTEGER_EXT, GL_INT, &triTable);

  //Datafield//
  //Store the volume data to polygonise
  glGenTextures(1, &dataTexture);
  glActiveTexture(GL_TEXTURE0);
  //glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, dataTexture);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  //Generate a distance field to the center of the cube

  float* dataField = new float[128 * 128 * 128];
  for (int k = 0; k<128; k++)
	  for (int j = 0; j<128; j++)
		  for (int i = 0; i<128; i++) {
			  dataField[i + j * 128 + k * 128 * 128] = distance( vec3(i, j, k), vec3(64, 64, 64) / 64.0f );
		  }
  glTexImage3D(GL_TEXTURE_3D, 0, GL_DEPTH_COMPONENT, 128, 128, 128, 0,
	  GL_DEPTH_COMPONENT, GL_FLOAT, dataField);
  delete[] dataField;
  dataField = NULL;

  ////Samplers assignment///
  glUniform1iARB(glGetUniformLocationARB(program, "dataFieldTex"), 0);
  glUniform1iARB(glGetUniformLocationARB(program, "edgeTableTex"), 1);
  glUniform1iARB(glGetUniformLocationARB(program, "triTableTex"), 2);

  ////Uniforms parameters////
  //Initial isolevel
  glUniform1fARB(glGetUniformLocationARB(program, "isolevel"), 1.0f);
  //Step in data 3D texture for gradient computation (lighting)
  glUniform3fARB(glGetUniformLocationARB(program, "dataStep"), 1.0f / 128.0f, 1.0f / 128.0f, 1.0f / 128.0f);

  //Decal for each vertex in a marching cube
  glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[0]"), 0.0f, 0.0f, 0.0f);
  glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[1]"), cubeStep.x, 0.0f, 0.0f);
  glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[2]"), cubeStep.x, cubeStep.y, 0.0f);
  glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[3]"), 0.0f, cubeStep.y, 0.0f);
  glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[4]"), 0.0f, 0.0f, cubeStep.z);
  glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[5]"), cubeStep.x, 0.0f, cubeStep.z);
  glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[6]"), cubeStep.x, cubeStep.y, cubeStep.z);
  glUniform3fARB(glGetUniformLocationARB(program, "vertDecals[7]"), 0.0f, cubeStep.y, cubeStep.z);

  glUseProgramObjectARB(0);
  /*
  ////Samplers assignment///
  glUniform1i(eff.get_uniform_location("dataFieldTex"), 0);
  glUniform1i(eff.get_uniform_location("edgeTableTex"), 1);
  glUniform1i(eff.get_uniform_location("triTableTex"), 2);
  ////Uniforms parameters////
  //Initial isolevel
  glUniform1f(eff.get_uniform_location("isolevel"), 1.0f);
  //Step in data 3D texture for gradient computation (lighting)
  glUniform3f(eff.get_uniform_location("dataStep"),
	  cubeStep.x, cubeStep.y, cubeStep.z);
  //Decal for each vertex in a marching cube
  glUniform3f(eff.get_uniform_location("vertDecals[0]"),
	  0.0f, 0.0f, 0.0f);
  glUniform3f(eff.get_uniform_location("vertDecals[1]"),
	  cubeStep.x, 0.0f, 0.0f);
  glUniform3f(eff.get_uniform_location("vertDecals[2]"),
	  cubeStep.x, cubeStep.y, 0.0f);
  glUniform3f(eff.get_uniform_location("vertDecals[3]"),
	  0.0f, cubeStep.y, 0.0f);
  glUniform3f(eff.get_uniform_location("vertDecals[4]"),
	  0.0f, 0.0f, cubeStep.z);
  glUniform3f(eff.get_uniform_location("vertDecals[5]"),
	  cubeStep.x, 0.0f, cubeStep.z);
  glUniform3f(eff.get_uniform_location("vertDecals[6]"),
	  cubeStep.x, cubeStep.y, cubeStep.z);
  glUniform3f(eff.get_uniform_location("vertDecals[7]"),
	  0.0f, cubeStep.y, cubeStep.z);

  GLint val = GL_FALSE;
  glGetShaderiv(program, GL_COMPILE_STATUS, &val);
  if (val != GL_TRUE)
  {
	  printf("Compilation failed");
	  // compilation failed
  }

  glUseProgram(0);
  */
  /*
  // a useless vao, but we need it bound or we get errors.
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  // *********************************
   //Generate Position Data buffer
  glGenBuffers(1, &G_Position_buffer);
  // Bind as GL_SHADER_STORAGE_BUFFER
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, G_Position_buffer);
  // Send Data to GPU, use GL_DYNAMIC_DRAW
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * MAX_PARTICLES, positions, GL_DYNAMIC_DRAW);

  // Generate Velocity Data buffer
  glGenBuffers(1, &G_Velocity_buffer);
  // Bind as GL_SHADER_STORAGE_BUFFER
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, G_Velocity_buffer);
  // Send Data to GPU, use GL_DYNAMIC_DRAW
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * MAX_PARTICLES, velocitys, GL_DYNAMIC_DRAW);

  // *********************************
   //Unbind
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  */
  // Set camera properties
  cam.set_position(vec3(0.0f, 2.5f, 10.0f));
  cam.set_target(vec3(0.0f, 0, 0.0f));
  cam.set_projection(quarter_pi<float>(), renderer::get_screen_aspect(), 0.1f, 1000.0f);
  return true;
}

bool update(float delta_time) {
  if (delta_time > 10.0f) {
    delta_time = 10.0f;
  }
  //renderer::bind(compute_eff);
  //glUniform3fv(compute_eff.get_uniform_location("max_dims"), 1, &(vec3(3.0f, 5.0f, 5.0f))[0]);
  //glUniform1f(compute_eff.get_uniform_location("delta_time"), delta_time);

  // Update the camera
  double current_x;
  double current_y;
  static const float sh = static_cast<float>(renderer::get_screen_height());
  static const float sw = static_cast<float>(renderer::get_screen_height());
  static const double ratio_width = quarter_pi<float>() / sw;
  static const double ratio_height = (quarter_pi<float>() * (sh / sw)) / sh;

  // Get the current cursor position
  glfwGetCursorPos(renderer::get_window(), &current_x, &current_y);

  // Calculate delta of cursor positions from last frame
  double delta_x = current_x - cursor_x;
  double delta_y = current_y - cursor_y;

  // Multiply deltas by ratios and delta_time - gets actual change in orientation
  delta_x *= ratio_width;
  delta_y *= ratio_height;

  // Rotate cameras by delta
  cam.rotate(delta_y, delta_x);

  // Use UP and DOWN to change camera distance
  if (glfwGetKey(renderer::get_window(), GLFW_KEY_UP)) {
    cam.move(-5.0f * delta_time);
  }
  if (glfwGetKey(renderer::get_window(), GLFW_KEY_DOWN)) {
    cam.move(5.0f * delta_time);
  }

  // Update the camera
  cam.update(delta_time);

  // Update cursor pos
  cursor_x = current_x;
  cursor_y = current_y;
  return true;
}

bool render() {
  // Bind Compute Shader
  //renderer::bind(compute_eff);
  // Bind data as SSBO
  //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, G_Position_buffer);
  //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, G_Velocity_buffer);
  // Dispatch
  //glDispatchCompute(MAX_PARTICLES / 128, 1, 1);
  // Sync, wait for completion
  //glMemoryBarrier(GL_ALL_BARRIER_BITS);
  //glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  // *********************************
  // Bind render effect
			//Shader program binding
	glUseProgramObjectARB(programObject);
	//Current isolevel uniform parameter setting
	glUniform1fARB(glGetUniformLocationARB(programObject, "isolevel"), 1.0f);
  // Create MV matrix
  auto M = mat4(1.0);
  auto V = cam.get_view();
  auto P = cam.get_projection();
  auto MVP = P * V * M;
  


  // Set MV, and P matrix uniforms seperatly
  glUniformMatrix4fv(eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
  glUniformMatrix4fv(eff.get_uniform_location("M"), 1, GL_FALSE, value_ptr(M));
  glUniformMatrix4fv(eff.get_uniform_location("MV_IT"), 1, GL_FALSE, value_ptr(inverse((V * M))));
  
  // glUniform4fv(eff.get_uniform_location("gMetaballs"), currentMetaballs, (const GLfloat*)positions);
  // Set point_size size uniform to .1f
  //glUniform1i(eff.get_uniform_location("gMetaballCount"), currentMetaballs);
  // Bind particle texture
  //renderer::bind(tex, 0);
  //glUniform1i(eff.get_uniform_location("tex"), 0);

   renderer::render(cube);
  /*
  // Bind position buffer as GL_ARRAY_BUFFER
  glBindBuffer(GL_ARRAY_BUFFER, G_Position_buffer);
  // Setup vertex format
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void *)0);
  // Enable Blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // Disable Depth Mask
  glDepthMask(GL_FALSE);
  // Render
  glDrawArrays(GL_LINES_ADJACENCY, 0, MAX_PARTICLES);
  // Tidy up, enable depth mask
  glDepthMask(GL_TRUE);
  // Disable Blend
  glDisable(GL_BLEND);
  // Unbind all arrays
  glDisableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  */
  glUseProgram(0);
  return true;
}

void main() {
  // Create application
  app application("68_Smoke_Effect");
  // Set load content, update and render methods
  application.set_load_content(load_content);
  application.set_update(update);
  application.set_render(render);
  // Run application
  application.run();
}