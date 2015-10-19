#version 330
precision lowp float;

out vec4 outColor;
in vec3 color_frag;

void main() {
	if (color_frag == vec3(0.0))
		discard;
	outColor = vec4(color_frag, 1.0);
}

