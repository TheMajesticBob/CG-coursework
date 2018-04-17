#version 440 core

uniform int gMetaballCount;

layout(location = 0) in vec3 position;

layout(location = 0) out float height;

void main() {
	gl_Position = vec4(position, 1.0);
	height = position.y;
}