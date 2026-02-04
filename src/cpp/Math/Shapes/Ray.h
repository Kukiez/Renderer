#pragma once
#include "../MathAPI.h"
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

struct AABB;

struct RayResult {
    glm::vec3 hitPos = glm::vec3(0);
    glm::vec3 normal = glm::vec3(0);
    float distance = -1.0f;

    [[nodiscard]] bool hasHit() const {
        return distance >= 0.0f;
    }
};

class MATHAPI Ray {
public:
    glm::vec3 origin = glm::vec3(0);
    glm::vec3 direction = glm::vec3(0);
    float length;

    Ray() = default;
    Ray(const glm::vec3 origin, const glm::vec3 direction, const float length) : origin(origin), direction(direction), length(length) {}

    static Ray cast(const glm::vec3 from, const glm::vec3 to) {
        const glm::vec3 dir = normalize(to - from);
        const float len = distance(from, to);
        return Ray(from, dir, len);
    }

    RayResult intersects(const glm::vec3& center, float radius) const;
    RayResult intersects(const AABB& aabb) const;

    bool intersectsSlab(const AABB& aabb) const;

    glm::vec3 getPoint(const float time) const {
        return origin + direction * time;
    }
};
