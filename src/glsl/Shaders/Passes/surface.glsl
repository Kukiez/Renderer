
struct Surface {
    vec3 worldPos;
    vec3 albedo;
    vec3 emissive;
    vec2 uv;
    float roughness;
    float metallic;
    float ao;
    vec3 normal;
    float F0;
    float opacity;
    float depth;
    bool shouldDiscard;
};