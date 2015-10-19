#version 330
precision lowp float;

out vec4 outColor;
uniform vec3 fixed_color;

void main() {
	outColor = vec4(fixed_color, 1.0);
}

