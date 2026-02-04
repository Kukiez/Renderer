#pragma once

#include <glm/glm.hpp>
#include <ostream>
#include <glm/gtc/quaternion.hpp>

constexpr glm::vec2 operator + (const glm::vec2& lhs, const double scalar) {
    return glm::vec2(lhs.x + scalar, lhs.y + scalar);
}

constexpr glm::vec2 operator - (const glm::vec2& lhs, const double scalar) {
    return glm::vec2(lhs.x - scalar, lhs.y - scalar);
}

constexpr glm::vec2 operator * (const glm::vec2& lhs, const double scalar) {
    return glm::vec2(lhs.x * scalar, lhs.y * scalar);
}

constexpr glm::vec2 operator / (const glm::vec2& lhs, const double scalar) {
    return glm::vec2(lhs.x / scalar, lhs.y / scalar);
}

constexpr glm::vec3 operator + (const glm::vec3& lhs, const double scalar) {
    return glm::vec3(lhs.x + scalar, lhs.y + scalar, lhs.z + scalar);
}

constexpr glm::vec3 operator - (const glm::vec3& lhs, const double scalar) {
    return glm::vec3(lhs.x - scalar, lhs.y - scalar, lhs.z - scalar);
}

constexpr glm::vec3 operator * (const glm::vec3& lhs, const double scalar) {
    return glm::vec3(lhs.x * scalar, lhs.y * scalar, lhs.z * scalar);
}

constexpr glm::vec3 operator / (const glm::vec3& lhs, const double scalar) {
    return glm::vec3(lhs.x / scalar, lhs.y / scalar, lhs.z / scalar);
}

constexpr glm::vec4 operator * (const glm::vec4& lhs, const double scalar) {
    return glm::vec4(lhs * static_cast<float>(scalar));
}

constexpr glm::quat operator * (const glm::quat& quat, const double scalar) {
    return quat * static_cast<float>(scalar);
}

inline std::ostream &operator<<(std::ostream &os, const glm::vec2& vec) {
    os << vec.x << ", " << vec.y;
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const glm::vec3& vec) {
    os << vec.x << ", " << vec.y << ", " << vec.z;
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const glm::ivec3& vec) {
    os << vec.x << ", " << vec.y << ", " << vec.z;
    return os;
}

inline std::ostream& operator<<(std::ostream &os, const glm::vec4& vec) {
    os << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w;
    return os;
}

inline std::ostream& operator<<(std::ostream &os, const glm::mat4& mat) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            os << mat[i][j] << ", ";
        }
        os << "\n";
    }
    return os;
}

inline std::ostream& operator<<(std::ostream &os, const glm::quat& q) {
    os << q.x << ", " << q.y << ", " << q.z << ", " << q.w;
    return os;
}

struct IVec3Hash {
    std::size_t operator()(const glm::ivec3& v) const noexcept {
        size_t h1 = std::hash<int>()(v.x);
        size_t h2 = std::hash<int>()(v.y);
        size_t h3 = std::hash<int>()(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};