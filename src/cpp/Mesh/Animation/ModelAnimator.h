#pragma once
#include "Renderer/Common/Interpolation.h"
#include "ModelAnimation.h"

enum class BlendMode {
    OVERRIDE, ADDITIVE, MULTIPLY, NUM_BLEND_MODES
};

struct AnimationTrack {
    struct Keyframe {
        FCurve3 position, scale;
        QCurve rotation;
        size_t pHint = 0, sHint = 0, rHint = 0;
        size_t affected;
        unsigned mesh;
    };

    std::vector<Keyframe> keyframes;
    double currentTime = 0;
    double duration = 0;
    double speed{};
    double weight{};
    BlendMode blend{};
    FadeIn fadein{};
    FadeOut fadeout{};
    bool repeating = false;
};

class ModelAnimator : public PrimaryComponent {
    std::vector<AnimationTrack> tracks;
public:
    void playAnimation(const ModelAnimation& animation, const double duration,
        const double speed, const double weight, const BlendMode blend,
        const FadeIn fadein, const FadeOut fadeout, const bool repeating)
    {
        std::vector<AnimationTrack::Keyframe> keyframes;
        keyframes.reserve(animation.getChannels().size());

        for (const AnimationChannel& channel : animation.getChannels()) {
            keyframes.emplace_back(
                channel.position,
                channel.scale,
                channel.rotation,
                0, 0, 0, 0, channel.mesh
            );
        }
        tracks.emplace_back(
            std::move(keyframes), 0, duration, speed, weight, blend, fadein, fadeout, repeating
        );
    }

    auto& getTracks() {
        return tracks;
    }
};