#version 330
precision lowp float;

in fData {
	vec4 color;
} frag;

out vec4 outColor;
uniform float alpha;

void main() {
	vec3 result = frag.color.rgb;
	if (abs(frag.color.a-0.5) < 0.1)
		result *= smoothstep(0, 1, abs(frag.color.a-0.5)/0.1) * 0.5 + 0.5;
	outColor = vec4(result, alpha);
}

