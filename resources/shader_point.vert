#version 330

in vec3 position;
in vec3 normal;
in vec3 tangent;
in vec3 uv;
in vec4 color;

out vData {
	vec3 normal;
	vec3 tangent;
	vec3 uv;
	vec4 color;
} vertex;

void main() {
	gl_Position = vec4(position, 1.0);
	vertex.normal = normal;
	vertex.tangent = tangent;
	vertex.uv = uv;
	vertex.color = color;
}
