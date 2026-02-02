#version 460 core
#include "../common/global_transforms.glsl"
#line 4

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

struct UITextMaterial {
    vec2 position;
    vec2 size;
    vec2 uvOffset;
    vec2 uvScale;
    vec4 color;
    uint texture;
    int mode;
    float borderThickness;
    uint font;
    vec4 charUV;
};

layout (std430, binding = 0) buffer Camera {
    CameraData camera;
};

layout(std430, binding = 1) buffer UIInstances {
    UITextMaterial instances[];
};

out vec2 TexCoords;
out flat int instanceID;

out vec2 v_LocalPosition;

void main() {
    int instID = gl_BaseInstance + gl_InstanceID;

    UITextMaterial instance = instances[instID];

    vec2 worldPos = instance.position + aPos * instance.size;

    gl_Position = camera.ortho * vec4(worldPos, 0.0, 1.0);

    TexCoords = aTexCoords;

    instanceID = instID;

    v_LocalPosition = aPos;
}