

struct mesh_pbr_material {
    vec4 albedoFactor;

    float metallicFactor;
    float roughnessFactor;

    uint diffuse;
    uint normal;

    uint metallicRoughness;
    uint occlusion;
    uint emissive;

    uint pad0;
};

in vec2 TexCoords;
in vec3 Normal;
in vec3 WorldPos;
in vec3 Tangent;
in vec3 Bitangent;

layout(std430, binding = 3) buffer MaterialData {
    uint64_t samplers[];
};

Surface material_main(mesh_pbr_material material) {
    vec2 uv = TexCoords;
    uv.y = 1 - uv.y;

    uint diffuseIdx = material.diffuse;
    uint emissiveIdx = material.emissive;
    uint normalIdx = material.normal;
    uint mrIdx = material.metallicRoughness;

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

    return surface;
}