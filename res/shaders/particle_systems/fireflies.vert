#version 440 core

uniform mat4 MV;

layout(location = 0) in vec3 position;

void main() {
	gl_Position = MV * vec4(position, 1.0);
}