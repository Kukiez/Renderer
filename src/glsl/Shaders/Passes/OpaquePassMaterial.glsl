#include "surface.glsl"
#include "../common/global_transforms.glsl"
#include "../Lighting/light.glsl"
#include "../Lighting/pbr.glsl"
#include "PoissonSamples.glsl"
#line 8

out vec4 FragColor;

struct GPUBiomeAtmosphere {
    vec3 rayleighScattering;
    float rayleighAbsorption;

    vec3 mieScattering;
    float mieAbsorption;

    vec3 ozoneAbsorption;
    float ozoneThickness;

    float fogDensity;
    float fogHeightFollow;
    float fogBaseHeight;
    float fogMaxOpacity;

    float aerialDistanceScale;
    float aerialBrightnessScale;
    float aerialColorShift;
    float pad0;

    vec3 sunDirectionalColor;
    float sunIntensity;

    vec3 skyTint;
    float skyIntensity;

    vec3 biomeColorShift;
    float biomeHazeDensity;

    vec3 biomeAmbient;
    float biomeAmbientIntensity;

    float volumetricFogShadowAmount;
    float volumetricFogScattering;
    float volumetricFogAnisotropy;

    GPUProbe skybox;
};

struct LightPass {
    int numPointLights;
    uint brdfLUT;
    uint shadowNoise;
};

layout(std430, binding = 49) buffer LightPassBuffer {
    LightPass Pass;
};

layout(std430, binding = 50) buffer PointLightsBuffer {
    PointLight PointLights[];
};

layout(std430, binding = 51) buffer LightIndicesBuffer {
    GPULightIndices LightIndices[];
};

layout(std430, binding = 53) buffer LightHeaderBuffer {
    LightClusterHeader LightHeader;
};

layout(std430, binding = 54) buffer IBLProbeHeader {
    GPUProbeHeader probeHeader;
};

layout(std430, binding = 55) buffer IBLProbes {
    GPUProbe probes[];
};

layout(std430, binding = 57) buffer DirectionalLights {
    DirectionalLight directionalLights[];
};

layout(std430, binding = 58) buffer GPUCamera {
    CameraData camera;
};

layout(std430, binding = 59) buffer _BindlessTextures2D {
    uint64_t _textures[];
};

layout(std430, binding = 60) buffer Atmosphere {
    GPUBiomeAtmosphere atmosphere;
};

sampler2D GetSampler(uint id) {
    return sampler2D(_textures[id]);
}

vec3 ComputeIBL(Pixel pixel, uint irradianceMapIdx, uint prefilterMapIdx, uint maxLod) {
    vec3 V = pixel.viewDir;
    vec3 N = pixel.normal;
    vec3 R = reflect(-V, N);
    vec3 F0 = pixel.F0;

    float roughness = pixel.roughness;
    float metallic = pixel.metallic;
    vec3 albedo = pixel.albedo;

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(samplerCube(_textures[irradianceMapIdx]), N).rgb;
    vec3 diffuse      = irradiance * albedo;

    vec3 prefilteredColor = textureLod(samplerCube(_textures[prefilterMapIdx]), R, roughness * float(maxLod)).rgb;
    vec2 brdf  = texture(GetSampler(Pass.brdfLUT), vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * pixel.ao;
    vec3 color = ambient;

    return color * 0.3f;
}

const int NUM_SAMPLES = 64;

float ShadowPCF(Pixel pixel, sampler2D shadowMap, vec3 shadowCoord, float filterRadiusUV) {
    float receiverDepth = shadowCoord.z;
    float filterRadius = filterRadiusUV;

    ivec2 noiseCoord = ivec2(floor(pixel.worldPos.xz * 1)) & 63;

    vec2 noise = texelFetch(GetSampler(Pass.shadowNoise), noiseCoord, 0).rg;

    float angle = atan(noise.y, noise.x);
    float s = sin(angle);
    float c = cos(angle);

    mat2 rot = mat2(c, -s, s,  c);

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    float shadow = 0.0;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        vec2 offset = (rot * poisson64[i] * filterRadius);

        float sampleDepth = texture(shadowMap, shadowCoord.xy + offset).r;

        shadow += sampleDepth >= receiverDepth ? 1 : 0;
    }

    shadow /= NUM_SAMPLES;

    return shadow;
}

