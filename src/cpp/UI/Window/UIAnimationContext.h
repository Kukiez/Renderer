#pragma once
#include <vector>
#include <UI/UI.h>
#include "UIAnimation.h"

namespace ui {
    class UIWindowAnimationContext {
        struct UITransformAnimationData {
            UITransformAnimationState* animObject{};
            UITransformAnimation animation{};
            double time = 0.f;
            UITransform localSnapshot{};
            bool active = true;
        };

        std::vector<UITransformAnimationData> activeTransformAnimations{};
        std::vector<unsigned> transformAnimationFreeIndices{};

    public:
        UIAPI void advanceTransformAnimations(double deltaTime);

        UIAPI void beginTransformAnimation(const UIRuntimeObject *uiObj, UITransformAnimationState *animObj, const UITransformAnimation &animation);
    };
}
