#version 330
precision lowp float;

in fData {
	vec2 texcoord;
	vec3 color;
	vec2 dir1, dir2;
} frag;

out vec4 outColor;

void main() {
	if (length(frag.texcoord.xy-vec2(0.5)) > 0.5)
		discard;
	vec3 col = frag.color;

	float size = 0.5;
	if (frag.dir1 != vec2(0.0) && frag.dir2 != vec2(0.0))
		size = 0.35;

	if (frag.dir1 != vec2(0.0) && abs(atan(frag.dir1.y, frag.dir1.x)) < size)
		col = vec3(0.0);
	if (frag.dir2 != vec2(0.0) && abs(atan(frag.dir2.y, frag.dir2.x)) < size)
		col = vec3(0.0);
	outColor = vec4(col, 1.0);
}
