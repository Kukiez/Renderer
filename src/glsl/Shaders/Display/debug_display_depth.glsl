#version 460 core
#include "../common/global_transforms.glsl"

in vec2 v_TexCoords;

layout(std430, binding = 0) buffer Camera {
    CameraData camera;
};

out vec4 FColor;

uniform sampler2D depthTex;

void main() {
    vec4 color = texture(depthTex, v_TexCoords).rgba;
//    float lz = LinearizeDepth(z, camera.nearClip, camera.farClip);
//
//    lz /= camera.farClip;

    FColor = vec4(color.x, color.x, color.x, 1);
}