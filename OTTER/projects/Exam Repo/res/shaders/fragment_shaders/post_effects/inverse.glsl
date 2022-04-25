#version 430

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec3 outColor;

uniform layour (binding = 0) sampler2D s_Image;

void main() {
	vec3 color = texture(s_Image, inUV)

	outColor = 1-color;
}