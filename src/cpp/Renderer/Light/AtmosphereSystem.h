#pragma once
#include <Renderer/Skybox/SkyboxSystem.h>

struct GPUBiomeAtmosphere {
    glm::vec3 rayleighScattering;
    float rayleighAbsorption;

    glm::vec3 mieScattering;
    float mieAbsorption;

    glm::vec3 ozoneAbsorption;
    float ozoneThickness;


    float fogDensity;
    float fogHeightFollow;
    float fogBaseHeight;
    float fogMaxOpacity;


    float aerialDistanceScale;
    float aerialBrightnessScale;
    float aerialColorShift;
    float pad0;


    glm::vec3 sunDirectionalColor;
    float sunIntensity;

    glm::vec3 skyTint;
    float skyIntensity;


    glm::vec3 biomeColorShift;
    float biomeHazeDensity;

    glm::vec3 biomeAmbient;
    float biomeAmbientIntensity;

    float volumetricFogShadowAmount;
    float volumetricFogScattering;
    float volumetricFogAnisotropy;

    GPUSkybox skybox;
};