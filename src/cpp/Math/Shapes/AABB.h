#pragma once

#include <glm/glm.hpp>
#include <array>
#include <chrono>

struct AABB {
    static double timeSpentIntersecting;

    glm::vec3 center;
    glm::vec3 halfSize;

    AABB() : center(0), halfSize(0) {}

    AABB(const glm::vec3& center_, const glm::vec3& halfSize_)
        : center(center_), halfSize(halfSize_)
    {}

    [[nodiscard]] bool contains(const glm::vec3& point) const {
        return glm::all(glm::lessThanEqual(glm::abs(point - center), halfSize));
    }

    static AABB fromTo(const glm::vec3 min, const glm::vec3 max) {
        return AABB((min + max) / 2.0f, (max - min) / 2.0f);
    }
    
    [[nodiscard]] glm::vec3 min() const { return center - halfSize; }
    [[nodiscard]] glm::vec3 max() const { return center + halfSize; }

    [[nodiscard]] bool contains(const AABB& other) const {
        const glm::vec3 thisMin = min();
        const glm::vec3 thisMax = max();
        const glm::vec3 otherMin = other.min();
        const glm::vec3 otherMax = other.max();

        return (otherMin.x >= thisMin.x && otherMax.x <= thisMax.x &&
                otherMin.y >= thisMin.y && otherMax.y <= thisMax.y &&
                otherMin.z >= thisMin.z && otherMax.z <= thisMax.z);
    }

    [[nodiscard]] bool contains(const AABB& other, float epsilon) const {
        const glm::vec3 thisMin = min() - epsilon;
        const glm::vec3 thisMax = max() + epsilon;
        const glm::vec3 otherMin = other.min() + epsilon;
        const glm::vec3 otherMax = other.max() - epsilon;

        return (otherMin.x >= thisMin.x && otherMax.x <= thisMax.x &&
                otherMin.y >= thisMin.y && otherMax.y <= thisMax.y &&
                otherMin.z >= thisMin.z && otherMax.z <= thisMax.z);
    }

    [[nodiscard]] bool intersects(const AABB& other, float epsilon = 0.f) const {
        const glm::vec3 thisMin = min() - epsilon;
        const glm::vec3 thisMax = max() + epsilon;
        const glm::vec3 otherMin = other.min() + epsilon;
        const glm::vec3 otherMax = other.max() - epsilon;

        return !(otherMin.x > thisMax.x || otherMax.x < thisMin.x ||
                 otherMin.y > thisMax.y || otherMax.y < thisMin.y ||
                 otherMin.z > thisMax.z || otherMax.z < thisMin.z);
    }


    [[nodiscard]] auto corners() const {
        return std::array{
            center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z),
            center + glm::vec3( halfSize.x, -halfSize.y, -halfSize.z),
            center + glm::vec3(-halfSize.x,  halfSize.y, -halfSize.z),
            center + glm::vec3( halfSize.x,  halfSize.y, -halfSize.z),
            center + glm::vec3(-halfSize.x, -halfSize.y,  halfSize.z),
            center + glm::vec3( halfSize.x, -halfSize.y,  halfSize.z),
            center + glm::vec3(-halfSize.x,  halfSize.y,  halfSize.z),
            center + glm::vec3( halfSize.x,  halfSize.y,  halfSize.z)
        };
    }

    bool operator==(const AABB & aabb) const {
        return aabb.center == center && aabb.halfSize == halfSize;
    }

    friend std::ostream& operator<<(std::ostream& os, const AABB& aabb) {
        os << "[AABB] pos: " << aabb.center.x << ", " << aabb.center.y << ", " << aabb.center.z << ", size: " << aabb.halfSize.x << ", " << aabb.halfSize.y << ", " << aabb.halfSize.z << "]";
        return os;
    }

    glm::vec3 size() const { return halfSize * 2.0f; }

    AABB& moveBy(const glm::vec3 offset) {
        center += offset;
        return *this;
    }

    AABB& expandBy(const glm::vec3 halfExtents) {
        halfSize += halfExtents;
        return *this;
    }
};


