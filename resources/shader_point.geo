#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec3 light_position;
uniform mat4 proj, model, view;
uniform float scale, inv_scale;
uniform float show_uvs;
uniform float point_size;

in vData {
	vec3 normal;
	vec3 tangent;
	vec3 uv;
	vec4 color;
} vertices[];

out fData {
	vec3 to_eye;
	vec3 to_light;
	vec3 normal;
	vec2 texcoord;
	vec2 texcoord_point;
	vec4 color;
} frag;

const vec2 corners[4] = vec2[](vec2(0.0, 1.0), vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(1.0, 0.0));

void main() {
	vec3 n = vertices[0].normal;
	vec3 s = vertices[0].tangent;
	vec3 t = cross(n, s);
	frag.normal = (model * (view * vec4(n, 0.0))).xyz;
	frag.color = vertices[0].color;

	for (int i=0; i<4; ++i) {
		vec2 corner = corners[i];
		vec3 d = s * (corner.x - 0.5) + t * (corner.y - 0.5);
		vec3 pos = gl_in[0].gl_Position.xyz + point_size * d;
		vec4 pos_camera = view * (model * vec4(pos, 1.0));

		frag.to_light = (view * vec4(light_position, 1.0)).xyz - pos_camera.xyz;
		frag.to_eye = -pos_camera.xyz;

		vec3 rel = pos - vertices[0].uv;
		frag.texcoord = vec2(
			dot(rel, s) * inv_scale,
			dot(rel, t) * inv_scale
		);
		frag.texcoord_point = corners[i];

		gl_Position = proj * pos_camera;
		EmitVertex();
	}
	EndPrimitive();
}
