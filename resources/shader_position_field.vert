#version 330

uniform mat4 mvp;
uniform float scale;

in vec3 uv;
in vec3 normal;

void main() {
	gl_Position = mvp*vec4(uv + normal * (scale * (1.0 / 10.0)), 1.0);
}
