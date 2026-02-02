#version 460 core

in vec3 v_WorldPos;
out vec4 FragColor;

uniform samplerCube skybox;

void main() {
    vec3 dir = normalize(v_WorldPos);
    vec3 color = textureLod(skybox, dir, 0).rgb;

    const float gamma = 2.2f;
    vec3 v = color.rgb / (1.f + color.rgb);
    v = pow(v, vec3(1.f/gamma));
    color.rgb = v;

    FragColor = vec4(color, 1.0);
}