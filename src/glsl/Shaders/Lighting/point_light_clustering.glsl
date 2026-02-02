#version 460
#include "../lighting/light.glsl"
#include "../common/global_transforms.glsl"
#line 5

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer ClusterHeader {
    LightClusterHeader Header;
};

layout(std430, binding = 1) buffer PointLightsBuffer {
    PointLight PointLights[];
};

layout(std430, binding = 2) buffer CameraBuffer {
    CameraData Camera;
};

layout(std430, binding = 3) buffer OutPointLightIndices {
    GPULightIndices OutLightIndices[];
};

uniform int pointLightCount;

vec3 TransformScreenSpaceToViewSpace(vec2 screenXY) {
    vec2 screenDims = vec2(Camera.width, Camera.height);
    vec2 ndc = vec2(screenXY / screenDims * 2 - 1);

    vec4 clip = vec4(ndc, -1.0, 1.0);

    vec4 view = Camera.inverseProjection * clip;
    view.xyz /= view.w;

    return vec3(view);
}

vec3 lineIntersectionWithZPlane(vec3 startPoint, vec3 endPoint, float zDistance) {
    vec3 direction = endPoint - startPoint;
    vec3 normal = vec3(0.0, 0.0, -1.0);
    float t = (zDistance - dot(normal, startPoint)) / dot(normal, direction);
    return startPoint + t * direction;
}

void main() {
    ivec3 gridSize = Header.clusterCount;

    uint tileIndex = gl_WorkGroupID.x + (gl_WorkGroupID.y * gridSize.x) + (gl_WorkGroupID.z * gridSize.x * gridSize.y);

    vec2 tileSize = vec2(Camera.width, Camera.height) / gridSize.xy;

    vec2 tileMin = gl_WorkGroupID.xy * tileSize;
    vec2 tileMax = (gl_WorkGroupID.xy + 1) * tileSize;

    vec3 minTileView = TransformScreenSpaceToViewSpace(tileMin);
    vec3 maxTileView = TransformScreenSpaceToViewSpace(tileMax);

    float zNear = Camera.nearClip;
    float zFar = Camera.farClip;

    float planeNear = zNear * pow(zFar / zNear, gl_WorkGroupID.z / float(gridSize.z));
    float planeFar = zNear * pow(zFar / zNear, (gl_WorkGroupID.z + 1) / float(gridSize.z));

    vec3 minPointNear = lineIntersectionWithZPlane(vec3(0, 0, 0), minTileView, planeNear);
    vec3 minPointFar = lineIntersectionWithZPlane(vec3(0, 0, 0), minTileView, planeFar);
    vec3 maxPointNear = lineIntersectionWithZPlane(vec3(0, 0, 0), maxTileView, planeNear);
    vec3 maxPointFar = lineIntersectionWithZPlane(vec3(0, 0, 0), maxTileView, planeFar);

    vec3 minPoint = min(minPointNear, minPointFar);
    vec3 maxPoint = max(maxPointNear, maxPointFar);

    AABB clusterBounds = AABB_FromMinMax(minPoint, maxPoint);

    int nextLightIndex = 0;
    for (int i = 0; i < pointLightCount; ++i) {
        PointLight light = PointLights[i];

        float maxInfluence = sqrt(light.color.a / 0.001f);

        vec3 lightPosView = vec3(Camera.view * vec4(light.position.xyz, 1));
        float lightRadius = light.position.w;

        Sphere sphere = Sphere_Create(lightPosView, lightRadius);

        if (AABB_Intersects_Sphere(clusterBounds, sphere)) {
            OutLightIndices[tileIndex].lightIndices[nextLightIndex] = i + 1;
            ++nextLightIndex;
        }

        if (nextLightIndex == LIGHTS_PER_STRUCT -1) return;
    }
    OutLightIndices[tileIndex].lightIndices[nextLightIndex] = 0;
}