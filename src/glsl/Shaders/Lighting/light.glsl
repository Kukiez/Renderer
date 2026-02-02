#include "../aabb.glsl"
#include "../common/global_transforms.glsl"

const int LIGHT_TYPE_INVALID     = 0;
const int LIGHT_TYPE_POINT       = 1;
const int LIGHT_TYPE_DIRECTIONAL = 2;

struct PointLight {
    vec4 position;      // .xyz, .range
    vec4 color;         // .rgb, .intensity

    int shadowMapIndex; // -1 if none
    int shadowSoftness;
    float shadowBias;

    int _;
};

struct ShadowCascade {
    mat4 view;
    float worldTexelSize;
    float worldFilterRadiusUV;
    float distance;
    uint texture;
    float constantBias;
    float slopeBias;
    float pad2[2];
};

struct DirectionalLight {
    vec4 direction;
    vec4 color; //.w = intensity
    vec4 worldOrigin;
    ShadowCascade cascades[4];
    uint debugBits[4];
};

const int LIGHTS_PER_STRUCT = 4;

struct GPULightIndices {
    int lightIndices[LIGHTS_PER_STRUCT];
};

struct LightClusterHeader {
    ivec3 clusterCount;
};

struct GPUShadowMap {
    mat4 lightViewProj;
    ivec2 resolution;
    float farPlane;
    int shadowMap;
};