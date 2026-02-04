#version 460 core
#include "../common/global_transforms.glsl"
#line 5

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec2 TexCoords;
out vec3 Normal;
out vec3 WorldPos;
out vec3 Tangent;
out vec3 Bitangent;
out flat int meshStaticMaterialIndex;

struct MeshTransform {
    mat4 transform;
};

struct MeshInstance {
    int transformIndex;
    int materialIndex;
    int staticMaterialIndex;
};

layout(std430, binding = 0) buffer Camera {
    CameraData aCamera;
};

layout(std430, binding = 1) buffer MeshTransforms {
    MeshTransform meshTransforms[];
};

layout(std430, binding = 2) buffer MeshInstances {
    MeshInstance meshInstances[];
};

out flat int instanceID;

void main() {
    instanceID = gl_BaseInstance + gl_InstanceID;

    MeshInstance instance = meshInstances[instanceID];

    mat4 meshTransform = meshTransforms[instance.transformIndex].transform;

    WorldPos = vec3(meshTransform * vec4(aPos, 1.0));
    Normal = normalize(mat3(meshTransform) * aNormal);

    TexCoords = aTexCoords;
    Tangent = aTangent;
    Bitangent = aBitangent;
    meshStaticMaterialIndex = instance.staticMaterialIndex;

    gl_Position = aCamera.projection * aCamera.view * vec4(WorldPos, 1.0);
}