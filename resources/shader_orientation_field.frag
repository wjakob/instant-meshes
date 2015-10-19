#version 330
precision lowp float;

in fData {
	vec3 color;
} frag;

out vec4 outColor;

void main() {
	outColor = vec4(frag.color, 1.0);
}

