#version 330
precision lowp float;

in fData {
	vec3 to_eye;
	vec3 to_light;
	vec3 normal;
} frag;

out vec4 outColor;

void main() {
	vec3 Kd = vec3(0.4, 0.5, 0.7);
	vec3 Ks = vec3(1.0);
	vec3 Ka = Kd * 0.2;

	vec3 to_light = normalize(frag.to_light);
	vec3 to_eye = normalize(frag.to_eye);
	vec3 normal = normalize(frag.normal);
	vec3 refl = reflect(-to_light, normal);

	float diffuse_factor = max(0.0, dot(to_light, normal));
	float specular_factor = pow(max(dot(to_eye, refl), 0.0), 10.0);

	outColor = vec4(Ka + Kd*diffuse_factor + Ks*specular_factor, 1.0f);
}

