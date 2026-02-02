#version 460 core
#include "../common/global_transforms.glsl"
#line 4

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

struct UIRectangleMaterial {
    vec2 position;
    vec2 size;
    vec2 uvMin;
    vec2 uvMax;
    vec4 color;
    uint texture;
    int mode;
    float borderThickness;
    int padding;
};

layout (std430, binding = 0) buffer Camera {
    CameraData camera;
};

layout(std430, binding = 1) buffer UIInstances {
    UIRectangleMaterial instances[];
};

out vec2 TexCoords;
out flat int instanceID;

out vec2 v_LocalPosition;

void main() {
    int instID = gl_BaseInstance + gl_InstanceID;

    UIRectangleMaterial instance = instances[instID];

    vec2 worldPos = instance.position + aPos * instance.size;
    gl_Position = camera.ortho * vec4(worldPos, 0.0, 1.0);

    vec2 uv = mix(instance.uvMax, instance.uvMin, aTexCoords);

    TexCoords = uv;

    instanceID = instID;

    v_LocalPosition = aPos;
}