float SampleCascade(Pixel pixel, DirectionalLight light, int cascade) {
    vec4 fragPosLightSpace = light.cascades[cascade].view * vec4(pixel.worldPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;

    if (currentDepth >= 1.0) {
        return 0.0;
    }

    uint cascadeTexture = light.cascades[cascade].texture;


    float ndotl = max(dot(pixel.normal, light.direction.xyz), 0.0);

    float constantBias = 0.0005f;
    float slopeBias = 0.005f;

    float bias =
    max(
    constantBias,
    slopeBias * (1.0 - ndotl)
    );

    float worldTexelSize = light.cascades[cascade].worldTexelSize;
    float filterRadiusUV = light.cascades[cascade].worldFilterRadiusUV;

    float shadow = ShadowPCF(pixel, GetSampler(cascadeTexture), vec3(projCoords.xy, projCoords.z - bias), filterRadiusUV);
    return shadow;
}

float getPixelDistance(Pixel pixel, ShadowCascade shadow) {
    vec3 d = abs(pixel.worldPos.xyz - vec3(0));

    return max(max(d.x, d.y), d.z);
}

float ShadowCalculation(Pixel pixel, DirectionalLight light) {
    vec4 fragPosViewSpace = camera.view * vec4(pixel.worldPos, 1.0);
    float depthValue = -fragPosViewSpace.z;

    int layer = 0;

    vec3 clipmapOrigin = light.worldOrigin.xyz;

    float pixelDistance = 0;

    for (int i = 0; i < 4; ++i) {
        pixelDistance = getPixelDistance(pixel, light.cascades[i]);
        if (pixelDistance <= light.cascades[i].distance) {
            layer = i;
            break;
        }
    }

    layer = 0;
    if (layer == -1) {
        return 0;
    }

    float shadow = SampleCascade(pixel, light, layer);

    return 1 - shadow;
}

uint GetTileIndex(vec3 fragViewPos) {
    float zNear = camera.nearClip;
    float zFar = camera.farClip;

    ivec3 gridSize = LightHeader.clusterCount;
    uint zTile = uint((log(abs(fragViewPos.z) / zNear) * gridSize.z) / log(zFar / zNear));
    vec2 tileSize = vec2(camera.width, camera.height) / gridSize.xy;
    uvec3 tile = uvec3(gl_FragCoord.xy / tileSize, zTile);
    uint tileIndex = tile.x + (tile.y * gridSize.x) + (tile.z * gridSize.x * gridSize.y);

    return tileIndex;
}

GPULightIndices GetLightsForFragment(vec3 fragViewPos) {
    return LightIndices[GetTileIndex(fragViewPos)];
}

void pass_main(Surface surface) {
    Pixel pixel;
    pixel.worldPos = surface.worldPos;
    pixel.camPos = camera.position;
    pixel.viewDir = normalize(camera.position - surface.worldPos);
    pixel.albedo = surface.albedo;
    pixel.emissive = surface.emissive;
    pixel.uv = surface.uv;
    pixel.roughness = surface.roughness;
    pixel.metallic = surface.metallic;
    pixel.ao = surface.ao;
    pixel.normal = normalize(surface.normal);
    pixel.F0 = mix(vec3(0.04), pixel.albedo, pixel.metallic);

    vec3 Lo = vec3(0);

    vec3 fragViewPos = vec3(camera.view * vec4(surface.worldPos, 1));

    GPULightIndices lights = GetLightsForFragment(fragViewPos);

    // POINT LIGHT
    for (int i = 0; i < LIGHTS_PER_STRUCT; ++i) {
        if (lights.lightIndices[i] == 0) {
            break;
        }
        PointLight pointLight = PointLights[lights.lightIndices[i] - 1];

        Light light = CreateLight(pointLight, pixel.worldPos);

        Lo += EvaluateLightingPBR(pixel, light, 0);
    }


    /// DIRECTIONAL LIGHT
    for (int i = 0; i < 1; ++i) {
        DirectionalLight dirLight = directionalLights[i];

        float shadow = ShadowCalculation(pixel, dirLight);

        Light light = CreateLight(dirLight);

        Lo += EvaluateLightingPBR(pixel, light, 0);
    }

    // IBL AMBIENT
    GPUProbe skybox = atmosphere.skybox;
    vec3 ambient = ComputeIBL(pixel, skybox.irradianceMap, skybox.prefilterMap, skybox.maxReflectionLOD);

    // FINAL
    FragColor = vec4(Lo + ambient, 1);
}