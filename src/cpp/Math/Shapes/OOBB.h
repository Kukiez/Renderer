#pragma once
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "AABB.h"

struct TransformComponent;

struct OOBB {
    glm::vec3 center{};
    glm::vec3 halfSize{};
    glm::mat3 orientation{};

    OOBB() = default;

    OOBB(const glm::vec3& center_, const glm::vec3& halfSize_, const float yaw, const float pitch, const float roll)
        : center(center_), halfSize(halfSize_)
    {
        const glm::quat q = glm::yawPitchRoll(yaw, pitch, roll);
        orientation = mat3_cast(q);
    }

    OOBB(const glm::vec3& center, const glm::vec3& halfSize, const glm::mat3& ori)
    : center(center), halfSize(halfSize), orientation(ori) {}

    OOBB(const glm::vec3 center, const glm::vec3 halfSize, const glm::quat q)
        : center(center), halfSize(halfSize), orientation(glm::mat3_cast(q)) {}

    [[nodiscard]] std::array<glm::vec3, 8> getCorners() const;

    AABB getAABB() const {
        if (orientation == glm::mat3(1)) return AABB::fromTo(center - halfSize, center + halfSize);
        return AABB::fromTo(center - halfSize * orientation, center + halfSize * orientation);
    }
};