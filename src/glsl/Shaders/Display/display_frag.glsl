#version 460 core
#include "tonemap.glsl"

layout(location = 0) out vec4 FragColor;

in vec2 v_TexCoords;

uniform sampler2D scene;
uniform sampler2D bloom;

uniform sampler2D ssao;
uniform bool isSSAOEnabled;

uniform float bloomStrength;
uniform bool isBloomEnabled;

uniform bool isTonemapEnabled;
uniform float tonemapExposure;
uniform int tonemapMethod;

void main()
{
    vec4 color = texture(scene, v_TexCoords).rgba;

    if (false) {
        float occlusion = texture(ssao, v_TexCoords).r;
        color *= 1 - occlusion;
    }

    if (isBloomEnabled) {
        vec3 bloomColor = texture(bloom, v_TexCoords).rgb;

        FragColor.rgb = color.rgb + bloomColor * bloomStrength;
    } else {
        FragColor = color;
    }

    if (isTonemapEnabled) {
        FragColor.xyz = Tonemap(FragColor.xyz, tonemapMethod, tonemapExposure);
    }
}