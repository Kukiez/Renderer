#version 460 core
#include "../common/global_transforms.glsl"
#line 5

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIds;
layout (location = 6) in vec4 aBoneWeights;

out vec2 TexCoords;
out vec3 Normal;
out vec3 WorldPos;
out vec3 Tangent;
out vec3 Bitangent;
out flat int meshStaticMaterialIndex;

struct SkinnedMeshTransform {
    mat4 transform;
};

struct SkinnedMeshInstance {
    int boneIndex;
    int materialIndex;
    int staticMaterialIndex;
};

layout(std430, binding = 0) buffer Camera {
    CameraData aCamera;
};

layout(std430, binding = 1) buffer BoneTransforms {
    mat4 boneMatrices[];
};

layout(std430, binding = 2) buffer SkinnedMeshInstances {
    SkinnedMeshInstance meshInstances[];
};

out flat int instanceID;

void main() {
    instanceID = gl_BaseInstance + gl_InstanceID;

    SkinnedMeshInstance instance = meshInstances[instanceID];


    int base = instance.boneIndex;

    vec4 skinned =
        (boneMatrices[base + aBoneIds[0]] * vec4(aPos, 1)) * aBoneWeights[0] +
        (boneMatrices[base + aBoneIds[1]] * vec4(aPos, 1)) * aBoneWeights[1] +
        (boneMatrices[base + aBoneIds[2]] * vec4(aPos, 1)) * aBoneWeights[2] +
        (boneMatrices[base + aBoneIds[3]] * vec4(aPos, 1)) * aBoneWeights[3];

    Normal = aNormal;
    TexCoords = aTexCoords;
    Tangent = aTangent;
    Bitangent = aBitangent;
    WorldPos = skinned.xyz;
    meshStaticMaterialIndex = instance.staticMaterialIndex;

    gl_Position = aCamera.projection * aCamera.view * skinned;
}