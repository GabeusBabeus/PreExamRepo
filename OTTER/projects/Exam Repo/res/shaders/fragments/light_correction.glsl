// We need the flags from the frame uniforms
#pragma once
#include "frame_uniforms.glsl"

vec3 SpecularCorrect(vec3 specular){
    if (IsFlagSet(FLAG_ENABLE_SPECULAR))
    {
        return specular;
    }
    else
        return vec3(0.0);
}
vec3 AmbientCorrect(vec3 specular){
    if (IsFlagSet(FLAG_ENABLE_AMBIENT))
    {
        return specular;
    }
    else
        return vec3(0.0);
}
vec3 DiffuseCorrect(vec3 diffuse){
    if (IsFlagSet(FLAG_ENABLE_DIFFUSE))
    {
        return diffuse;
    }
    else
        return vec3(0.0);
}
