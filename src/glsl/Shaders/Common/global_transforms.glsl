
struct CameraData {
    mat4 view;
    mat4 projection;
    mat4 ortho;
    mat4 inverseView;
    mat4 inverseProjection;
    vec3 position;
    float pad;
    float nearClip;
    float farClip;
    float width;
    float height;
};

float LinearizeDepth(float depth, float zNear, float zFar) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * zNear * zFar) / (zFar + zNear - z * (zFar - zNear));
}

float LinearizeDepth(CameraData camera, float fragZ) {
    return LinearizeDepth(fragZ, camera.nearClip, camera.farClip);
}

vec3 ReconstructViewSpacePosition(CameraData camera, vec2 uv, float zDepth01) {
    float z = zDepth01 * 2.0 - 1.0;
    vec2 ndc = uv * 2.0 - 1.0;

    vec4 clip = vec4(ndc, z, 1.0);
    vec4 view = camera.inverseProjection * clip;
    return view.xyz / view.w;
}

#ifdef __FRAGMENT_SHADER__
vec3 ReconstructViewSpaceNormals(CameraData camera, vec2 uv, float depth) {
    vec3 P = ReconstructViewSpacePosition(camera, uv, depth);
    vec3 normalVS = normalize(cross(dFdx(P), dFdy(P)));

    if (normalVS.z > 0.0) normalVS = -normalVS;

    return normalVS;
}
#endif

float ReconstructViewSpaceZ(CameraData camera, vec2 uv, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 view = camera.inverseProjection * clip;
    return view.z / view.w;
}
