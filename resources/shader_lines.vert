#version 330

uniform mat4 mvp;

in vec3 color;
in vec3 position;

out vec3 color_frag;

void main() {
	gl_Position = mvp*vec4(position, 1.0);
	color_frag = color;
}
