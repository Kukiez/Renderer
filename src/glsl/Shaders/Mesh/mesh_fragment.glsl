#version 460 core
#extension GL_ARB_gpu_shader_int64 : enable
#include "../Passes/OpaquePassMaterial.glsl"
#include "../common/global_transforms.glsl"
#line 7

in vec2 TexCoords;
in vec3 Normal;
in vec3 WorldPos;
in vec3 Tangent;
in vec3 Bitangent;

in flat int meshStaticMaterialIndex;

layout(std430, binding = 3) buffer MaterialData {
    uint64_t samplers[];
};

layout(std430, binding = 4) buffer MeshMaterialHeader {
    GPUPBRMaterial meshMaterialNodes[];
};

void main() {
    vec2 uv = TexCoords;
    uv.y = 1 - uv.y;

    GPUPBRMaterial staticMaterialData = meshMaterialNodes[meshStaticMaterialIndex];

    uint diffuseIdx = staticMaterialData.diffuse;
    uint emissiveIdx = staticMaterialData.emissive;
    uint normalIdx = staticMaterialData.normal;
    uint mrIdx = staticMaterialData.metallicRoughness;

    vec3 albedo = texture(sampler2D(samplers[diffuseIdx]), uv).xyz;
    vec3 emissive = texture(sampler2D(samplers[emissiveIdx]), uv).xyz;
    vec3 metallicRoughness = texture(sampler2D(samplers[mrIdx]), uv).xyz;
    vec3 tangentNormal = texture(sampler2D(samplers[normalIdx]), uv).xyz * 2.0 - 1.0;

    vec3 N = normalize(Normal);
    vec3 T = normalize(Tangent);

    T = normalize(T - N * dot(N, T));

    vec3 B = cross(N, T);

    mat3 TBN = mat3(T, B, N);
    vec3 worldNormal = normalize(TBN * tangentNormal);

    Surface surface;
    surface.albedo = albedo;
    surface.normal = worldNormal;
    surface.worldPos = WorldPos;
    surface.roughness = metallicRoughness.g;
    surface.metallic = metallicRoughness.b;
    surface.ao = 1;
    surface.uv = uv;
    surface.emissive = emissive;
    surface.depth = gl_FragCoord.z;
    surface.F0 = 0;
    surface.opacity = 1;

	pass_main(surface);
}