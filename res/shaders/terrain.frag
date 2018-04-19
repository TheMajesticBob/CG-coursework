#version 440

// Forward declaration
vec4 weighted_texture(in sampler2D tex[4], in vec2 tex_coord, in vec4 weights);

// Textures
uniform sampler2D tex[4];

// Incoming vertex position
layout(location = 0) in vec4 position;
// Incoming normal
layout(location = 1) in vec4 normal;
// Incoming tex_coord
layout(location = 2) in vec2 tex_coord;
// Incoming tex_weight
layout(location = 3) in vec4 tex_weight;
// Incoming diffuse
layout(location = 4) in vec4 diffuse;
// Incoming specular
layout(location = 5) in vec4 specular;

// Outgoing colour
layout(location = 0) out vec4 position_out;
layout(location = 1) out vec4 diffuse_out;
layout(location = 2) out vec4 normal_out;
layout(location = 3) out vec4 mat_diffuse_out;
layout(location = 4) out vec4 mat_specular_out;

void main() {
  // Get tex colour
  vec4 tex_colour = weighted_texture(tex, tex_coord, tex_weight);

  position_out = position;
  diffuse_out = tex_colour;
  normal_out = normal;
  mat_diffuse_out = diffuse;
  mat_specular_out = specular;
}