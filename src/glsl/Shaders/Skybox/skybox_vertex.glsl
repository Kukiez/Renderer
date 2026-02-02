#version 460 core
#include "../common/global_transforms.glsl"

layout (location = 0) in vec3 aPos;

out vec3 v_WorldPos;

layout(std430, binding = 0) buffer Camera {
    CameraData camera;
};

void main()
{
    v_WorldPos = aPos;
    vec4 pos = camera.projection * mat4(mat3(camera.view)) * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}