#version 330

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform vec3 light_position;
uniform mat4 proj, model, view;
uniform float scale, inv_scale;
uniform float show_uvs;
uniform vec3 camera_local;

in vData {
	vec3 normal;
	vec3 normal_data;
	vec3 tangent;
	vec3 uv;
	vec4 color;
} vertices[];

out fData {
	vec3 to_eye;
	vec3 to_light;
	vec3 normal;
	vec2 texcoord;
	vec4 color;
} frag;

#if ROSY == 2
	vec3 compat_orientation(vec3 q, vec3 ref, vec3 n) {
		return q * sign(dot(q, ref));
	}
#elif ROSY == 4
	vec3 compat_orientation(vec3 q, vec3 ref, vec3 n) {
		vec3 t = cross(n, q);
		float dp0 = dot(q, ref), dp1 = dot(t, ref);
		if (abs(dp0) > abs(dp1))
			return q * sign(dp0);
		else
			return t * sign(dp1);
	}
#else
	vec3 rotate60(vec3 d, vec3 n) { return 0.8660254037 * cross(n, d) + 0.5 * (d + n * dot(n, d)); }

	vec3 compat_orientation(vec3 q, vec3 ref, vec3 n) {
		vec3 t[3] = vec3[](rotate60(q, -n), q, rotate60(q, n));
		float dp[3] = float[](dot(t[0], ref), dot(t[1], ref), dot(t[2], ref));
		float abs_dp[3] = float[](abs(dp[0]), abs(dp[1]), abs(dp[2]));

		if (abs_dp[0] >= abs_dp[1] && abs_dp[0] >= abs_dp[2])
			return t[0] * sign(dp[0]);
		else if (abs_dp[1] >= abs_dp[0] && abs_dp[1] >= abs_dp[2])
			return t[1] * sign(dp[1]);
		else
			return t[2] * sign(dp[2]);
	}
#endif

#if POSY == 4
	vec3 compat_position(vec3 o, vec3 ref, vec3 q, vec3 t, vec3 n) {
		vec3 d = ref - o;
		return o +
		   q * round(dot(q, d) * inv_scale) * scale +
		   t * round(dot(t, d) * inv_scale) * scale;
	}
#else
	vec3 compat_position(vec3 o, vec3 ref, vec3 q, in vec3 t, vec3 n) {
		vec3 d = ref - o;
		t = rotate60(q, n);

		float dpq = dot(q, d), dpt = dot(t, d);
		float u = floor(( 4*dpq - 2*dpt) * (1.0f / 3.0f) * inv_scale);
		float v = floor((-2*dpq + 4*dpt) * (1.0f / 3.0f) * inv_scale);

		q *= scale; t *= scale;
		o = o + q*u + t*v - ref;

		vec3 candidates[] = vec3[4](o, o+q, o+t, o+q+t);

		float best_length = 1e20;
		int best_index = -1;
		for (int i=0; i<4; ++i) {
			float length = dot(candidates[i], candidates[i]);
			if (length < best_length) {
				best_length = length;
				best_index = i;
			}
		}
		return candidates[best_index] + ref;
	}
#endif

void main() {
	vec3 face_normal = normalize(cross(
		gl_in[1].gl_Position.xyz-gl_in[0].gl_Position.xyz,
		gl_in[2].gl_Position.xyz-gl_in[0].gl_Position.xyz));

	if (dot(gl_in[0].gl_Position.xyz - camera_local, face_normal) > 0.0)
		return;

	vec2 texcoord[3];
	if (show_uvs == 1.0) {
		/* Step 1: Rotate everthing into the triangle plane */
		vec3 tangents[3], uv[3];
		for (int i=0; i<3; ++i) {
			float cosTheta = dot(vertices[i].normal_data, face_normal);

			if (cosTheta < 0.9999f) {
				vec3 axis = cross(vertices[i].normal_data, face_normal);
				float sinTheta2 = dot(axis, axis),
					  factor = (1.0 - cosTheta) / sinTheta2;

				vec3 v_tangent = vertices[i].tangent;
				tangents[i] = v_tangent * cosTheta + cross(axis, v_tangent)
					+ axis * dot(axis, v_tangent) * factor;

				vec3 v_uv = vertices[i].uv - gl_in[i].gl_Position.xyz;
				uv[i] = v_uv * cosTheta + cross(axis, v_uv)
					+ axis * dot(axis, v_uv) * factor + gl_in[i].gl_Position.xyz;
			} else {
				tangents[i] = vertices[i].tangent;
				uv[i] = vertices[i].uv;
			}
		}

		/* Step 2: search orientation field quotient space */
		for (int i=1; i<3; ++i)
			tangents[i] = compat_orientation(tangents[i], tangents[0], face_normal);

		vec3 bitangents[3];
		for (int i=0; i<3; ++i)
			bitangents[i] = cross(face_normal, tangents[i]);

		/* Step 3: search position field quotient space */
		for (int i=1; i<3; ++i)
			uv[i] = compat_position(uv[i], uv[0], tangents[i], bitangents[i], face_normal);

		/* Step 4: compute uv coordinates */
		for (int i=0; i<3; ++i) {
			vec3 rel = gl_in[i].gl_Position.xyz - uv[i];
			texcoord[i] = vec2(
				dot(rel, tangents[i]) * inv_scale,
				dot(rel, bitangents[i]) * inv_scale
			);
		}
	}

	for (int i=0; i<3; ++i) {
		vec4 pos_camera = view * (model * gl_in[i].gl_Position);
		vec3 vn = vertices[i].normal;

		gl_Position = proj * pos_camera;
		frag.to_light = (view * vec4(light_position, 1.0)).xyz - pos_camera.xyz;
		frag.to_eye = -pos_camera.xyz;
		frag.normal = (model * (view * vec4(vn, 0.0))).xyz;
		frag.texcoord = texcoord[i];
		frag.color = vertices[i].color;

		EmitVertex();
	}
	EndPrimitive();
}
