#version 460 core
#define __FRAGMENT_SHADER__
#include "../common/global_transforms.glsl"

out vec4 FragColor;

in vec2 v_TexCoords;

uniform sampler2D depthBuffer;
uniform sampler2D texNoise;

struct SSAOSettings {
    float radius;
    float bias;
    int kernelSize;
};

layout(std430, binding = 0) buffer Camera {
    CameraData camera;
};

layout(std430, binding = 1) buffer Samples {
    vec4 samples[];
};

uniform float radius;
uniform int kernelSize;
uniform float bias;

void main() {
    vec2 uv = v_TexCoords;
    float fragDepth = texture(depthBuffer, uv).x;

    vec2 noiseTexSize = textureSize(texNoise, 0);

    vec2 noiseScale = vec2(camera.width / noiseTexSize.x, camera.height / noiseTexSize.y);

    vec3 normal = ReconstructViewSpaceNormals(camera, uv, fragDepth);
    vec3 fragPos = ReconstructViewSpacePosition(camera, uv, fragDepth);

    vec3 randomVec = normalize(texture(texNoise, uv * noiseScale).xyz);

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i) {
        vec3 samplePos = TBN * samples[i].xyz;

        samplePos = fragPos + samplePos * radius;

        vec4 offset = vec4(samplePos, 1.0);
        offset = camera.projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepthNDC = texture(depthBuffer, offset.xy).r;
        float sampleDepth = ReconstructViewSpaceZ(camera, offset.xy, sampleDepthNDC);

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;

        float sampleZ = samplePos.z;
        float sceneZ  = sampleDepth;

        occlusion += (sceneZ >= sampleZ + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / kernelSize);

    FragColor = vec4(vec3(occlusion), 1);

    fragPos /= camera.farClip;
    FragColor = vec4(vec3(occlusion), 1);
}