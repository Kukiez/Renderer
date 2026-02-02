#version 460 core

in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D ssao;

void main() {
    float occlusion = texture(ssao, v_TexCoords).r;
    FragColor = vec4(vec3(occlusion), 1.0); // visualize as grayscale
}