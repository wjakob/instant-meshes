#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 mvp;
uniform float point_size;

in vData {
	vec3 color;
	vec3 normal;
	vec3 dir1, dir2;
} vertices[];

out fData {
	vec3 color;
	vec2 texcoord;
	vec2 dir1, dir2;
} frag;


const vec2 corners[4] = vec2[](vec2(0.0, 1.0), vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(1.0, 0.0));

void main() {
	vec3 n = vertices[0].normal;
	vec3 s = vec3(1.0, 2.0, 4.5);
	s = normalize(s - n*dot(n, s));
	vec3 t = cross(n, s);

	for (int i=0; i<4; ++i) {
		vec4 pos = gl_in[0].gl_Position;
		vec2 corner = corners[i];
		vec3 d = s * (corner.x - 0.5) + t * (corner.y - 0.5);
		pos.xyz += point_size * d;
		gl_Position = mvp * pos;
		frag.texcoord = corners[i];
		frag.color = vertices[0].color;
		frag.dir1 = vec2(dot(vertices[0].dir1, d), dot(cross(n, vertices[0].dir1), d));
		frag.dir2 = vec2(dot(vertices[0].dir2, d), dot(cross(n, vertices[0].dir2), d));
		EmitVertex();
	}
	EndPrimitive();
}
