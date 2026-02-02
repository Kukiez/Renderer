#pragma once

namespace ui {
    enum class UITransformAnimationBlend {
        ADDITIVE = 0,
        OVERRIDE = 1
    };

    enum class UITransformTarget {
        NONE = 0,
        POSITION = 1 << 0,
        SCALE = 1 << 1,
        ROTATION = 1 << 2,
        POS_SCALE = POSITION | SCALE,
        POS_ROT = POSITION | ROTATION,
        SCALE_ROT = SCALE | ROTATION,
        ALL = POSITION | SCALE | ROTATION
    };

    enum class UIAnimationStatus {
        RUNNING = 0,
        FINISHED = 1
    };

    struct UITransformAnimation {
        UITransformAnimationBlend blend{};
        UITransform targetPose{};
        float duration = 1.f;
        float weight = 1.f;
        float speed = 1.f;

        bool keepTargetPose = false;

        FadeIn fadein{};
        FadeOut fadeout{};

        EasingFunction easing = Easing::linear;
        UITransformTarget target = UITransformTarget::POS_ROT;

        bool isPositionAffected() const { return (target & UITransformTarget::POSITION) == UITransformTarget::POSITION; }
        bool isSizeAffected() const { return (target & UITransformTarget::SCALE) == UITransformTarget::SCALE; }
        bool isRotationAffected() const { return (target & UITransformTarget::ROTATION) == UITransformTarget::ROTATION; }

        bool isAdditive() const { return blend == UITransformAnimationBlend::ADDITIVE; }
        bool isOverride() const { return blend == UITransformAnimationBlend::OVERRIDE; }
    };

    struct UITransformAnimationState {
        UITransform layoutTransform{};
        UITransform localTransform{};
        UITransform animOverrideFinalTransform{};
        UITransform animAdditiveFinalTransform{};
        UITransform finalTransform{};

        float persistentAnimationsTotalWeight = 0;
        uint8_t numPersistentAnimations = 0;
        uint8_t numPersistentAnimationsSnapshot = 0;
        uint8_t numConcurrentAnimations = 0;
        uint8_t numConcurrentAnimationsSnapshot = 0;
    };
}