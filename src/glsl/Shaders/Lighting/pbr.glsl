#include "light.glsl"
#include "ibl.glsl"

const float PI = 3.14f;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

struct Pixel {
    vec3 worldPos;
    vec3 camPos;
    vec3 normal;
    vec3 viewDir;
    vec2 uv;
    float roughness;
    float metallic;
    float ao;
    vec3 albedo;
    vec3 emissive;
    vec3 F0;
};

struct LightFalloff {
    float distance;
    float radius;
    float intensity;
    float falloffExponent;
    float customCurve;
};

float smoothFalloff(PointLight light, float distance) {
    float x = clamp(distance / light.position.w, 0.0, 1.0);
    return (1.0 - x * x) * (1.0 - x * x);
}

struct Light {
    vec3 radiance;
    vec3 L;
};

Light CreateLight(DirectionalLight dirLight) {
    vec3 direction = -dirLight.direction.xyz;
    vec3 L = normalize(direction);
    float intensity = dirLight.color.a;

    Light light;
    light.L = L;
    light.radiance = dirLight.color.rgb * 40.f;
    return light;
}

Light CreateLight(PointLight pLight, vec3 pixelWorldPos) {
    vec3 toLight = pLight.position.xyz - pixelWorldPos;

    vec3 L = normalize(toLight);
    float distance = length(toLight);
    float radius = pLight.position.w;

    float d2 = max(distance * distance, 0.01 * 0.01);
    float r2 = radius * radius;

    float attenuation = max(1.0 - d2 / r2, 0.0);
    attenuation *= attenuation;
    attenuation /= d2;

    Light light;
    light.L = L;
    light.radiance = pLight.color.rgb * attenuation * pLight.color.a;
    return light;
}


vec3 EvaluateLightingPBR(Pixel pixel, Light light, float shadow) {
    vec3 N = pixel.normal;
    vec3 V = pixel.viewDir;

    vec3 F0 = pixel.F0;

    vec3 L = light.L;
    vec3 H = normalize(V + L);
    vec3 radiance = light.radiance;

    float NDF = DistributionGGX(N, H, pixel.roughness);
    float G   = GeometrySmith(N, V, L, pixel.roughness);
    vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - pixel.metallic;

    float NdotL = max(dot(N, L), 0.0);

    vec3 lightContrib = ((kD * pixel.albedo / PI) + specular) * radiance * NdotL;
    vec3 Lo = (1.0 - shadow) * lightContrib;
    return Lo;
}