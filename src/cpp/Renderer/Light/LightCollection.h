#pragma once


class DirectionalLightCollection : public IPrimitiveCollection {
    glm::vec3 direction{};
    glm::vec3 color{};
    float intensity{};

    std::array<TextureKey, 4> shadowMaps{};

    unsigned index{};
public:
    DirectionalLightCollection() = default;

    DirectionalLightCollection(const glm::vec3& direction, const glm::vec3& color, float intensity, std::array<TextureKey, 4> shadowMaps, unsigned index)
        : direction(direction), color(color), intensity(intensity), shadowMaps(shadowMaps), index(index) {}


    glm::vec3 getDirection() const { return direction; }

    void setDirection(glm::vec3 direction) { this->direction = direction; }

    float getIntensity() const { return intensity; }

    std::array<TextureKey, 4> getShadowMaps() const { return shadowMaps; }

    glm::vec3 getColor() const { return color; }

    unsigned getIndex() const { return index; }
};

class PointLightCollection : public IPrimitiveCollection {
    glm::vec3 color{};
    float radius{};
    float intensity{};
    TextureKey shadowMap{};
    int shadowMapResolution{};
public:
    PointLightCollection() = default;
    PointLightCollection(const glm::vec3& color, float radius, float intensity) : color(color), radius(radius), intensity(intensity) {}
    PointLightCollection(glm::vec3 color, float radius, float intensity, TextureKey shadowMap, int shadowMapResolution)
    : color(color), radius(radius), intensity(intensity), shadowMap(shadowMap), shadowMapResolution(shadowMapResolution) {}

    glm::vec3 getColor() const { return color; }
    float getRadius() const { return radius; }
    float getIntensity() const { return intensity; }

    TextureKey getShadowMap() const { return shadowMap; }
    int getShadowMapResolution() const { return shadowMapResolution; }
};