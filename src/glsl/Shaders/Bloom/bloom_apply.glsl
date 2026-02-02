#version 460 core

in vec2 v_TexCoords;

out vec4 FragColor;

uniform sampler2D scene;
uniform sampler2D bloom;

uniform float strength;

void main() {
    vec3 hdrColor = texture(scene, v_TexCoords).rgb;
    vec3 bloomColor = texture(bloom, v_TexCoords).rgb;

    FragColor = vec4(mix(hdrColor, bloomColor, strength), 1);
}

