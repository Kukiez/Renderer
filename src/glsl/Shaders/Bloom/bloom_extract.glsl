#version 460 core

uniform sampler2D scene;
uniform float threshold; // e.g., 1.0 for HDR color

out vec4 fragColor;

in vec2 v_TexCoords;

void main() {

    vec3 color = texture(scene, v_TexCoords).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    fragColor = vec4((brightness > threshold) ? color : vec3(0.0), 1);
}