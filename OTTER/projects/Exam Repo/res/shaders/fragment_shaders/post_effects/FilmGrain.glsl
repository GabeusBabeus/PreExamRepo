#version 430
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

uniform layout(binding = 0) sampler2D s_Image;
uniform float iAmount;
uniform float iTime;
uniform vec2 pi;

void main ()
{
	vec4 color = texture2D(s_Image, inUV);

	float amount = iAmount;
	float input = (inUV.x + inUV.y * iTime) * pi.y;
	float n = fract(11000 * sin(input));

	amount *= n;
	color += amount;
	color.a = 1.0;
	outColor = color;
}		