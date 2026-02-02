#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/fwd.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <util/glm_double.h>
#include <iomanip>


struct Transform {
    union {
        glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 position;
    };

    glm::vec3 scale = glm::vec3(1.0f);
    glm::quat rotation = glm::quat(1, 0, 0, 0);

    Transform() = default;

    Transform(const float x, const float y, float z) : translation(x, y, z) {}

    Transform(const glm::vec3 translation, const glm::vec3 scale = glm::vec3(1), const glm::vec3 r = glm::vec3(0))
        : translation(translation), scale(scale) {
        setRotation(r.x, r.y, r.z);
    }

    Transform(const glm::vec3 translation, const glm::vec3 scale, const glm::quat rotation)
        : translation(translation), scale(scale), rotation(rotation) {}

    void setRotation(const float pitch, const float yaw, const float roll) {
        const glm::quat qPitch = glm::angleAxis(glm::radians(pitch), glm::vec3(1,0,0));
        const glm::quat qYaw   = glm::angleAxis(glm::radians(yaw),   glm::vec3(0,1,0));
        const glm::quat qRoll  = glm::angleAxis(glm::radians(roll),  glm::vec3(0,0,1));
        rotation = qYaw * qPitch * qRoll;
    }

    void setYaw(const float yaw) {
        const glm::vec3 euler = glm::eulerAngles(rotation);
        const float pitch = glm::degrees(euler.x);
        const float roll = glm::degrees(euler.z);

        const glm::quat qPitch = glm::angleAxis(glm::radians(pitch), glm::vec3(1,0,0));
        const glm::quat qYaw   = glm::angleAxis(glm::radians(yaw), glm::vec3(0,1,0));
        const glm::quat qRoll  = glm::angleAxis(glm::radians(roll), glm::vec3(0,0,1));

        rotation = qYaw * qPitch * qRoll;
    }

    void setRotation(const glm::quat rotation) {
        this->rotation = rotation;
    }

    void addYaw(const float yaw) {
        const glm::quat yawRotation = glm::angleAxis(glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = yawRotation * rotation;
    }

    void addPitch(const float pitch) {
        const glm::quat pitchRotation = glm::angleAxis(glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
        rotation = pitchRotation * rotation;
    }

    void addRoll(const float roll) {
        const glm::quat rollRotation = glm::angleAxis(glm::radians(roll), glm::vec3(0.0f, 0.0f, 1.0f));
        rotation = rollRotation * rotation;
    }

    [[nodiscard]] glm::mat4 createModel3D() const {
        auto model = glm::mat4(1.0f);
        model = glm::translate(model, translation);
        model *= glm::mat4_cast(rotation);
        model = glm::scale(model, scale);
        return model;
    }

    [[nodiscard]] glm::mat4 createModel3DSkew(const glm::vec3 skew) const {
        const glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
        const glm::mat4 R = glm::mat4_cast(rotation);
        const glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

        glm::mat4 skewMat(1.0f);

        skewMat[1][0] = skew.x; // XY shear factor: second row, first column
        skewMat[2][0] = skew.y; // XZ shear factor: third row, first column
        skewMat[2][1] = skew.z; // YZ shear factor: third row, second column

        return T * R * skewMat * S;
    }

    friend std::ostream& operator<<(std::ostream& os, const Transform& t) {
        os << std::fixed << std::setprecision(3);
        os << "Transform {\n"
           << "  Translation: (" << t.translation.x << ", " << t.translation.y << ", " << t.translation.z << ")\n"
           << "  Scale:       (" << t.scale.x << ", " << t.scale.y << ", " << t.scale.z << ")\n"
           << "  Rotation: (" << t.rotation.x << ", " << t.rotation.y << ", "
                                    << t.rotation.z << ", " << t.rotation.w << ")\n"
           << "}";
        return os;
    }

};

inline Transform operator*(const Transform& parent, const Transform& child) {
    Transform out;
    out.scale = parent.scale * child.scale;
    out.rotation = parent.rotation * child.rotation;
    out.translation = parent.translation + (parent.rotation * (parent.scale * child.translation));
    return out;
}

inline Transform& operator*=(Transform& self, const Transform& rhs) {
    self = self * rhs;
    return self;
}