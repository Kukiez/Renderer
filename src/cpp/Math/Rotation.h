#pragma once

class Rotation {
    glm::quat rotation{};
    float yaw = 0, pitch = 0, roll = 0;
public:
    static Rotation identity() {
        return Rotation(glm::quat(1, 0, 0, 0));
    }

    Rotation() = default;
    Rotation(const glm::quat& rotation) : rotation(rotation) {}
    Rotation(const float pitch, const float yaw, const float roll) : yaw(yaw), pitch(pitch), roll(roll) {
        const glm::quat qPitch = glm::angleAxis(glm::radians(pitch), glm::vec3(1,0,0));
        const glm::quat qYaw   = glm::angleAxis(glm::radians(yaw),   glm::vec3(0,1,0));
        const glm::quat qRoll  = glm::angleAxis(glm::radians(roll),  glm::vec3(0,0,1));
        rotation = qYaw * qPitch * qRoll;
    }

    const glm::quat& quat() const { return rotation; }

    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }
    float getRoll() const { return roll; }
};

inline bool isNearZero(const float value, const float epsilon = 1e-6f) {
    return std::abs(value) < epsilon;
}

struct Quaternion : glm::quat {
    Quaternion() = default;

    Quaternion(const glm::quat& rotation) : glm::quat(rotation) {}

    static Quaternion identity() {
        return Quaternion(glm::quat(1, 0, 0, 0));
    }

    static Quaternion fromEuler(const glm::vec3& euler) {
        return Quaternion(glm::quat(glm::radians(euler)));
    }
};