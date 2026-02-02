#pragma once
#include "ModelAnimation.h"
#include <math/Rotation.h>

AnimationChannel & ModelAnimation::findChannel(const unsigned mesh) {
    const auto it = std::ranges::find_if(channels,
                               [&](const AnimationChannel& c) { return c.mesh == mesh; });
    if (it == channels.end()) {
        channels.emplace_back(mesh);
        return channels.back();
    }
    return *it;
}

void ModelAnimation::addRotation(const unsigned mesh, double time, const float pitch, const float yaw,
                                 const float roll) {
    auto& kfs = findChannel(mesh);
    kfs.rotation.add(Rotation(pitch, yaw, roll).quat(), time);
    if (time > duration) {
        duration = time;
    }
}

void ModelAnimation::addRotation(const unsigned mesh, double time, const glm::quat rot) {
    auto& kfs = findChannel(mesh);
    kfs.rotation.add(rot, time);

    if (time > duration) {
        duration = time;
    }
}

void ModelAnimation::addTranslation(const unsigned mesh, double time, glm::vec3 position) {
    auto& kfs = findChannel(mesh);
    kfs.position.add(position, time);
    if (time > duration) {
        duration = time;
    }
}

void ModelAnimation::addScale(const unsigned mesh, double time, glm::vec3 scale) {
    auto& kfs = findChannel(mesh);
    kfs.scale.add(scale, time);

    if (time > duration) {
        duration = time;
    }
}