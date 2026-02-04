#include "Ray.h"
#include "AABB.h"
#include <glm/glm.hpp>

RayResult Ray::intersects(const glm::vec3 &center, float radius) const {
    glm::vec3 oc = origin - center;

    float a = glm::dot(direction, direction);
    float b = 2.0f * glm::dot(oc, direction);
    const float c = glm::dot(oc, oc) - radius * radius;
    const float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0.0f) {
        return {};
    }

    const float sqrtD = sqrt(discriminant);
    const float t0 = (-b - sqrtD) / (2.0f * a);
    const float t1 = (-b + sqrtD) / (2.0f * a);

    const float tHit = (t0 >= 0.0f) ? t0 : ((t1 >= 0.0f) ? t1 : -1.0f);
    if (tHit < 0.0f || tHit > length) {
        return {};
    }

    const glm::vec3 pos = origin + direction * tHit;
    return {
        .hitPos = pos,
        .normal = glm::normalize(pos - center),
        .distance = tHit
    };
}

RayResult Ray::intersects(const AABB& aabb) const {
    const glm::vec3 min = aabb.min();
    const glm::vec3 max = aabb.max();

    glm::vec3 invDir = 1.0f / direction;
    glm::vec3 tMin = (min - origin) * invDir;
    glm::vec3 tMax = (max - origin) * invDir;

    float tNear = 0.0f;
    float tFar  = length;
    int hitAxis = -1;

    for (int i = 0; i < 3; ++i) {
        if (direction[i] == 0.0f) {
            if (origin[i] < min[i] || origin[i] > max[i])
                return {};
        } else {
            float invD = 1.0f / direction[i];
            float t0 = (min[i] - origin[i]) * invD;
            float t1 = (max[i] - origin[i]) * invD;
            if (t0 > t1) std::swap(t0, t1);

            if (t0 > tNear) {
                tNear = t0;
                hitAxis = i;
            }

            tFar = std::min(tFar, t1);

            if (tNear > tFar)
                return {};
        }
    }
    float hitDist = glm::max(tNear, 0.0f);

    glm::vec3 hitPoint = origin + direction * hitDist;
    glm::vec3 normal(0.0f);

    if (hitAxis != -1) {
        normal[hitAxis] = (direction[hitAxis] > 0.0f) ? -1.0f : 1.0f;
    }

    return { hitPoint, normal, hitDist};
}

bool Ray::intersectsSlab(const AABB &aabb) const {
    glm::vec3 invDir = 1.0f / direction;
    glm::vec3 t0 = (aabb.min() - origin) * invDir;
    glm::vec3 t1 = (aabb.max() - origin) * invDir;

    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);

    float tEnter = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
    float tExit  = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

    return tExit >= tEnter && tExit >= 0.0f;
}
