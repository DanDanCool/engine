#version 450

layout(location = 0) in vec2 i_position;
layout(location = 1) in vec3 i_color;

layout(location = 0) out vec3 v_color;

void main() {
	gl_Position = vec4(i_position, 0, 1);
	v_color = i_color;
}
