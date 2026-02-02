#pragma once
#include <glm/gtx/quaternion.hpp>

struct Capsule {
    glm::vec3 p1;
    glm::vec3 p2;
    float radius;

    Capsule(const glm::vec3 p1, const glm::vec3 p2, const float radius)
        : p1(p1), p2(p2), radius(radius) {}
};
