#include "UIAnimationContext.h"

#include <algorithm>
#include <Renderer/Common/Interpolation.h>
#include <UI/UI.h>

void ui::UIWindowAnimationContext::advanceTransformAnimations(double deltaTime) {
    for (size_t i = 0; i < activeTransformAnimations.size(); ++i) {
        auto& [entity, animation, time, localSnapshot, isThisAnimActive] = activeTransformAnimations[i];

        auto& node = entity;

        if (!isThisAnimActive || animation.speed <= 0.0001f) continue;

        const double duration = animation.duration;
        double animNow = std::clamp(time + deltaTime * animation.speed, 0.0, duration);
        const double animSpeed = animation.speed;

        const double weight = animation.weight;

        const bool repeating = false;

        const double fadeoutDuration = animation.fadeout.duration / animSpeed;
        const double fadeinDuration = animation.fadein.duration / animSpeed;

        const double fadeOutBeginTime = duration - fadeoutDuration;

        double fadeInInfluence = 1;
        double fadeOutInfluence = 1;

        if (fadeinDuration > 0.0 && animNow < fadeinDuration) {
            const double t = animNow / fadeinDuration;
            fadeInInfluence = Interpolate::lerp(0.0, 1.0, t, animation.fadein.easing);
        }

        if (fadeoutDuration > 0.0 && animNow >= fadeOutBeginTime) {
            const double t = (animNow - fadeOutBeginTime) / fadeoutDuration;
            fadeOutInfluence = Interpolate::lerp(1.0, 0.0, t, animation.fadeout.easing);
        }
        const double fadeInOutWeight = fadeInInfluence * fadeOutInfluence;

        const double resultWeight = weight * fadeInOutWeight;

        if (animNow == duration) {
            if (repeating) {
                // TODO implement
            } else {
                animNow = duration;
                transformAnimationFreeIndices.emplace_back(i);

                isThisAnimActive = false;

                if (animation.keepTargetPose) {
                    node->numPersistentAnimations -= 1;
                } else {
                    node->numConcurrentAnimations -= 1;
                }
            }
        }
        const double t = animNow / duration;

        if (t > 0.0001f) {
            UITransform finalLocalAnimPose{};

            const glm::vec2 samplePos  = Interpolate::lerp(localSnapshot.position, animation.targetPose.position, t, animation.easing);
            const glm::vec2 sampleSize = Interpolate::lerp(localSnapshot.size, animation.targetPose.size, t, animation.easing);

            finalLocalAnimPose.position = animation.isPositionAffected() ? samplePos : localSnapshot.position;
            finalLocalAnimPose.size = animation.isSizeAffected() ? sampleSize : localSnapshot.size;

            auto& animFinalTransformPtr = animation.keepTargetPose ? node->animOverrideFinalTransform : node->animAdditiveFinalTransform;
            animFinalTransformPtr += finalLocalAnimPose * resultWeight;

            node->persistentAnimationsTotalWeight += animation.keepTargetPose ? static_cast<float>(resultWeight) : 0;
        }
        time = static_cast<float>(animNow);
    }
}

void ui::UIWindowAnimationContext::beginTransformAnimation(const UIRuntimeObject* uiObj, UITransformAnimationState *animObj,
    const UITransformAnimation &animation) {
    UITransformAnimationData anim{};
    anim.animation = animation;
    anim.animObject = animObj;
    anim.time = 0.f;

    if (animation.keepTargetPose) {
        animObj->numPersistentAnimations += 1;
        animObj->numPersistentAnimationsSnapshot += 1;

        if (animation.isOverride()) {
            anim.localSnapshot = uiObj->localTransform;
        }
    } else {
        animObj->numConcurrentAnimations += 1;
        animObj->numConcurrentAnimationsSnapshot += 1;

        if (animation.isOverride()) {
            anim.animation.targetPose -= uiObj->localTransform;
        }
    }

    if (transformAnimationFreeIndices.empty()) {
        activeTransformAnimations.emplace_back(anim);
    } else {
        auto back = transformAnimationFreeIndices.back();
        activeTransformAnimations[back] = anim;
        transformAnimationFreeIndices.pop_back();
    }
}