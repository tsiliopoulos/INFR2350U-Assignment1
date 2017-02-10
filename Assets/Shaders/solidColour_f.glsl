#version 400

uniform vec4 u_lightPos;
uniform vec4 u_colour;

// Fragment Shader Inputs
in VertexData
{
	vec3 normal;
	vec3 texCoord;
	vec4 colour;
	vec3 posEye;
} vIn;

layout(location = 0) out vec4 FragColor;

void main()
{

	FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}