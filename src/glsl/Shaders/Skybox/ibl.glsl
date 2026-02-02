
struct GPUProbe {
    uint envCubemap;
    uint irradianceMap;
    uint prefilterMap;
    uint maxReflectionLOD;
};

struct GPUProbeHeader {
    uint probeCount;
    uint brdfLUT;
    uint pad[2];
};


struct IBL {
    vec3 irradiance;
    vec3 prefilter;
    vec2 brdfLUT;
};

