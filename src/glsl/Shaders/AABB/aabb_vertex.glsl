#version 460 core
#include "../../common/global_transforms.glsl"

layout(location = 0) in vec3 a_Position;

struct Instance {
    mat4 model;
    vec4 color;
};

layout(std430, binding = 0) buffer Camera {
    CameraData camera;
};

layout(std430, binding = 1) buffer Instances {
    Instance instances[];
};

out vec4 color;

void main() {
    gl_Position = camera.projection * camera.view * instances[gl_BaseInstance + gl_InstanceID].model * vec4(a_Position, 1.0);
    color = instances[gl_BaseInstance + gl_InstanceID].color;
}