#pragma once

#include <utility>

#include "FCurve.h"
#include "Mesh/MeshAPI.h"

struct AnimationChannel {
    unsigned mesh;
    FCurve3 position;
    FCurve3 scale;
    QCurve rotation;
};


class ModelAnimation {
    AnimationChannel& findChannel(const unsigned mesh);

    std::string name;
    std::vector<AnimationChannel> channels;
    double duration = 0;
public:
    ModelAnimation() = default;
    explicit ModelAnimation(std::string animationName) : name(std::move(animationName)) {}

    MESHAPI void addRotation(const unsigned mesh, double time, float pitch, float yaw, float roll);
    MESHAPI void addRotation(const unsigned mesh, double time, glm::quat rot);
    MESHAPI void addTranslation(const unsigned mesh, double time, glm::vec3 position);
    MESHAPI void addScale(const unsigned mesh, double time, glm::vec3 scale);

    void clear() {
        channels.clear();
    }

    const std::vector<AnimationChannel>& getChannels() const {
        return channels;
    }

    double getDuration() const {
        return duration;
    }

    const std::string& getName() const {
        return name;
    }
};