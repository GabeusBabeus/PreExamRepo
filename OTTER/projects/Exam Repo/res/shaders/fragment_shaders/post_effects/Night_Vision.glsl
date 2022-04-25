#version 430
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;
uniform float t;

//Night Effect Shader
uniform layout(binding = 0) sampler2D s_Image;
uniform layout(binding = 1) sampler2D TexNoise;
uniform layout(binding = 2) sampler2D TexMask;

uniform float L = 0.35; 
uniform float A = 5.1;
uniform float E = 1.1; 

void main ()
{
  vec4 finalColor;
  if (inUV.x < E) 
  {
    vec2 uv;           
    
    uv.x = 0.31*sin(t*46.0);                                 
    uv.y = 0.31*cos(t*46.0);                                 
    
    float m = texture2D(TexMask, inUV.st).r;
    vec3 n = texture2D(TexNoise, (inUV.st*2.56) + uv).rgb;
    vec3 v = texture2D(s_Image, inUV.st + (n.xy*0.010)).rgb;
  
    float l = dot(vec3(0.30, 0.59, 0.11), v);
    if (l < L)
      v *= A; 
  
    vec3 f = vec3(0.16, 1.1, 0.16);
    finalColor.rgb = (v + (n*0.31)) * f * m;
   }
   else
   {
    finalColor = texture2D(s_Image, inUV);
   }
  outColor.rgb = finalColor.rgb;
  outColor.a = 1.0;
}			