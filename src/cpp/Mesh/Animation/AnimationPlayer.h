#pragma once
#include "FCurve.h"
#include "Renderer/Common/Interpolation.h"
#include <Mesh/Model.h>
#include "ModelAnimator.h"
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_exponential.hpp>

enum class AnimationCancel {
    KEEP_FADEOUT = 1,
    SKIP_TO_END = 2,
    IMMEDIATE = 4,
    FADE_TO_IDENTITY = 8
};

struct AnimationStartInfo {
    std::string animationByString;
    Entity model;
    FadeIn fadein;
    FadeOut fadeout;
    double speed;
    double weight;
    BlendMode blendMode;
    bool repeating;
    bool overrideExisting;

    AnimationStartInfo() = default;

    AnimationStartInfo(const std::string& anim, const Entity& model,
        const BlendMode blend = BlendMode::OVERRIDE, const FadeIn fadein = FadeIn(),
        const FadeOut fadeout = FadeOut(), const bool repeating = false,
        const double speed = 1.0, const bool overrideExisting = false, const double weight = 1.0)
        : animationByString(anim), model(model), fadein(fadein), fadeout(fadeout), speed(speed),
    weight(weight), blendMode(blend), repeating(repeating), overrideExisting(overrideExisting) {}
};

struct AnimationPlayer {
    static void animate(Model& model, ModelAnimator& animator, TransformComponent* outFinalAnimatedTransforms, const double deltaTime) {
        auto& tracks = animator.getTracks();

        std::vector<std::vector<TransformComponent>> finalTransforms; // TODO local allocator
        std::vector<double> weights;

        std::array<std::vector<unsigned>, static_cast<int>(BlendMode::NUM_BLEND_MODES)> animationIndices;

        finalTransforms.reserve(tracks.size());
        weights.assign(tracks.size(), {});

        for (size_t i = 0; i < tracks.size();) {
            auto& animationResult = finalTransforms.emplace_back();
            animationResult.assign(model.getAsset()->getNodesCount(), {});
            std::memcpy(animationResult.data(), model.getAsset()->localNodeTransforms, sizeof(TransformComponent) * model.getAsset()->getNodesCount());

            auto& [keyframes, currTime, duration, speed,
                weight, blend, fadein,
                fadeout, repeating] = tracks[i];

            const double fadeoutDuration = fadeout.duration / speed;
            const double fadeinDuration = fadein.duration / speed;

            const double fadeOutBeginTime = duration - fadeoutDuration;

            double fadeInInfluence = 1;
            double fadeOutInfluence = 1;

            if (fadeinDuration > 0.0 && currTime < fadeinDuration) {
                const double t = currTime / fadeinDuration;
                fadeInInfluence = Interpolate::lerp(0.0, 1.0, t, fadein.easing);
            }

            if (!repeating && fadeoutDuration > 0.0 && currTime >= fadeOutBeginTime) {
                const double t = (currTime - fadeOutBeginTime) / fadeoutDuration;
                fadeOutInfluence = Interpolate::lerp(1.0, 0.0, t, fadeout.easing);
            }
            const double fadeInOutWeight = fadeInInfluence * fadeOutInfluence;
            const double resultWeight = weight * fadeInOutWeight;

            for (AnimationTrack::Keyframe & frame: keyframes) {
                TransformComponent transform;
                transform.translation = frame.position.at(currTime, frame.pHint);
                transform.scale = frame.scale.at(currTime, frame.sHint);
                transform.rotation = frame.rotation.at(currTime, frame.rHint);
                animationResult[frame.mesh] = transform;
            }

            weights[i] = resultWeight;

            animationIndices[static_cast<int>(blend)].emplace_back(i);

            if (currTime >= duration) {
                if (!repeating) {
                    tracks.erase(tracks.begin() + i);
                }
            } else {
                ++i;
            }
            currTime += deltaTime * speed;
            currTime = repeating ? std::fmod(currTime, duration) : std::fmin(currTime, duration);
        }

        auto out = outFinalAnimatedTransforms;
        size_t boneCount = model.getAsset()->getNodesCount();

        bool hasOverride = false;

        double totalWeight = 0;
        for (auto index : animationIndices[static_cast<int>(BlendMode::OVERRIDE)]) {
            double w = weights[index];
            if (w <= 0) continue;

            auto& pose = finalTransforms[index];

            if (!hasOverride) {
                for (size_t i = 0; i < boneCount; ++i)
                    out[i] = pose[i];
                totalWeight = w;
                hasOverride = true;
            } else {
                double t = w / (totalWeight + w);
                for (size_t i = 0; i < boneCount; ++i) {
                    out[i].position = Interpolate::lerp(out[i].position, pose[i].position, t);
                    out[i].rotation = Interpolate::slerp(out[i].rotation, pose[i].rotation, t);
                    out[i].scale    = Interpolate::lerp(out[i].scale,pose[i].scale, t);
                }
                totalWeight += w;
            }
        }

        for (auto index : animationIndices[static_cast<int>(BlendMode::ADDITIVE)]) {
            double w = weights[index];

            auto& pose = finalTransforms[index];

            for (size_t i = 0; i < boneCount; ++i) {
                out[i].position = out[i].position + pose[i].position * w;
                out[i].rotation = out[i].rotation * glm::slerp(glm::quat(1, 0, 0, 0), pose[i].rotation, static_cast<float>(w));
                out[i].scale = out[i].scale + Interpolate::lerp(glm::vec3(1), pose[i].position, w);
            }
        }

        for (auto index : animationIndices[static_cast<int>(BlendMode::MULTIPLY)]) {
            double w = weights[index];

            auto& pose = finalTransforms[index];

            for (size_t i = 0; i < boneCount; ++i) {
                glm::quat delta = glm::pow(pose[i].rotation, static_cast<float>(w));
                out[i].position = out[i].position * Interpolate::lerp(glm::vec3(1), pose[i].position, w);
                out[i].rotation = out[i].rotation * delta;
                out[i].scale = out[i].scale * Interpolate::lerp(glm::vec3(1), pose[i].position, w);
            }
        }
    }
};