#version 460 core

#include "brdf.glsl"

out vec4 FragColor;
in vec2 v_TexCoords;

void main() {
    vec2 integratedBRDF = IntegrateBRDF(v_TexCoords.x, v_TexCoords.y);
    FragColor = vec4(integratedBRDF, 0, 1);
}