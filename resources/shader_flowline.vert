#version 330

uniform mat4 mvp;

in vec3 position;
in vec4 color;

out fData {
	out vec4 color;
} frag;

void main() {
	gl_Position = mvp * vec4(position, 1.0);
	frag.color = color;
}
