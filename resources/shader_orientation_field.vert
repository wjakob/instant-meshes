#version 330

in vec3 position;
in vec3 normal;
in vec3 tangent;

out vData {
	vec3 normal;
	vec3 tangent;
} vertex;

void main() {
	gl_Position = vec4(position, 1.0);
	vertex.normal = normal;
	vertex.tangent = tangent;
}
